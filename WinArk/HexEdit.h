#pragma once


class CHexEdit : public CWindowImpl<CHexEdit, CEdit, CControlWinTraits>
{
public:
	DECLARE_WND_CLASS(L"WTL_HexEdit")

	static const short int _base = 16;
	static const size_t _digits = sizeof(ULONG_PTR) * 2; // 2 digits/byte
	static const size_t _strSize = _digits + 1;

	static inline const TCHAR _strDigits[] = _T("0123456789ABCDEF");
	static const TCHAR _strOldProcProp[];

	// Operations

	BOOL SubclassWindow(HWND hWnd) {
		ATLASSERT(m_hWnd == nullptr);
		ATLASSERT(::IsWindow(hWnd));
		BOOL bRet = CWindowImpl<CHexEdit, CEdit>::SubclassWindow(hWnd);
		if (bRet)
			_Init();
		return bRet;
	}

	ULONG_PTR GetValue() const {
		ATLASSERT(::IsWindow(m_hWnd));
		TCHAR str[_strSize] = { 0 };
		GetWindowText(str, _countof(str));
		return _StringToNum(str);
	}

	void SetValue(ULONG_PTR num, bool fill = true) {
		ATLASSERT(::IsWindow(m_hWnd));
		TCHAR str[_strSize] = { 0 };
		_NumToString(num, str, fill);
		SetWindowText(str);
	}

	// Implementation

	void _Init() {
		ATLASSERT(::IsWindow(m_hWnd));

		LimitText(_digits);
	}

	bool _IsValidChar(TCHAR c) const {
		return ((ULONG_PTR)-1 != _CharToNum(c));
	}

	bool _IsValidString(const TCHAR str[_strSize]) const {
		for (int i = 0; str[i]; i++) {
			if (!_IsValidChar(str[i])) {
				return false;
			}
		}
		return true;
	}

	ULONG_PTR _CharToNum(TCHAR c) const {
		c = _totupper(c);
		for (int i = 0; _strDigits[i]; i++) {
			if (c == _strDigits[i])
				return i;
		}

		return -1;
	}

	ULONG_PTR _StringToNum(const TCHAR str[_strSize]) const {
		ULONG_PTR cNum;
		ULONG_PTR num = 0;

		for (int i = 0; str[i]; i++) {
			cNum = _CharToNum(str[i]);
			if (cNum == (ULONG_PTR)-1)
				break;

			num *= _base;
			num += cNum;
		}

		return num;
	}

	TCHAR _NumToChar(ULONG_PTR num) const {
		return _strDigits[num % _base];
	}

	void _NumToString(ULONG_PTR num, TCHAR str[_strSize], bool fill) const {
		ULONG_PTR nums[_digits];
		int i, j;
		for (i = _digits - 1; i >= 0; i--) {
			nums[i] = num % _base;
			num /= _base;
		}
		for (i = j = 0; i < _digits; i++) {
			// Only copy num if : non-null OR Fill OR non-null encountered before OR last num
			if (nums[i] || fill || j || i == _digits - 1) {
				str[j++] = _NumToChar(nums[i]);
			}
		}
		str[j] = '\0';
	}

	bool _GetClipboardText(TCHAR str[_strSize]) {
#ifdef UNICODE
		const UINT format = CF_UNICODETEXT;
#else
		const UINT format = CF_TEXT;
#endif // UNICODE

		bool retVal = false;

		if (IsClipboardFormatAvailable(format) && OpenClipboard()) {
			HANDLE hMem = GetClipboardData(format);
			if (hMem) {
				_tcsncpy_s(str, _strSize, (TCHAR*)GlobalLock(hMem), _strSize - 1);
				str[_strSize - 1] = '\0';
				GlobalUnlock(hMem);
				retVal = true;
			}
			CloseClipboard();
		}

		return retVal;
	}

	BEGIN_MSG_MAP_EX(CHexEdit)
		MESSAGE_HANDLER_EX(WM_CREATE, OnCreate)
		MSG_WM_SETTEXT(OnSetText)
		MSG_WM_PASTE(OnPaste)
		MSG_WM_CHAR(OnChar)
	END_MSG_MAP()


	LRESULT OnCreate(UINT uMsg, WPARAM, LPARAM) {
		LRESULT lRes = DefWindowProc();
		_Init();
		return lRes;
	}

	int OnSetText(LPCTSTR text) {
		bool passThru = (_tcslen(text) <= _digits) && _IsValidString(text);
		if (!passThru) {
			MessageBeep(-1);
			return FALSE;
		}

		SetMsgHandled(FALSE);
		return TRUE;
	}

	void OnPaste() {
		TCHAR str[_strSize];
		bool passThru = !_GetClipboardText(str) || _IsValidString(str);
		if (!passThru) {
			MessageBeep(-1);
			return;
		}
		SetMsgHandled(FALSE);
		return;
	}

	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
		// ignore all printable chars (incl. space) which are not valid digits
		bool passThru = !_istprint(nChar) || _IsValidChar(nChar);
		if (!passThru) {
			MessageBeep(-1);
			return;
		}
		SetMsgHandled(FALSE);
	}
};