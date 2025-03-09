#include "stdafx.h"
#include "TreeImportExport.h"
#include "Architecture.h"
#include <Helpers.h>

TreeImportExport::TreeImportExport(const WCHAR* pTargetXmlFile){
	wcscpy_s(_xmlPath, pTargetXmlFile);
}

bool TreeImportExport::ExportTreeList(const std::map<DWORD_PTR, ImportModuleThunk>& moduleThunk, const std::string processName, 
	DWORD_PTR oep, DWORD_PTR iat, DWORD iatSize){
	// 定义doc对象
	tinyxml2::XMLDocument doc;
	// 增加xml文档声明
	tinyxml2::XMLDeclaration* pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);

	// 添加节点
	tinyxml2::XMLElement* pRootElement = doc.NewElement("target");

	SetTargetInformation(pRootElement, processName, oep, iat, iatSize);
	AddModuleListToRootElement(pRootElement, moduleThunk);

	doc.LinkEndChild(pRootElement);

	return SaveXmlToFile(doc, _xmlPath);
}

bool TreeImportExport::ImportTreeList(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunk, 
	DWORD_PTR* pOEP, DWORD_PTR* pIAT, DWORD* pSize) {
	moduleThunk.clear();
	*pOEP = *pIAT = 0;
	*pSize = 0;

	tinyxml2::XMLDocument doc;
	if (!ReadXmlFile(doc, _xmlPath)) {
		return false;
	}

	tinyxml2::XMLElement* pTargetElement = doc.FirstChildElement();
	if (!pTargetElement) {
		return false;
	}

	*pOEP = ConvertStringToDwordPtr(pTargetElement->Attribute("oep_va"));
	*pIAT = ConvertStringToDwordPtr(pTargetElement->Attribute("iat_va"));
	*pSize = ConvertStringToDwordPtr(pTargetElement->Attribute("iat_size"));

	ParseAllElementModules(pTargetElement, moduleThunk);

	return true;
}

void TreeImportExport::SetTargetInformation(tinyxml2::XMLElement* pRootElement, std::string processName,
	DWORD_PTR oep, DWORD_PTR iat, DWORD iatSize) {
	pRootElement->SetAttribute("process_name", processName.c_str());

	ConvertDwordPtrToString(oep);
	pRootElement->SetAttribute("oep_va", _xmlStringBuffer);

	ConvertDwordPtrToString(iat);
	pRootElement->SetAttribute("iat_va", _xmlStringBuffer);

	ConvertDwordPtrToString(iatSize);
	pRootElement->SetAttribute("iat_size", _xmlStringBuffer);
}

void TreeImportExport::AddModuleListToRootElement(tinyxml2::XMLElement* pRootElement, 
	const std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap){
	
	for (auto& moduleThunk : moduleThunkMap) {
		const ImportModuleThunk& importModultThunk = moduleThunk.second;
		tinyxml2::XMLElement* pModuleElement = GetModuleXmlElement(pRootElement,&importModultThunk);

		for (auto& thunk : importModultThunk.m_ThunkMap) {
			const ImportThunk& importThunk = thunk.second;
			tinyxml2::XMLElement* pImportElement = GetImportXmlElement(pModuleElement, &importThunk);
		}
	}
}


void TreeImportExport::ParseAllElementModules(tinyxml2::XMLElement* pTargetElement, 
	std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap)
{
	ImportModuleThunk importModuleThunk;

	for (tinyxml2::XMLElement* pModuleElement = pTargetElement->FirstChildElement();
		pModuleElement; pModuleElement = pModuleElement->NextSiblingElement()) {
		std::string moduleName = pModuleElement->Attribute("module_name");
		std::wstring wname = Helpers::StringToWstring(moduleName);
		wcscpy_s(importModuleThunk.m_ModuleName, wname.c_str());
		if (!moduleName.empty()) {
			importModuleThunk.m_FirstThunk = ConvertStringToDwordPtr(pModuleElement->Attribute("first_thunk_rva"));
			importModuleThunk.m_ThunkMap.clear();
			ParseAllElementImports(pModuleElement, &importModuleThunk);

			moduleThunkMap[importModuleThunk.m_FirstThunk] = importModuleThunk;
		}
	}
}

