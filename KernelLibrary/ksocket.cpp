#include "pch.h"
#include <ntddk.h>
#include <tdikrnl.h>
#include <ws2def.h>
#include "ktdi.h"
#include "ksocket.h"

#define SOCK_TAG 'kcos'

typedef struct _STREAM_SOCKET {
	HANDLE              connectionHandle;
	PFILE_OBJECT        connectionFileObject;
	KEVENT              disconnectEvent;
} STREAM_SOCKET, * PSTREAM_SOCKET;

typedef struct _SOCKET {
	int                 type;
	BOOLEAN             isBound;
	BOOLEAN             isConnected;
	BOOLEAN             isListening;
	BOOLEAN             isShuttingdown;
	BOOLEAN             isShared;
	HANDLE              addressHandle;
	PFILE_OBJECT        addressFileObject;
	PSTREAM_SOCKET      streamSocket;
	struct sockaddr     peer;
} SOCKET, * PSOCKET;

NTSTATUS event_disconnect(PVOID TdiEventContext, CONNECTION_CONTEXT ConnectionContext, LONG DisconnectDataLength,
	PVOID DisconnectData, LONG DisconnectInformationLength, PVOID DisconnectionInformation,
	ULONG DisconnectFlags) {
	PSOCKET s = (PSOCKET)TdiEventContext;
	PSTREAM_SOCKET streamSocket = (PSTREAM_SOCKET)ConnectionContext;
	KeSetEvent(&streamSocket->disconnectEvent, IO_NO_INCREMENT, FALSE);
	return STATUS_SUCCESS;
}

int bind(HANDLE socket, const struct sockaddr* addr, int addrlen) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	const struct sockaddr_in* localAddr = (const struct sockaddr_in*)addr;
	UNICODE_STRING devName;
	NTSTATUS status;

	if (s->isBound || addr == nullptr || addrlen < sizeof(struct sockaddr_in)) {
		return -1;
	}

	if (s->type == SOCK_DGRAM) {
		RtlInitUnicodeString(&devName, L"\\Device\\Udp");
	}
	else if (s->type == SOCK_STREAM) {
		RtlInitUnicodeString(&devName, L"\\Device\\Tcp");
	}
	else {
		return -1;
	}

	status = tdi_open_transport_address(
		&devName,
		localAddr->sin_addr.s_addr,
		localAddr->sin_port,
		s->isShared,
		&s->addressHandle,
		&s->addressFileObject
	);

	if (!NT_SUCCESS(status)) {
		s->addressFileObject = nullptr;
		s->addressHandle = (HANDLE)-1;
		return status;
	}

	if (s->type == SOCK_STREAM) {
		tdi_set_event_handler(s->addressFileObject, TDI_EVENT_DISCONNECT, event_disconnect, s);
	}


	s->isBound = TRUE;


	return 0;
}


int close(HANDLE socket) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);

	if (s->isBound) {
		if (s->type == SOCK_STREAM && s->streamSocket) {

			if (s->isConnected) {
				if (!s->isShuttingdown) {
					tdi_disconnect(s->streamSocket->connectionFileObject, TDI_DISCONNECT_RELEASE);
				}
				// wait 3s, if timeout, then force close
				LARGE_INTEGER timeout{ 0 };
				timeout.QuadPart = ((LONGLONG)(-10)) * 1000 * 1000 * 3;
				KeWaitForSingleObject(&s->streamSocket->disconnectEvent, Executive, KernelMode, FALSE, &timeout);
				tdi_unset_event_handler(s->addressFileObject, TDI_EVENT_DISCONNECT);
			}

			if (s->streamSocket->connectionFileObject) {
				tdi_disassociate_address(s->streamSocket->connectionFileObject);
				ObDereferenceObject(s->streamSocket->connectionFileObject);
			}
			if (s->streamSocket->connectionHandle != (HANDLE)-1) {
				ZwClose(s->streamSocket->connectionHandle);
			}
			ExFreePool(s->streamSocket);
		}

		if (s->type == SOCK_DGRAM || s->type == SOCK_STREAM) {
			ObDereferenceObject(s->addressFileObject);
			if (s->addressHandle != (HANDLE)-1) {
				ZwClose(s->addressHandle);
			}
		}
	}

	ExFreePool(s);

	return 0;
}

