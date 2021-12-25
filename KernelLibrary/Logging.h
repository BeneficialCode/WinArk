#pragma once

enum class LogLevel {
	Error = 0,
	Warning,
	Information,
	Debug,
	Verbose
};

#ifndef DRIVER_PREFIX
#define DRIVER_PREFIX "[Log]: "
#endif // !DRIVER_PREFIX

ULONG Log(LogLevel level, PCSTR format, ...);
ULONG LogError(PCSTR format, ...);
ULONG LogInfo(PCSTR format, ...);
ULONG LogDebug(PCSTR format, ...);
ULONG LogVerbose(PCSTR format, ...);