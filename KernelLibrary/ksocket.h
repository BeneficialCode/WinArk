#pragma once
/*
	HTTP Virtual Disk.
	Copyright (C) 2006 Bo Brantén.
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/*
 * Basic system type definitions, taken from the BSD file sys/types.h.
 */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

#define MSG_OOB             0x1


int bind(HANDLE socket, const struct sockaddr* addr, int addrlen);
int close(HANDLE socket);
int connect(HANDLE socket, const struct sockaddr* addr, int addrlen);
int getpeername(HANDLE socket, struct sockaddr* addr, int* addrlen);
int getsockname(HANDLE socket, struct sockaddr* addr, int* addrlen);
char* inet_ntoa(struct in_addr addr);
int listen(HANDLE socket, int backlog);
int recv(HANDLE socket, char* buf, int len, int flags);
int recvfrom(HANDLE socket, char* buf, int len, int flags, struct sockaddr* addr, int* addrlen);
int send(HANDLE socket, const char* buf, int len, int flags);
int sendto(HANDLE socket, const char* buf, int len, int flags, const struct sockaddr* addr,
	int addrlen);
int shutdown(HANDLE socket, int how);
HANDLE socket(int af, int type, int protocol);
int stream_recv(HANDLE socket, char* buf, int len);