int connect(HANDLE socket, const struct sockaddr* addr, int addrlen) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	const struct sockaddr_in* remoteAddr = (const struct sockaddr_in*)addr;
	UNICODE_STRING devName;
	NTSTATUS status;

	if (addr == nullptr || addrlen < sizeof(struct sockaddr_in)) {
		return -1;
	}

	if (!s->isBound) {
		struct sockaddr_in localAddr;
		localAddr.sin_family = AF_INET;
		localAddr.sin_port = 0;
		localAddr.sin_addr.s_addr = INADDR_ANY;

		status = bind(socket, (struct sockaddr*)&localAddr, sizeof(localAddr));

		if (!NT_SUCCESS(status)) {
			return status;
		}
	}

	if (s->type == SOCK_STREAM) {
		if (s->isConnected || s->isListening) {
			return -1;
		}

		if (!s->streamSocket) {
			s->streamSocket = (PSTREAM_SOCKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(STREAM_SOCKET), SOCK_TAG);
			if (!s->streamSocket) {
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			RtlZeroMemory(s->streamSocket, sizeof(STREAM_SOCKET));
			s->streamSocket->connectionHandle = (HANDLE)-1;
			KeInitializeEvent(&s->streamSocket->disconnectEvent, NotificationEvent, FALSE);
		}

		RtlInitUnicodeString(&devName, L"\\Device\\Tcp");
		status = tdi_open_connection_endpoint(&devName,
			s->streamSocket,
			s->isShared,
			&s->streamSocket->connectionHandle,
			&s->streamSocket->connectionFileObject);

		if (!NT_SUCCESS(status)) {
			s->streamSocket->connectionFileObject = nullptr;
			s->streamSocket->connectionHandle = (HANDLE)-1;
			return status;
		}

		status = tdi_associate_address(s->streamSocket->connectionFileObject, s->addressHandle);
		if (!NT_SUCCESS(status)) {
			ObDereferenceObject(s->streamSocket->connectionFileObject);
			s->streamSocket->connectionFileObject = nullptr;
			ZwClose(s->streamSocket->connectionHandle);
			s->streamSocket->connectionHandle = (HANDLE)-1;
			return status;
		}

		status = tdi_connect(
			s->streamSocket->connectionFileObject,
			remoteAddr->sin_addr.s_addr,
			remoteAddr->sin_port
		);

		if (!NT_SUCCESS(status)) {
			tdi_disassociate_address(s->streamSocket->connectionFileObject);
			ObDereferenceObject(s->streamSocket->connectionFileObject);
			s->streamSocket->connectionFileObject = nullptr;
			ZwClose(s->streamSocket->connectionHandle);
			s->streamSocket->connectionHandle = (HANDLE)-1;
			return status;
		}
		else {
			s->peer = *addr;
			s->isConnected = TRUE;
			return 0;
		}
	}
	else if (s->type == SOCK_DGRAM) {
		s->peer = *addr;
		if (remoteAddr->sin_addr.s_addr == 0 && remoteAddr->sin_port == 0) {
			s->isConnected = FALSE;
		}
		else {
			s->isConnected = TRUE;
		}
		return 0;
	}
	else {
		return -1;
	}
}

int getpeername(HANDLE socket, struct sockaddr* addr, int* addrlen) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	if (!s->isConnected || addr == nullptr || addrlen == nullptr || *addrlen < sizeof(struct sockaddr_in)) {
		return -1;
	}

	*addr = s->peer;
	*addrlen = sizeof(s->peer);

	return 0;
}

int getsockname(HANDLE socket, struct sockaddr* addr, int* addrlen) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	struct sockaddr_in* localAddr = (sockaddr_in*)addr;

	if (!s->isBound || addr == nullptr || addrlen == nullptr || *addrlen < sizeof(sockaddr_in)) {
		return -1;
	}

	*addrlen = sizeof(sockaddr_in);

	if (s->type == SOCK_DGRAM) {
		return tdi_query_address(s->addressFileObject, &localAddr->sin_addr.s_addr, &localAddr->sin_port);
	}
	else if (s->type == SOCK_STREAM) {
		PFILE_OBJECT FileObject = s->streamSocket && s->streamSocket->connectionFileObject ?
			s->streamSocket->connectionFileObject : s->addressFileObject;
		return tdi_query_address(FileObject,
			&localAddr->sin_addr.s_addr,
			&localAddr->sin_port);
	}
	else {
		return -1;
	}
}

char* inet_ntoa(struct in_addr addr) {
	char* name, * s;
	ULONG n;
	UCHAR byte;

	static UCHAR buf[16] = { 0 };

	name = (char*)buf;

	if (name) {
		for (n = 0, s = name; n < 4; n++) {
			byte = (u_char)((addr.s_addr >> (8 * n)) & 0xff);
			if (byte / 100) {
				*s++ = byte / 100 + '0';
				if (0 == ((byte % 100) / 100)) {
					*s++ = '0';
				}
			}
			if ((byte % 100) / 10) {
				*s++ = (byte % 100) / 10 + '0';
			}
			*s++ = byte % 10 + '0';
			*s++ = '.';
		}
		*--s = '\0';
	}

	return name;
}

