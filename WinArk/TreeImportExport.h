#pragma once
#include "Thunks.h"
#include <tinyxml2.h>

class TreeImportExport
{
public:
	TreeImportExport(const WCHAR* pTargetXmlFile);

	bool ExportTreeList(const std::map<DWORD_PTR, ImportModuleThunk>& moduleThunk, const std::string processName,
		DWORD_PTR oep, DWORD_PTR iat, DWORD iatSize);
	bool ImportTreeList(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunk, 
		DWORD_PTR* pOEP, DWORD_PTR* pIAT, DWORD* pSize);

private:

	WCHAR _xmlPath[MAX_PATH] = { 0 };
	char _xmlStringBuffer[MAX_PATH];

	void SetTargetInformation(tinyxml2::XMLElement* pRootElement, std::string processName,
		DWORD_PTR oep, DWORD_PTR iat, DWORD iatSize);
	void AddModuleListToRootElement(tinyxml2::XMLElement* pRootElement,
		const std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);

	void ParseAllElementModules(tinyxml2::XMLElement* pTargetElement, 
		std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void ParseAllElementImports(tinyxml2::XMLElement* pModuleElement, ImportModuleThunk* pModuleThunk);

	tinyxml2::XMLElement* GetModuleXmlElement(tinyxml2::XMLElement* pParentElement,const ImportModuleThunk* pModuleThunk);
	tinyxml2::XMLElement* GetImportXmlElement(tinyxml2::XMLElement* pParentElement,const ImportThunk* pThunk);

	bool SaveXmlToFile(tinyxml2::XMLDocument& doc, const WCHAR* pXmlFilePath);
	bool ReadXmlFile(tinyxml2::XMLDocument& doc, const WCHAR* pXmlFilePath);

	void ConvertBoolToString(const bool value);
	void ConvertWordToString(const WORD value);
	void ConvertDwordPtrToString(const DWORD_PTR value);

	DWORD_PTR ConvertStringToDwordPtr(const char* pValue);
	WORD ConvertStringToWord(const char* pValue);
	bool ConvertStringToBool(const char* pValue);
};