void TreeImportExport::ParseAllElementImports(tinyxml2::XMLElement* pModuleElement, 
	ImportModuleThunk* pModuleThunk)
{
	ImportThunk importThunk;
	for (tinyxml2::XMLElement* pImportElement = pModuleElement->FirstChildElement();
		pImportElement; pImportElement = pImportElement->NextSiblingElement()) {
		const char* pTemp = pImportElement->Value();
		if (!strcmp(pTemp, "import_valid")) {
			pTemp = pImportElement->Attribute("name");
			if (pTemp) {
				strcpy_s(importThunk.m_Name, pTemp);
			}
			else {
				importThunk.m_Name[0] = 0;
			}

			wcscpy_s(importThunk.m_ModuleName, pModuleThunk->m_ModuleName);

			importThunk.m_Suspect = ConvertStringToBool(pImportElement->Attribute("suspect"));
			importThunk.m_Ordinal = ConvertStringToWord(pImportElement->Attribute("ordinal"));
			importThunk.m_Hint = ConvertStringToWord(pImportElement->Attribute("hint"));

			importThunk.m_Valid = true;
		}
		else {
			importThunk.m_Valid = false;
			importThunk.m_Suspect = true;
		}

		importThunk.m_ApiAddressVA = ConvertStringToDwordPtr(pImportElement->Attribute("address_va"));
		importThunk.m_RVA = ConvertStringToDwordPtr(pImportElement->Attribute("iat_rva"));

		if (importThunk.m_RVA != 0) {
			pModuleThunk->m_ThunkMap[importThunk.m_RVA] = importThunk;
		}
	}
}

tinyxml2::XMLElement* TreeImportExport::GetModuleXmlElement(tinyxml2::XMLElement* pParentElement, 
	const ImportModuleThunk* pModuleThunk)
{
	tinyxml2::XMLElement* pModuleElement = pParentElement->InsertNewChildElement("module");
	std::string moduleName = Helpers::WstringToString(pModuleThunk->m_ModuleName);
	pModuleElement->SetAttribute("module_name", moduleName.c_str());

	ConvertDwordPtrToString(pModuleThunk->GetFirstThunk());
	pModuleElement->SetAttribute("first_thunk_rva", _xmlStringBuffer);

	return pModuleElement;
}

tinyxml2::XMLElement* TreeImportExport::GetImportXmlElement(tinyxml2::XMLElement* pParentElement, 
	const ImportThunk* pThunk)
{	
	tinyxml2::XMLElement* pImportElement = nullptr;
	if (pThunk->m_Valid) {
		pImportElement = pParentElement->InsertNewChildElement("import_valid");
		if (pThunk->m_Name[0] != '\0') {
			pImportElement->SetAttribute("name", pThunk->m_Name);
		}

		ConvertWordToString(pThunk->m_Ordinal);
		pImportElement->SetAttribute("ordinal", _xmlStringBuffer);

		ConvertWordToString(pThunk->m_Hint);
		pImportElement->SetAttribute("hint", _xmlStringBuffer);

		ConvertBoolToString(pThunk->m_Suspect);
		pImportElement->SetAttribute("suspect", _xmlStringBuffer);
	}
	else {
		pImportElement = pParentElement->InsertNewChildElement("import_invalid");
	}

	ConvertDwordPtrToString(pThunk->m_RVA);
	pImportElement->SetAttribute("iat_rva", _xmlStringBuffer);

	ConvertDwordPtrToString(pThunk->m_ApiAddressVA);
	pImportElement->SetAttribute("address_va", _xmlStringBuffer);

	return pImportElement;
}


bool TreeImportExport::SaveXmlToFile(tinyxml2::XMLDocument& doc, const WCHAR* pXmlFilePath){
	std::string xmlFilePath = Helpers::WstringToString(pXmlFilePath);
	tinyxml2::XMLError error = doc.SaveFile(xmlFilePath.c_str());
	return error == tinyxml2::XML_SUCCESS ? true : false;
}

bool TreeImportExport::ReadXmlFile(tinyxml2::XMLDocument& doc, const WCHAR* pXmlFilePath){
	std::string xmlFilePath = Helpers::WstringToString(pXmlFilePath);
	tinyxml2::XMLError error = doc.LoadFile(xmlFilePath.c_str());
	return error == tinyxml2::XML_SUCCESS ? true : false;
}

void TreeImportExport::ConvertBoolToString(const bool value) {
	if (value) {
		strcpy_s(_xmlStringBuffer, "1");
	}
	else {
		strcpy_s(_xmlStringBuffer, "0");
	}
}

void TreeImportExport::ConvertWordToString(const WORD value)
{
	sprintf_s(_xmlStringBuffer, "%04X", value);
}

void TreeImportExport::ConvertDwordPtrToString(const DWORD_PTR value)
{
	sprintf_s(_xmlStringBuffer, PRINTF_DWORD_PTR_FULL_S, value);
}

DWORD_PTR TreeImportExport::ConvertStringToDwordPtr(const char* pValue)
{
	DWORD_PTR result = 0;

	if (pValue)
	{
#ifdef _WIN64
		result = _strtoi64(pValue, NULL, 16);
#else
		result = strtoul(pValue, NULL, 16);
#endif
	}

	return result;
}

WORD TreeImportExport::ConvertStringToWord(const char* pValue)
{
	WORD result = 0;

	if (pValue)
	{
		result = (WORD)strtoul(pValue, NULL, 16);
	}

	return result;
}

bool TreeImportExport::ConvertStringToBool(const char* pValue)
{
	if (pValue)
	{
		if (pValue[0] == '1')
		{
			return true;
		}
	}

	return false;
}