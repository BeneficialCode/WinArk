#include "pch.h"
#include "Logging.h"
#include <stdarg.h>

ULONG Log(LogLevel level, PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintExWithPrefix(DRIVER_PREFIX, DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(level), format, list);
}

ULONG LogError(PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintExWithPrefix(DRIVER_PREFIX, DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(LogLevel::Error), format, list);
}

ULONG LogInfo(PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintExWithPrefix(DRIVER_PREFIX, DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(LogLevel::Information), format, list);
}

ULONG LogDebug(PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintExWithPrefix(DRIVER_PREFIX, DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(LogLevel::Debug), format, list);
}

ULONG LogVerbose(PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintExWithPrefix(DRIVER_PREFIX, DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(LogLevel::Verbose), format, list);
}