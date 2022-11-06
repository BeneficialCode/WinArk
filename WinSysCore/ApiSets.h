#pragma once
#include <set>
#include <map>


struct ApiSetEntry {
	std::wstring Name;
	// hosts
	std::vector<std::wstring> Values, Aliases;
	bool Sealed;
};

// API Sets exist starting from Windows 7.
class ApiSets {
public:
	ApiSets() = default;

	void SearchFiles();

	const std::vector<ApiSetEntry>& GetApiSets() const;
	bool IsFileExists(const wchar_t* name) const;
	void Build(HANDLE hProcess);

	

private:
	struct LessNoCase {
		bool operator()(const std::wstring& s1, const std::wstring& s2) const {
			return _wcsicmp(s1.c_str(), s2.c_str()) < 0;
		}
	};

	using FileSet = std::set<std::wstring, LessNoCase>;


	void SearchDirectory(const std::wstring& directory, const std::wstring& pattern, FileSet& files);

private:
	std::vector<ApiSetEntry> _entries;
	FileSet _files;
	HANDLE m_hProcess;
};