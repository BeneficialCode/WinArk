#pragma once

class RegHelper final {
public:
	static HANDLE OpenKey(const CString& path, ACCESS_MASK access);
};