int listen(HANDLE socket, int backlog) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	NTSTATUS status;
	UNICODE_STRING devName;

	if (s->type == SOCK_STREAM) {
		if (s->isConnected) {
			return -1;
		}
		if (s->isListening) {
			return 0;
		}

		if (!s->isBound) {
			return 10022L;
		}
		else {
			if (!s->streamSocket) {
				s->streamSocket = (PSTREAM_SOCKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(STREAM_SOCKET), SOCK_TAG);
				if (!s->streamSocket) {
					return STATUS_INSUFFICIENT_RESOURCES;
				}

				RtlZeroMemory(s->streamSocket, sizeof(STREAM_SOCKET));
				s->streamSocket->connectionHandle = (HANDLE)-1;
				KeInitializeEvent(&s->streamSocket->disconnectEvent, NotificationEvent, FALSE);
			}

			RtlInitUnicodeString(&devName, L"\\Device\\Tcp");
			status = tdi_open_connection_endpoint(
				&devName,
				s->streamSocket,
				s->isShared,
				&s->streamSocket->connectionHandle,
				&s->streamSocket->connectionFileObject
			);
			if (!NT_SUCCESS(status)) {
				s->streamSocket->connectionFileObject = nullptr;
				s->streamSocket->connectionHandle = (HANDLE)-1;
				return status;
			}

			status = tdi_associate_address(s->streamSocket->connectionFileObject, s->addressHandle);

			if (!NT_SUCCESS(status)) {
				ObDereferenceObject(s->streamSocket->connectionFileObject);
				s->streamSocket->connectionFileObject = nullptr;
				ZwClose(s->streamSocket->connectionHandle);
				s->streamSocket->connectionHandle = (HANDLE)-1;
				return status;
			}


			if (!s->streamSocket) {
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			s->isListening = TRUE;
			return 0;
		}
	}
	else {
		return -1;
	}

	return -1;
}

int recv(HANDLE socket, char* buf, int len, int flags) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);

	if (s->type == SOCK_DGRAM) {
		
	}
}

int recvfrom(HANDLE socket, char* buf, int len, int flags, struct sockaddr* addr, int* addrlen) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	struct sockaddr_in* returnAddr = (sockaddr_in*)addr;

	if (s->type == SOCK_STREAM) {
		return recv(socket, buf, len, flags);
	}
	else if (s->type == SOCK_DGRAM) {
		u_long* sin_addr = 0;
		u_short* sin_port = 0;

		if (!s->isBound) {
			return -1;
		}

		if (addr != nullptr & addrlen != nullptr && *addrlen >= sizeof(sockaddr_in)) {
			sin_addr = &returnAddr->sin_addr.s_addr;
			sin_port = &returnAddr->sin_port;
			*addrlen = sizeof(sockaddr_in);
		}

		return tdi_recv_dgram(s->addressFileObject, sin_addr, sin_port, buf, len, TDI_RECEIVE_NORMAL);
	}
	else {
		return -1;
	}
}

int send(HANDLE socket, const char* buf, int len, int flags) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);

	if (!s->isConnected) {
		return -1;
	}

	if (s->type == SOCK_DGRAM) {
		return sendto(socket, buf, len, flags, &s->peer, sizeof(s->peer));
	}
}

int sendto(HANDLE socket, const char* buf, int len, int flags, 
	const struct sockaddr* addr, int addrlen) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);
	const sockaddr_in* remoteAddr = (const sockaddr_in*)addr;

	if (s->type == SOCK_STREAM) {
		return send(socket, buf, len, flags);
	}
	else if (s->type == SOCK_DGRAM) {
		sockaddr_in localAddr;
		NTSTATUS status;

		localAddr.sin_family = AF_INET;
		localAddr.sin_port = 0;
		localAddr.sin_addr.s_addr = INADDR_ANY;

		status = bind(socket, (struct sockaddr*)&localAddr, sizeof(localAddr));
		if (!NT_SUCCESS(status)) {
			return status;
		}

		return tdi_send_dgram(s->addressFileObject,
			remoteAddr->sin_addr.s_addr,
			remoteAddr->sin_port,
			buf, len);
	}
	else {
		return -1;
	}
}

int shutdown(HANDLE socket, int how) {
	PSOCKET s = (PSOCKET)(-(INT_PTR)socket);

	if (!s->isConnected) {
		return -1;
	}

	if (s->type == SOCK_STREAM) {
		s->isShuttingdown = TRUE;
		return tdi_disconnect(s->streamSocket->connectionFileObject, TDI_DISCONNECT_RELEASE);
	}
	else {
		return -1;
	}
}

HANDLE socket(int af, int type, int protocol) {
	PSOCKET s;
	
	if (af != AF_INET) {
		return LongToHandle(STATUS_INVALID_PARAMETER);
	}

	s = (PSOCKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(SOCKET), SOCK_TAG);

	if (!s) {
		return LongToHandle(STATUS_INSUFFICIENT_RESOURCES);
	}

	RtlZeroMemory(s, sizeof(SOCKET));

	s->type = type;
	s->addressHandle = (HANDLE)-1;

	return (HANDLE)(-(INT_PTR)s);
}

int stream_recv(HANDLE socket, char* buf, int len) {
	int left, recvBytes;
	char* p;
	left = len;
	p = buf;
	while (left > 0) {
		recvBytes = left > 65536 ? 65536 : left;
		recvBytes = recv(socket, p, recvBytes, 0);
		if (recvBytes < 0)
			return recvBytes;
		left -= recvBytes;
		p += recvBytes;
	}
	return 0;
}