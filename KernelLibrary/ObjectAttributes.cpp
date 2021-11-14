#include "pch.h"
#include "ObjectAttributes.h"

ObjectAttributes::ObjectAttributes(PUNICODE_STRING name, ObjectAttributesFlags flags /* = ObjectAttributesFlags::None */, HANDLE rootDirectory /* = nullptr */, PSECURITY_DESCRIPTOR sd /* = nullptr */) {
	InitializeObjectAttributes(this, name, static_cast<ULONG>(flags), rootDirectory, sd);
}