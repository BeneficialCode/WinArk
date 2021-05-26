#pragma once

void RegCreateKey(LPWSTR KeyName);
void RegRenameKey(LPWSTR OldKeyName, LPWSTR NewKeyName);
void RegDeleteKey(LPWSTR KeyName);
void RegSetValueKey(LPWSTR KeyName, LPWSTR ValueName, ULONG DataType, PVOID DataBuffer, ULONG DataLength);
NTSTATUS RegQueryValueKey(LPWSTR KeyName, LPWSTR ValueName, PKEY_VALUE_PARTIAL_INFORMATION *pkvpi);
void RegDeleteValueKey(LPWSTR KeyName, LPWSTR ValueName);
void EnumSubKeyTest();
void EnumSubValueTest();
