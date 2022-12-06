#include "pch.h"
#include "FilterFileNameInformation.h"

FilterFileNameInformation::FilterFileNameInformation(PFLT_CALLBACK_DATA data, FileNameOptions options) {
	auto status = FltGetFileNameInformation(data, (FLT_FILE_NAME_OPTIONS)options, &m_info);
	if (!NT_SUCCESS(status))
		m_info = nullptr;
}

FilterFileNameInformation::~FilterFileNameInformation() {
	if (m_info)
		FltReleaseFileNameInformation(m_info);
}

NTSTATUS FilterFileNameInformation::Parse() {
	return FltParseFileNameInformation(m_info);
}