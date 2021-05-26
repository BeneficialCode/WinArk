#include<ntddk.h>
#include"kstring.h"

#define BUFFER_SIZE 1024
void StringInitTest() {
	ANSI_STRING AnsiString1;
	char* string1 = "hello";
	RtlInitAnsiString(&AnsiString1, string1);
	KdPrint(("AnsiString: %Z\n", &AnsiString1));
	/*string1[0] = 'H';
	string1[1] = 'E';
	string1[2] = 'L';
	string1[3] = 'L';
	string1[4] = 'O';*/	// 这里的内存是只读的
	KdPrint(("AnsiString: %Z\n", &AnsiString1));

	UNICODE_STRING UnicodeString1 = { 0 };
	WCHAR* wString = L"hello";
	size_t size;
	UnicodeString1.MaximumLength = BUFFER_SIZE;
	UnicodeString1.Buffer = (PWSTR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	size =  2 * wcslen(wString);
	UnicodeString1.Length = (USHORT)size;
	ASSERT(UnicodeString1.MaximumLength >= UnicodeString1.Length);
	RtlCopyMemory(UnicodeString1.Buffer, wString, UnicodeString1.Length);
	KdPrint(("UnicodeString:%wZ\n", &UnicodeString1));
	ExFreePool(UnicodeString1.Buffer);
	UnicodeString1.Buffer = NULL;
	UnicodeString1.Length = UnicodeString1.MaximumLength = 0;
}

void StringCopyTest() {
	UNICODE_STRING UnicodeString1;
	UNICODE_STRING UnicodeString2;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");
	UnicodeString2.Buffer = (PWSTR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	UnicodeString2.MaximumLength = BUFFER_SIZE;
	RtlCopyUnicodeString(&UnicodeString2, &UnicodeString1);
	KdPrint(("UnicodeString1: %wZ\n", &UnicodeString1));
	KdPrint(("UnicodeString2: %wZ\n", &UnicodeString2));
	RtlFreeUnicodeString(&UnicodeString2);
}

void StringCompareTest() {
	UNICODE_STRING UnicodeString1;
	UNICODE_STRING UnicodeString2;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");
	RtlInitUnicodeString(&UnicodeString2, L"Hello");
	if (RtlEqualUnicodeString(&UnicodeString1, &UnicodeString2, TRUE)) {
		KdPrint(("UnicodeString1 and UnicodeString2 are equal\n"));
	}
	else {
		KdPrint(("UnicodeString2 and UnicodeString2 are not equal\n"));
	}
}

void StringToUpperTest() {
	UNICODE_STRING UnicodeString1;
	UNICODE_STRING UnicodeString2;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");
	KdPrint(("UnicodeString1 :%wZ\n", &UnicodeString1));
	RtlUpcaseUnicodeString(&UnicodeString2, &UnicodeString1, TRUE);
	KdPrint(("UnicodeString2: %wZ\n", &UnicodeString2));
	RtlFreeUnicodeString(&UnicodeString2);
}

void StringToIntegerTest() {
	UNICODE_STRING UnicodeString1;
	ULONG lNumber;
	NTSTATUS status;
	RtlInitUnicodeString(&UnicodeString1, L"-100");
	status = RtlUnicodeStringToInteger(&UnicodeString1, 10, &lNumber);
	if (NT_SUCCESS(status)) {
		KdPrint(("Convert to integer successfully!\n"));
		KdPrint(("Result:%d\n", lNumber));
	}
	else {
		KdPrint(("Convert to integer unsuccessfully!\n"));
	}

	UNICODE_STRING UnicodeString2 = { 0 };
	UnicodeString2.Buffer = (PWSTR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	UnicodeString2.MaximumLength = BUFFER_SIZE;
	status = RtlIntegerToUnicodeString(200, 10, &UnicodeString2);
	if (NT_SUCCESS(status)) {
		KdPrint(("Convert to string successfully!\n"));
		KdPrint(("Result:%wZ\n", &UnicodeString2));
	}
	else {
		KdPrint(("Convert to string unsuccessfully!\n"));
	}
	RtlFreeUnicodeString(&UnicodeString2);
}

void StringConverTest() {
	UNICODE_STRING UnicodeString1;
	ANSI_STRING AnsiString1;
	NTSTATUS status;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");
	status = RtlUnicodeStringToAnsiString(&AnsiString1, &UnicodeString1, TRUE);
	if (NT_SUCCESS(status)) {
		KdPrint(("Convert successfully!\n"));
		KdPrint(("Result: %Z\n", &AnsiString1));
	}
	else {
		KdPrint(("Convert unsuccessfully!\n"));
	}
	RtlFreeAnsiString(&AnsiString1);

	ANSI_STRING AnsiString2;
	UNICODE_STRING UnicodeString2;
	RtlInitString(&AnsiString2, "Hello World");
	status = RtlAnsiStringToUnicodeString(&UnicodeString2, &AnsiString2, TRUE);
	if (NT_SUCCESS(status)) {
		KdPrint(("Convert successfully!\n"));
		KdPrint(("Result:%wZ\n", &UnicodeString2));
	}
	else {
		KdPrint(("Convert unsuccessfully!\n"));
	}
	RtlFreeUnicodeString(&UnicodeString2);
}

void UnicodeToChar(PUNICODE_STRING dst, char *src) {
	ANSI_STRING string;
	RtlUnicodeStringToAnsiString(&string, dst, TRUE);
	strcpy(src, string.Buffer);
	RtlFreeAnsiString(&string);
}

void WcharToChar(PWCHAR src, PCHAR dst) {
	UNICODE_STRING uString;
	ANSI_STRING aString;
	RtlInitUnicodeString(&uString, src);
	RtlUnicodeStringToAnsiString(&aString, &uString, TRUE);
	strcpy(dst, aString.Buffer);
	RtlFreeAnsiString(&aString);
}

void CharToWchar(PCHAR src, PWCHAR dst) {
	UNICODE_STRING uString;
	ANSI_STRING aString;
	RtlInitString(&aString, src);
	RtlAnsiStringToUnicodeString(&uString, &aString, TRUE);
	wcscpy(dst, uString.Buffer);
	RtlFreeUnicodeString(&uString);
}