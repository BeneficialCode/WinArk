#pragma once
#include <string>
#include <algorithm>
#include <atlstr.h>
#include <vector>
#include <memory>
#include "SortHelper.h"
#include "Helpers.h"

constexpr int NBAR = 30;

typedef unsigned char  uchar;          // Unsigned character (byte)
typedef unsigned short ushort;         // Unsigned short
typedef unsigned int   uint;           // Unsigned integer
typedef unsigned long  ulong;          // Unsigned long

#define BAR_PRESSED    0x01            // Bar segment pressed, used internally
#define BAR_DISABLED   0x02            // Bar segment disabled
#define BAR_NOSORT     0x04            // Flat bar column, supports no sorting
#define BAR_NORESIZE   0x08            // Bar column cannot be resized
#define BAR_BUTTON     0x10            // Segment sends WM_USER_BAR
#define BAR_SHIFTSEL   0x20            // Selection shifted 1/2 char to left

#define CAPT_FREE      0               // Bar and data are not captured
#define CAPT_SPLIT	   0x1
#define CAPT_BAR	   0x2
#define CAPT_DATA	   0x3

#define DRAW_NORMAL    0x0000          // Normal plain text
#define DRAW_GRAY      0x0001          // Grayed text
#define DRAW_HILITE    0x0002          // 绘制高亮文本
#define DRAW_UL        0x0004          // Underlined text
#define DRAW_SELECT    0x0008          // 绘制选中背景色
#define DRAW_EIP       0x0010          // Inverted normal text/background
#define DRAW_BREAK     0x0020          // 绘制断点背景色
#define DRAW_GRAPH     0x0040          // Graphical element
#define DRAW_DIRECT    0x0080          // Direct text colour index (mask only)
#define DRAW_MASK      0x0080          // Use mask to set individual colors
#define DRAW_EXTSEL    0x0100          // Extend mask till end of column
#define DRAW_UNICODE   0x0200          // Text in UNICODE
#define DRAW_TOP       0x0400          // 绘制文本上半截
#define DRAW_BOTTOM    0x0800          // 绘制文本下半截

#define TABLE_DIR      0x0001          // Bottom-to-top table
#define TABLE_COPYMENU 0x0002          // Attach copy item
#define TABLE_SORTMENU 0x0004          // Attach sort menu
#define TABLE_APPMENU  0x0010          // Attach appearance menu
#define TABLE_WIDECOL  0x0020          // Attach wide columns menu item
#define TABLE_USERAPP  0x0040          // Attach user-processed appearance menu
#define TABLE_USERDEF  0x0080          // User-drawn table
#define TABLE_NOHSCR   0x0100          // Table contains no horizontal scroll
#define TABLE_SAVEPOS  0x0200          // Save position & appearance to .ini
#define TABLE_CPU      0x0400          // Table belongs to CPU window
#define TABLE_FASTSEL  0x0800          // Update when selection changes
#define TABLE_COLSEL   0x1000          // Column-wide selection
#define TABLE_SAVEAPP  0x2000          // Save multiinstance appearance to .ini
#define TABLE_HILMENU  0x4000          // Attach Syntax highlighting menu
#define TABLE_ONTOP    0x8000          // Attach Always on top menu

// Symbolic names for graphical characters. Any other character is displayed
// as space. Use only characters in range [1..0x7F]!
#define D_SPACE        'N'             // Space
#define D_SEP          ' '             // 分割线
#define D_BEGIN        'B'             // 函数头
#define D_BODY         'I'             // 函数体
#define D_ENTRY        'J'             // Loop entry point
#define D_LEAF         'K'             // Intermediate leaf on a tree
#define D_END          'E'             // 函数尾
#define D_SINGLE       'S'             // Single-line scope
#define D_ENDBEG       'T'             // End and begin of stack scope
#define D_POINT        '.'             // Point
#define D_JMPUP        'U'             // Jump upstairs
#define D_JMPOUT       '<'             // 跳转到模块外，或者同一个地方
#define D_JMPDN        'D'             // 向下跳转
#define D_PATHUP       'u'             // Jump path upstairs (highlighted)
#define D_GRAYUP       'v'             // Jump path upstairs (grayed)
#define D_PATH         'i'             // Jump path through text (highlighted)
#define D_GRAYPATH     'j'             // Jump path through text (grayed)
#define D_PATHDN       'd'             // Jump path downstairs (highlighted)
#define D_GRAYDN       'e'             // Jump path downstairs (grayed)
#define D_PATHUPEND    'r'             // End of path upstairs (highlighted)
#define D_GRAYUPEND    's'             // End of path upstairs (grayed)
#define D_PATHDNEND    'f'             // End of path downstairs (highlighted)
#define D_GRAYDNEND    'g'             // End of path downstairs (grayed)
#define D_SWTOP        't'             // switch 开始
#define D_SWBODY       'b'             // switch 体
#define D_CASE         'c'             // Intermediate switch case
#define D_LASTCASE     'l'             // Last switch case

#define WM_USER_MENU   (WM_USER+101)   // 激活上下文菜单	// 0x465
#define WM_USER_SCR    (WM_USER+102)   // Redraw scroll(s)	0x466
#define WM_USER_SAVE   (WM_USER+103)   // Save data to disk			0x467
#define WM_USER_VABS   (WM_USER+104)   // Scroll contents of window by lines	0x468
#define WM_USER_VREL   (WM_USER+105)   // Scroll contents of window by percent	0x469
#define WM_USER_VBYTE  (WM_USER+106)   // Scroll contents of window by bytes	0x46a
#define WM_USER_STS    (WM_USER+107)   // Start selection in window		0x46b
#define WM_USER_CNTS   (WM_USER+108)   // Continue selection in window	0x46c
#define WM_USER_CHGS   (WM_USER+109)   // 移动单行选择		0x46d
#define WM_USER_BAR    (WM_USER+110)   // Message from bar segment as button	0x46e
#define WM_USER_DBLCLK (WM_USER+111)   // Doubleclick in column			0x46f
#define WM_USER_SIZE   (WM_USER+112)   // Resize children in CPU window	0x470
#define WM_USER_FOCUS  (WM_USER+113)   // Set focus to child of CPU window	0x471
#define WM_USER_FILE   (WM_USER+114)   // Change state of file dump		0x472
#define WM_USER_HERE   (WM_USER+115)   // Query presence list		0x473
#define WM_USER_CHALL  (WM_USER+116)   // Redraw (almost) everything		0x474
#define WM_USER_CHMEM  (WM_USER+117)   // Range of debuggee's memory changed	0x475
#define WM_USER_CHREG  (WM_USER+118)   // Debuggee's register(s) changed	// 0x476
#define WM_USER_CHNAM  (WM_USER+119)   // Redraw name tables		//	0x477
#define WM_USER_MOUSE  (WM_USER+120)   // Check mouse coordinates	// 0x478
#define WM_USER_KEY    (WM_USER+121)   // Emulate WM_KEYDOWN		// 0x479
#define WM_USER_SYSKEY (WM_USER+122)   // Emulate WM_SYSKEYDOWN		// 0x47A

struct t_bar {
	int nbar;					// 列数
	int font;					// 字体
	int dx[NBAR];				// 实际宽度
	int defdx[NBAR];			// 默认宽度
	std::string name[NBAR];		// 名字
	uchar mode[NBAR];			// 模式
	int captured;				// 是否被捕获
	int active;					// 鼠标捕获的哪一列
	int prevx;					// 鼠标之前的坐标
};

struct BarDesc {
	int defdx;
	std::string name;
	uchar mode;
};

struct BarInfo {
	BarDesc bar[NBAR];
	int font;
	int nbar;
	int captured;
};

struct TableInfo {
	int showbar;
	int hscroll;
	int mode;
	int font;
	int scheme;
	int offset;
	int xshift;
};

template<typename T>
struct t_sorted {
	std::string name;				// 表名
	size_t n{0};					// 实际项数
	int nmax;						// 最大项数
	int selected;					// 选中索引
	ulong seladdr;					// 选中的基地址
	bool sorted;					// 索引是否被排序
	int sort;						// 被排序的列
	int version{ 0 };				// 版本
	std::vector<int> index[NBAR];	// 索引
	bool AutoSort{ false };		// 是否可以自动排序
	std::vector<T> info;
};

template<typename T>
struct t_table {
	HWND hw;			// 窗口句柄
	t_sorted<T> data;	// 数据
	int showbar;		// 显示，隐藏，缺省
	short hscroll;		// 水平滚动条，显示，隐藏
	short colsel;		// 选中表格第二项
	int mode;			// 模式
	int font;			// 字体
	short scheme;		// 色彩主题
	short hilite;		// 高亮主题
	size_t offset;			// 显示首行位置
	int xshift;			// 位置移动
};

struct t_process {
	DWORD pid;
	CString ProcessName;
	CString Path;
};

template<class T>
class CTable {
public:
	CTable(BarInfo& bars,TableInfo& table);

	void Defaultbar();
	void PaintBar(HDC hdc, LPRECT rect, int iColumn);
	void PaintTable(HWND hw);
	int Tablefunction(HWND hw, UINT msg, WPARAM wp, LPARAM lp);
	
	virtual int GetRowImage(HWND, size_t row) const {
		return -1;
	}

	bool Sortsorteddata(int sort);
	virtual bool CompareItems(const T& i1, const T& i2, int col,bool asc=true) {
		return false;
	}

	T Getsortedbyselection(int row);

	template<class T>
	static DWORD GetSeladdr(const T& t) {
		return 0;
	}
	
	template<>
	static DWORD GetSeladdr(const t_process& t) {
		return t.pid;
	}

	static void Addsorteddata(const T& t);

	static void Deletesorteddata(ulong addr);

	virtual int ParseTableEntry(CString& s, char& mask, int& select, T& info, int column) {
		return 0;
	}

	void GetSysKeyState(UINT msg, LPARAM lp, bool& pressShift, bool& pressCtrl);

	void SetTableWindowInfo(int maxColumns,int nlines = 20);

public:
	t_bar m_Bar;
	static RECT m_Rect;
	static DWORD _style;
	static t_table<T> m_Table;
	bool m_bSnowFreeDrawing{ true };	// 是否内存绘图
	bool m_bHighlightSortedColumn{ false };
	int m_BkColorIndex[8] = { -1,0,7,15,11,10,12,14 };
	int _row;
	int _page{ 0 };
	mutable CImageList m_Images;
};

template <class T>
DWORD CTable<T>::_style = 0;

template <class T>
RECT CTable<T>::m_Rect = { 0 };

template <class T>
t_table<T> CTable<T>::m_Table;

template<class T>
T CTable<T>::Getsortedbyselection(int row) {
	return m_Table.data.info[row];
}

template<class T>
void CTable<T>::GetSysKeyState(UINT msg, LPARAM lp, bool& pressShift, bool& pressCtrl) {
	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
		pressShift = GetKeyState(VK_SHIFT) & 0x8000;
		pressCtrl = GetKeyState(VK_CONTROL) & 0x8000;
	}
	else {
		pressShift = LOWORD(lp);
		pressCtrl = HIWORD(lp);
	}
}

template<class T>
CTable<T>::CTable(BarInfo& bars,TableInfo& table) {
	ATLASSERT(bars.nbar < NBAR&& bars.nbar>0);
	ATLASSERT(bars.font > 0 && bars.font < 11);

	m_Table.showbar = table.showbar;
	m_Table.hscroll = table.hscroll;
	m_Table.mode = table.mode;
	m_Table.font = table.font;
	m_Table.scheme = table.scheme;
	m_Table.offset = table.offset;
	m_Table.xshift = table.xshift;

	m_Bar.font = bars.font;
	m_Bar.nbar = bars.nbar;
	m_Bar.captured = bars.captured;

	for (int i = 0; i < bars.nbar; i++) {
		m_Bar.defdx[i] = bars.bar[i].defdx;
		m_Bar.name[i] = bars.bar[i].name;
		m_Bar.mode[i] = bars.bar[i].mode;
	}

	Defaultbar();
}

template<class T>
void CTable<T>::Defaultbar() {
	int i = 0;
	while (i < m_Bar.nbar) {
		m_Bar.dx[i] = g_AvWidthFont * m_Bar.defdx[i];
		++i;
	}
}

template<class T>
void CTable<T>::PaintBar(HDC hdc, LPRECT client, int iColumn) {
	ATLASSERT(hdc && client);
	int font;
	RECT bar, col;
	TEXTMETRIC tmFont;
	if (client->left < client->right && client->top < client->bottom) {
		font = m_Bar.font;
		SetTextAlign(hdc, TA_LEFT | TA_BOTTOM);
		SetBkColor(hdc, g_crLightGray);
		SelectObject(hdc, g_hAppFont);
		GetTextMetrics(hdc, &tmFont);
		g_AvHighFont = tmFont.tmHeight;
		memcpy(&bar, client, sizeof(bar));
		client->top += g_AvHighFont + 4;
		bar.bottom = client->top - 1;
		SelectObject(hdc, g_hBlackPen);
		MoveToEx(hdc, bar.left, bar.bottom, NULL);
		LineTo(hdc, bar.right, bar.bottom);	// 画出列标题的框

		int i = 0;
		while (m_Bar.nbar > i) {
			if (m_Bar.dx[i] > 0) {
				if (client->right <= bar.left)
					break;
				memcpy(&col, &bar, sizeof(col));
				bar.left = col.left + m_Bar.dx[i];
				col.right = bar.left - 1;
				SelectObject(hdc, g_hBlackPen);
				MoveToEx(hdc, col.right, col.top, NULL);
				LineTo(hdc, col.right, col.bottom);	// 右边界为黑色
				int x = col.left + g_AvWidthFont / 2;
				int y = col.bottom - 1;
				// 设置边框分格
				if (m_Bar.mode[i] & BAR_NOSORT) {	// 不可排序上边界为白线
					SelectObject(hdc, g_hWhitePen);
					MoveToEx(hdc, col.left, col.top, NULL);
					LineTo(hdc, col.right, col.top);
					++col.top;
				}
				else if ((m_Bar.mode[i] & BAR_PRESSED) == BAR_PRESSED) {	// 按压时,左边界和上边界为黑灰色
					SelectObject(hdc, g_hDarkGrayPen);
					MoveToEx(hdc, col.left, col.bottom - 1, NULL);
					LineTo(hdc, col.left, col.top);
					LineTo(hdc, col.right, col.top);
					++col.left;
					++col.top;
					++x;
					++y;
				}
				else {
					--col.bottom;
					SelectObject(hdc, g_hWhitePen);				// 左边界为白色
					MoveToEx(hdc, col.left, col.bottom, NULL);
					LineTo(hdc, col.left, col.top);
					LineTo(hdc, col.right, col.top);
					++col.left;
					--col.right;
					SelectObject(hdc, g_hDarkGrayPen);			// 右边界为黑灰色
					MoveToEx(hdc, col.left, col.bottom, NULL);
					LineTo(hdc, col.right, col.bottom);
					LineTo(hdc, col.right, col.top);
					++col.top;
				}
				// 设置字体颜色和背景颜色
				if (m_Bar.mode[i] & BAR_DISABLED) {
					SetTextColor(hdc, g_crDarkGray);
					SetBkColor(hdc, g_crLightGray);
				}
				else if (m_Bar.mode[i] & BAR_NOSORT || !m_bHighlightSortedColumn || i != iColumn) {
					SetTextColor(hdc, g_crBlack);
					SetBkColor(hdc, g_crLightGray);
				}
				else {
					SetTextColor(hdc, g_crBlack);
					SetBkColor(hdc, g_myColor[18]);
				}

				// 输出列标题 column captions
				UINT len = 0;
				len = static_cast<UINT>(m_Bar.name[i].length());
				ExtTextOutA(hdc, x, y, ETO_CLIPPED | ETO_OPAQUE, &col, m_Bar.name[i].c_str(), len, NULL);
			}
			i++;
		}
		if (bar.left < bar.right) {	// 未定义区域的上边界为白色
			SelectObject(hdc, g_hWhitePen);
			MoveToEx(hdc, bar.left, bar.top, NULL);
			LineTo(hdc, bar.right, bar.top);
			++bar.top;
			FillRect(hdc, &bar, g_hLightGrayBrush);
		}
	}
}

template<class T>
bool CTable<T>::Sortsorteddata(int sort) {

	if (m_Table.data.sorted && sort == m_Table.data.sort) {
		return false;
	}
	std::sort(m_Table.data.info.begin(), m_Table.data.info.end(), [=](const auto& i1, const auto& i2) {
		return CompareItems(i1, i2, sort);
		});
	m_Table.data.sort = sort;
	return true;
}

template<class T>
void CTable<T>::Deletesorteddata(ulong addr) {
	std::remove_if(m_Table.data.info.begin(), m_Table.data.info.end(), [=](auto& p) {
		return GetSeladdr(p) == addr;
		});
	m_Table.data.n--;
}

template<class T>
void CTable<T>::Addsorteddata(const T& t) {
	m_Table.data.info.push_back(t);
	m_Table.data.n++;
}


template<class T>
void CTable<T>::PaintTable(HWND hw) {
	HDC hdc, hdcMem;
	PAINTSTRUCT ps;
	RECT client, row, entry, txt;
	int column;
	int fontHeight;
	HBITMAP hBitmap;
	HGDIOBJ hgdiMem;
	T info;
	TEXTMETRIC tmFont;
	Tablefunction(hw, WM_USER_SCR, 0, 0);	// 重绘滚动条
	hdc = BeginPaint(hw, &ps);
	GetClientRect(hw, &client);

	if (m_Table.mode & TABLE_USERDEF || m_Bar.nbar > 0) {
		client.left = -m_Table.xshift;
		if (m_Table.showbar == 1) { // 显示
			column = -1;
			PaintBar(hdc, &client, column);
		}

		fontHeight = g_AvHighFont;
		if (m_bSnowFreeDrawing) {
			hdcMem = CreateCompatibleDC(hdc);
			if (hdcMem) {
				hBitmap = CreateCompatibleBitmap(hdc, client.right, fontHeight);
			}
			else {
				hBitmap = NULL;
			}
			if (hBitmap) {
				hgdiMem = SelectObject(hdcMem, hBitmap);
			}
			else {
				hdcMem = hdc;
				m_bSnowFreeDrawing = false;
			}
		}
		else {
			hdcMem = hdc;
		}
		SelectObject(hdcMem, g_hAppFont);
		GetTextMetrics(hdcMem, &tmFont);
		g_AvHighFont = tmFont.tmHeight;
		SetTextAlign(hdcMem, TA_LEFT | TA_BOTTOM);
		SetBkMode(hdcMem, TRANSPARENT);
		if (m_Table.mode & TABLE_USERDEF) {
			m_Table.data.n = (client.bottom - client.top - 1) / fontHeight + 1;
			m_Table.offset = 0;							// 设置显示首行
		}
		else {
			if (m_Table.data.n <= m_Table.offset) {
				m_Table.offset = m_Table.data.n - 1;	// 显示最后一行
			}
			if (m_Table.offset < 0) {
				m_Table.offset = 0;
			}
		}

		memcpy(&row, &client, sizeof(row));
		for (size_t i = m_Table.offset; m_Table.mode & TABLE_USERDEF || i < m_Table.data.n; ++i) {	// 循环次数为表行数
			if (m_Table.mode & TABLE_DIR) {
				if (client.top >= client.bottom)
					break;
				row.top = row.bottom - fontHeight;
				if (client.top > row.bottom - fontHeight)
					row.top = client.top;
			}
			else {
				if (row.top > client.bottom)
					break;
				row.bottom = fontHeight + row.top;
			}
			memcpy(&entry, &row, sizeof(entry));
			if (m_bSnowFreeDrawing) {
				entry.top = 0;
				entry.bottom = fontHeight;
			}
			if (m_Table.mode & TABLE_USERDEF) {
				m_Table.offset = i;
				info = m_Table.data.info[i];
			}
			else {
				info = m_Table.data.info[i];
			}
			int scheme = m_Table.scheme;
			int iTxColor=0, iBkColor;
			int col;
			int index = -1;
			int imageWidth = 18;
			for (col = 0; col < m_Bar.nbar;) {	// 循环次数为表列数
				int width = m_Bar.dx[col];
				if (entry.left > row.right)
					break;
				entry.right = entry.left + width - 1;
				if (col == 0&&m_Images!=NULL) {
					index = GetRowImage(hw,i);
					if (index != -1) {
						POINT pt;
						pt.x = entry.left+5;
						pt.y = entry.top;
						FillRect(hdcMem, &entry, g_myBrush[Custom1]);
						m_Images.Draw(hdcMem, index,pt, ILD_NORMAL);
						entry.left += imageWidth+8;
					}
				}
				if (width > 3) {
					if (entry.right >= row.left) {
						CString s;
						char mask = 0;
						int select = 0;
						int symName = 0;
						int len = ParseTableEntry(s, mask, select, info, col);
						select |= DRAW_UNICODE;
						HBRUSH hBkBrush;
						if (!(m_Table.mode & TABLE_USERDEF)	// 非自绘
							&& i==m_Table.data.selected
							&& !(select & DRAW_BREAK)	// 不是断点
							&& (!(m_Table.mode & TABLE_COLSEL) || m_Table.colsel == col)) {	// 不是在改变表项宽度
							select |= DRAW_SELECT;
						}
						int bottom;
						if (select & DRAW_DIRECT) {
							int drawMask = DRAW_NORMAL;
							memcpy(&txt, &entry, sizeof(txt));
							if (m_Bar.mode[col] & BAR_SHIFTSEL) {
								txt.right = txt.left;
							}
							else {
								txt.right = txt.left + g_AvWidthFont / 2;
							}
							int j = 0;
							int k = 0;
							while (j < len && txt.left < entry.right) {
								if (k >= len || drawMask != mask || txt.right > entry.right) {
									if (k != len || drawMask & DRAW_DIRECT || drawMask & (DRAW_UL | DRAW_SELECT)
										&& !(select & DRAW_EXTSEL)) {
										if (entry.right < txt.right) {
											txt.right = entry.right;
										}
									}
									else {
										txt.right = entry.right;
									}
									if (txt.right > txt.left) {
										if (drawMask & DRAW_DIRECT) {
											iTxColor = drawMask & 0xf;
											if (iTxColor > 20) {
												iTxColor = g_myScheme[m_Table.scheme].textcolor;
											}
											iBkColor = m_BkColorIndex[((drawMask) >> 4) & 7];
											if (iBkColor < 0) {
												if (select & DRAW_EIP) {
													iBkColor = g_myScheme[m_Table.scheme].textcolor;
												}
												else if (select & DRAW_BREAK) {
													iBkColor = g_myScheme[m_Table.scheme].hitextcolor;
												}
												else if (select & DRAW_SELECT) {
													iBkColor = g_myScheme[m_Table.scheme].selbkcolor;
												}
												else {
													iBkColor = g_myScheme[m_Table.scheme].bkcolor;
												}
											}
										}
										else {
											if (drawMask & DRAW_HILITE) {
												iTxColor = g_myScheme[m_Table.scheme].hitextcolor;
											}
											else if ((drawMask | select) & DRAW_GRAY) {
												iTxColor = g_myScheme[m_Table.scheme].lowcolor;
											}
											if ((drawMask | select) & DRAW_EIP) {
												if (!(drawMask & DRAW_HILITE)) {
													iTxColor = g_myScheme[m_Table.scheme].bkcolor;
												}
												iBkColor = g_myScheme[m_Table.scheme].textcolor;
											}
											else if ((drawMask | select) & DRAW_BREAK) {
												iTxColor = g_myScheme[m_Table.scheme].textcolor;
												iBkColor = g_myScheme[m_Table.scheme].hitextcolor;
											}
											else if ((drawMask ^ select) & DRAW_SELECT) {
												iBkColor = g_myScheme[m_Table.scheme].selbkcolor;
											}
											else {
												iBkColor = g_myScheme[m_Table.scheme].bkcolor;
											}
										}
										SetTextColor(hdcMem, g_myColor[iTxColor]);
										SetBkColor(hdcMem, g_myColor[iBkColor]);
										bottom = entry.bottom;
										if (select & DRAW_TOP) {
											txt.bottom -= fontHeight / 2;
											if (select & DRAW_SELECT) {
												hBkBrush = g_myBrush[g_myScheme[m_Table.scheme].selbkcolor];
											}
											else {
												hBkBrush = g_myBrush[g_myScheme[m_Table.scheme].bkcolor];
											}
											FillRect(hdcMem, &txt, hBkBrush);
											txt.top += fontHeight / 2;
											txt.bottom += fontHeight / 2;
											bottom += fontHeight / 2;
										}
										else if (select & DRAW_BOTTOM) {
											txt.bottom -= fontHeight / 2;
											bottom -= fontHeight - fontHeight / 2;
										}
										if (select & DRAW_UNICODE) {
											ExtTextOutW(hdcMem, entry.left + g_AvWidthFont / 2, bottom, ETO_CLIPPED | ETO_OPAQUE,
												&txt, s.GetString(), len, NULL);
										}
										
										if (drawMask & DRAW_UL) {
											SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].lowcolor]);
											MoveToEx(hdcMem, txt.left, bottom - 1, NULL);
											LineTo(hdcMem, txt.right, bottom - 1);
										}
										if (select & DRAW_TOP) {
											txt.top -= fontHeight / 2;
										}
										else if (select & DRAW_BOTTOM) {
											txt.bottom += fontHeight / 2;
											txt.top += fontHeight;
											if (select & DRAW_SELECT) {
												hBkBrush = g_myBrush[g_myScheme[m_Table.scheme].selbkcolor];
											}
											else {
												hBkBrush = g_myBrush[g_myScheme[m_Table.scheme].bkcolor];
											}
											FillRect(hdcMem, &txt, hBkBrush);
											txt.top -= fontHeight / 2;
										}
										switch (symName) {
											case D_SEP:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].auxcolor]);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 1, txt.top, NULL);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.bottom);
												break;

											case D_POINT:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, txt.left, txt.top + fontHeight / 2, NULL);
												LineTo(hdcMem, txt.left, txt.top + fontHeight / 2 + 2);
												MoveToEx(hdcMem, txt.left + 1, txt.top + fontHeight / 2, 0);
												LineTo(hdcMem, txt.left + 1, txt.top + fontHeight / 2 + 2);
												break;

											case D_JMPOUT:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, txt.left, (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, txt.left + 2 * g_AvWidthFont / 3 + 1, (txt.bottom + txt.top) / 2);
												break;

											case D_BEGIN:
											case D_SWTOP:
												if (symName == D_BEGIN) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].condbkcolor]);
												}
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 2, txt.top + fontHeight / 2 - 1, 0);
												LineTo(hdcMem, txt.left, txt.top + fontHeight / 2 - 1);
												LineTo(hdcMem, txt.left, txt.bottom);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 2, txt.top + fontHeight / 2 - 2, 0);
												LineTo(hdcMem, txt.left + 1, txt.top + fontHeight / 2 - 2);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												break;

											case D_JMPDN:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, txt.left, txt.bottom - g_AvWidthFont / 3 - 3, 0);
												LineTo(hdcMem, txt.left + g_AvWidthFont / 3, txt.bottom - 3);
												LineTo(hdcMem, txt.left + 2 * (g_AvWidthFont / 3) + 1, txt.bottom - g_AvWidthFont / 3 - 4);
												break;

											case D_END:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, txt.left, txt.top, 0);
												LineTo(hdcMem, txt.left, txt.top + fontHeight / 2 + 1);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.top + fontHeight / 2 + 1);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.top + fontHeight / 2 + 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.top + fontHeight / 2 + 2);
												break;

											case D_BODY:
											case D_SWBODY:
												if (symName == D_BODY) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].condbkcolor]);
												}
												MoveToEx(hdcMem, txt.left, txt.top, 0);
												LineTo(hdcMem, txt.left, txt.bottom);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												break;

											case D_ENTRY:
											case D_CASE:
												if (symName == D_ENTRY) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].condbkcolor]);
												}
												MoveToEx(hdcMem, txt.left, txt.top, 0);
												LineTo(hdcMem, txt.left, txt.bottom);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												MoveToEx(hdcMem, txt.left + 2, txt.top + fontHeight / 2, 0);
												LineTo(hdcMem, txt.left + 5, txt.top + fontHeight / 2);
												MoveToEx(hdcMem, txt.left + 2, txt.top + fontHeight / 2 - 2, 0);
												LineTo(hdcMem, txt.left + 2, txt.top + fontHeight / 2 + 3);
												MoveToEx(hdcMem, txt.left + 3, txt.top + fontHeight / 2 - 1, 0);
												LineTo(hdcMem, txt.left + 3, txt.top + fontHeight / 2 + 2);
												break;

											case D_LEAF:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, txt.left, txt.top, 0);
												LineTo(hdcMem, txt.left, txt.bottom);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												MoveToEx(hdcMem, txt.left + 1, txt.top + fontHeight / 2 + 1, 0);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.top + fontHeight / 2 + 1);
												MoveToEx(hdcMem, txt.left + 1, txt.top + fontHeight / 2 + 2, 0);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.top + fontHeight / 2 + 2);
												break;

											case D_SINGLE:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 2, txt.top + 1, 0);
												LineTo(hdcMem, txt.left + 1, txt.top + 1);
												LineTo(hdcMem, txt.left + 1, txt.bottom - 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.bottom - 2);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 2, txt.top + 2, 0);
												LineTo(hdcMem, txt.left, txt.top + 2);
												LineTo(hdcMem, txt.left, txt.bottom - 3);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.bottom - 3);
												break;

											case D_ENDBEG:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 2, txt.bottom - 3, 0);
												LineTo(hdcMem, txt.left, txt.bottom - 3);
												LineTo(hdcMem, txt.left, txt.bottom);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 2, txt.bottom - 4, 0);
												LineTo(hdcMem, txt.left + 1, txt.bottom - 4);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												MoveToEx(hdcMem, txt.left, txt.top, 0);
												LineTo(hdcMem, txt.left, txt.top + 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.top + 2);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.top + 3);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, txt.top + 3);
												break;

											case D_JMPUP:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].textcolor]);
												MoveToEx(hdcMem, txt.left, txt.top + g_AvWidthFont / 3 + 2, 0);
												LineTo(hdcMem, txt.left + g_AvWidthFont / 3, txt.top + 2);
												LineTo(hdcMem, txt.left + 2 * (g_AvWidthFont / 3) + 1, txt.top + g_AvWidthFont / 3 + 3);
												break;

											case 'a':
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												MoveToEx(hdcMem, txt.left + 2 * (g_AvWidthFont / 3), (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, txt.left + 3, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 - 2);
												LineTo(hdcMem, txt.left + 1, txt.top - 1);
												MoveToEx(hdcMem, txt.left + 1, txt.bottom - 1, 0);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 - 1);
												break;

											case D_PATHDN:
											case D_GRAYDN:
												if (symName == D_PATHDN) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].lowcolor]);
												}
												MoveToEx(hdcMem, txt.left + 2 * (g_AvWidthFont / 3), (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, txt.left + 3, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 + 2);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												break;

											case D_PATHDNEND:
											case D_GRAYDNEND:
												if (symName == D_PATHDNEND) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].lowcolor]);
												}
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 - 1);
												LineTo(hdcMem, txt.left + 2, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 4, (txt.bottom + txt.top) / 2 - 3);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 1, (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 4, (txt.bottom + txt.top) / 2 + 3);
												break;

											case 'h':
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												MoveToEx(hdcMem, txt.left + 2 * (g_AvWidthFont / 3), (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, txt.left + 3, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 + 2);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 + 1);
												break;

											case D_PATH:
											case D_GRAYPATH:
												if (symName == D_PATH) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].lowcolor]);
												}
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.bottom);
												break;

											case D_LASTCASE:
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].condbkcolor]);
												MoveToEx(hdcMem, txt.left, txt.top, 0);
												LineTo(hdcMem, txt.left, txt.top + fontHeight / 2 + 1);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, txt.top + fontHeight / 2 + 2);
												MoveToEx(hdcMem, txt.left + 2, txt.top + fontHeight / 2, 0);
												LineTo(hdcMem, txt.left + 5, txt.top + fontHeight / 2);
												MoveToEx(hdcMem, txt.left + 2, txt.top + fontHeight / 2 - 2, 0);
												LineTo(hdcMem, txt.left + 2, txt.top + fontHeight / 2 + 3);
												MoveToEx(hdcMem, txt.left + 3, txt.top + fontHeight / 2 - 1, 0);
												LineTo(hdcMem, txt.left + 3, txt.top + fontHeight / 2 + 2);
												break;

											case D_PATHUPEND:
											case D_GRAYUPEND:
												if (symName == D_PATHUPEND) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].lowcolor]);
												}
												MoveToEx(hdcMem, txt.left + 2 * (g_AvWidthFont / 3), (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, txt.left + 3, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 - 2);
												LineTo(hdcMem, txt.left + 1, txt.top - 1);
												break;

											case D_PATHUP:
											case D_GRAYUP:
												if (symName == D_PATHUP) {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												}
												else {
													SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].lowcolor]);
												}
												MoveToEx(hdcMem, txt.left + 2 * (g_AvWidthFont / 3), (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, txt.left + 3, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 - 2);
												LineTo(hdcMem, txt.left + 1, txt.top - 1);
												break;

											case 'z':
												SelectObject(hdcMem, g_myPen[g_myScheme[m_Table.scheme].hitextcolor]);
												MoveToEx(hdcMem, txt.left + 1, txt.top, 0);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 - 1);
												LineTo(hdcMem, txt.left + 2, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 1, (txt.bottom + txt.top) / 2);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 4, (txt.bottom + txt.top) / 2 - 3);
												MoveToEx(hdcMem, txt.left + 1, txt.bottom - 1, 0);
												LineTo(hdcMem, txt.left + 1, (txt.bottom + txt.top) / 2 + 1);
												LineTo(hdcMem, txt.left + 2, (txt.bottom + txt.top) / 2);
												MoveToEx(hdcMem, g_AvWidthFont + txt.left - 1, (txt.bottom + txt.top) / 2, 0);
												LineTo(hdcMem, g_AvWidthFont + txt.left - 4, (txt.bottom + txt.top) / 2 + 3);
												break;

											default:
												break;
										}
									}
									symName = 0;
									j = k;
									txt.left = txt.right;	// 下一个文本位置
								}
								if (txt.left >= entry.right)
									break;
								if (mask & DRAW_GRAPH) {
									if (select & DRAW_UNICODE) {
										symName = s[j];
									}
								}
								drawMask = mask;
								if (!(drawMask & DRAW_MASK)) {
									drawMask &= ~DRAW_GRAPH;
								}
								k++;
								txt.right += g_AvWidthFont;
							}
							if (entry.right > txt.left) {
								txt.right = entry.right;
								if (select & DRAW_SELECT) {
									iBkColor = g_myScheme[m_Table.scheme].selbkcolor;
								}
								else {
									iBkColor = g_myScheme[m_Table.scheme].bkcolor;
								}
								FillRect(hdcMem, &txt, g_myBrush[iBkColor]);
							}
						}
						else {
							int opt;
							if (select & DRAW_GRAY) {
								iTxColor = g_myScheme[m_Table.scheme].lowcolor;
							}
							else if (select & DRAW_HILITE) {
								iTxColor = g_myScheme[m_Table.scheme].hitextcolor;
							}
							else {
								iTxColor = g_myScheme[m_Table.scheme].textcolor;
							}
							if (select & DRAW_EIP) {
								if (select & DRAW_BREAK) {
									iTxColor = g_myScheme[m_Table.scheme].hitextcolor;
								}
								else {
									iTxColor = g_myScheme[m_Table.scheme].bkcolor;
								}
								iBkColor = g_myScheme[m_Table.scheme].textcolor;
							}
							else if (select & DRAW_BREAK) {
								iTxColor = g_myScheme[m_Table.scheme].textcolor;
								iBkColor = g_myScheme[m_Table.scheme].hitextcolor;
							}
							else if (select & DRAW_SELECT) {
								iBkColor = g_myScheme[m_Table.scheme].selbkcolor;
							}
							else {
								iBkColor = g_myScheme[m_Table.scheme].bkcolor;
							}
							SetTextColor(hdcMem, g_myColor[iTxColor]);
							SetBkColor(hdcMem, g_myColor[iBkColor]);
							bottom = entry.bottom;
							if (select & DRAW_TOP) {
								entry.bottom -= fontHeight / 2;
								FillRect(hdcMem, &entry, g_myBrush[g_myScheme[m_Table.scheme].bkcolor]);
								entry.top += fontHeight;
								entry.bottom += fontHeight / 2;
								bottom += fontHeight / 2;
							}
							else if (select & DRAW_BOTTOM) {
								entry.bottom -= fontHeight - fontHeight / 2;
								bottom += fontHeight / 2;
							}
							if ((select & (DRAW_BREAK | DRAW_SELECT)) == (DRAW_BREAK | DRAW_SELECT)) {
								FillRect(hdcMem, &entry, g_myBrush[g_myScheme[m_Table.scheme].condbkcolor]);
								opt = ETO_CLIPPED;
							}
							else {
								opt = ETO_CLIPPED | ETO_OPAQUE;
							}
							if (select & DRAW_UNICODE) {
								ExtTextOut(hdcMem, entry.left + g_AvWidthFont / 2, bottom, opt, &entry, s.GetString(), len, NULL);
							}
							if (select & DRAW_TOP) {
								entry.top -= fontHeight / 2;
							}
							else if (select & DRAW_BOTTOM) {
								entry.bottom += fontHeight - fontHeight / 2;
								entry.top += fontHeight - fontHeight / 2;
								FillRect(hdcMem, &entry, g_myBrush[1]);
								entry.top -= fontHeight - fontHeight / 2;
							}
						}
					}
				}
				else {
					FillRect(hdcMem, &entry, g_myBrush[g_myScheme[scheme].auxcolor]);
				}
				SelectObject(hdcMem, g_myPen[g_myScheme[scheme].linecolor]);
				MoveToEx(hdcMem, entry.right, entry.top, 0);
				LineTo(hdcMem, entry.right, entry.bottom);	// 短的分割线
				entry.left = entry.right + 1;	// 下一个表项位置
				col++;	// 下一个entry
			}
			if (row.right > entry.left) {
				entry.right = row.right;
				FillRect(hdcMem, &entry, g_myBrush[g_myScheme[m_Table.scheme].bkcolor]);
			}
			int height;
			if (m_Table.mode & TABLE_DIR) {
				if (m_bSnowFreeDrawing) {
					height = row.bottom - row.top;
					BitBlt(hdc, row.left, row.top, row.right - row.left, row.bottom - row.top, hdcMem, row.left,
						fontHeight - (row.bottom - row.top),
						SRCCOPY);
				}
				row.bottom = row.top;
			}
			else {
				if (m_bSnowFreeDrawing) {
					BitBlt(hdc, row.left, row.top, row.right - row.left, fontHeight, hdcMem, row.left, 0, SRCCOPY);
				}
				row.top = row.bottom;
			}
		}

		if (!(m_Table.mode & TABLE_DIR) && row.top<client.bottom || m_Table.mode & TABLE_DIR
			&& row.bottom>client.top) {
			SelectObject(hdc, g_myPen[g_myScheme[m_Table.scheme].linecolor]);
			if (m_Table.mode & TABLE_DIR) {
				row.top = client.top;
			}
			else {
				row.bottom = client.bottom;
			}

			for (int m = 0; m < m_Bar.nbar; m++) {
				int dx = m_Bar.dx[m];
				row.right = row.left + dx - 1;
				if (dx > 3) {
					FillRect(hdc, &row, g_myBrush[g_myScheme[m_Table.scheme].bkcolor]);
				}
				else {
					FillRect(hdc, &row, g_myBrush[g_myScheme[m_Table.scheme].auxcolor]);
				}
				MoveToEx(hdc, row.right, row.top, NULL);
				LineTo(hdc, row.right, row.bottom);
				row.left = row.right + 1;
			}
			if (client.right > row.left) { // 处理表格外的
				row.right = client.right;
				FillRect(hdc, &row, g_myBrush[g_myScheme[m_Table.scheme].bkcolor]);
			}
		}
		if (m_bSnowFreeDrawing) {
			SelectObject(hdcMem, hgdiMem);
			DeleteObject(hBitmap);
			DeleteDC(hdcMem);
		}
		EndPaint(hw, &ps);
	}
	else {
		FillRect(hdc, &client, g_myBrush[g_myScheme[m_Table.scheme].bkcolor]);
		EndPaint(hw, &ps);
	}
}

template<class T>
void CTable<T>::SetTableWindowInfo(int maxColumns,int nlines) {
	ATLASSERT(nlines > 5 && maxColumns > 2);

	m_Rect.right = GetSystemMetrics(SM_CXVSCROLL) - 1;
	int j = 0;
	while (m_Bar.nbar > j && j < maxColumns) {
		if (m_Bar.nbar - 1 != j || m_Bar.defdx[j] != 256) {
			m_Rect.right += m_Bar.defdx[j];
		}
		else {
			m_Rect.right += 50 * g_AvWidthFont;
		}
		++j;
	}
	m_Rect.bottom = nlines * g_AvHighFont;
	if (m_Table.hscroll == 1) {
		m_Rect.bottom += GetSystemMetrics(SM_CYHSCROLL);
		m_Rect.bottom += g_AvHighFont + 4;
	}

	AdjustWindowRect(&m_Rect, WS_CHILD | WS_GROUP | WS_TABSTOP | WS_SYSMENU | WS_THICKFRAME | WS_CAPTION, false);
	m_Rect.right -= m_Rect.left;
	m_Rect.bottom -= m_Rect.top;
	/*RECT client;
	GetClientRect(g_hWndMDIClient, &client);*/

	_style = WS_CHILD | WS_GROUP | WS_TABSTOP | WS_CLIPSIBLINGS
		| WS_VISIBLE | WS_VSCROLL | WS_SYSMENU | WS_THICKFRAME | WS_CAPTION;
	m_Rect.top = CW_USEDEFAULT;
	m_Rect.left = CW_USEDEFAULT;

	if (!(m_Table.mode & TABLE_NOHSCR)) {
		_style = WS_CHILD | WS_GROUP | WS_TABSTOP | WS_CLIPSIBLINGS
			| WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_SYSMENU | WS_THICKFRAME | WS_CAPTION;
	}
	//if (client.right < m_Rect.right) {
	//	m_Rect.right = client.right;
	//}
	//if (client.bottom < m_Rect.bottom) {
	//	m_Rect.bottom = client.bottom;
	//}
}

template<class T>
int CTable<T>::Tablefunction(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
	RECT client, bar;
	int bottomRow;
	int xPos=0, yPos = 0;
	WPARAM col = -1;
	int newdx;
	bool pressShiftKey, pressCtrlKey;
	int ret = 0;
	int selRow;

	GetClientRect(hw, &client);
	memcpy(&bar, &client, sizeof(bar));
	if (m_Table.showbar == 1) {
		client.top = g_AvHighFont + 4;
		bar.bottom = g_AvHighFont + 4;
	}
	else {
		bar.bottom = bar.top;
	}
	int rows = (client.bottom - client.top) / g_AvHighFont;
	if (rows <= 0) {
		rows = 1;
	}
	if (m_Table.mode & TABLE_DIR) {
		bottomRow = -1;
	}
	else {
		bottomRow = 1;
	}
	bool onBar = false;
	int xshift;
	int iHScrollPos, iVScrollPos;
	int lines;
	int iSplitLine = -1, left=0, x;

	if (msg == WM_MOUSEMOVE || WM_LBUTTONDOWN == msg || msg == WM_LBUTTONDBLCLK || msg == WM_LBUTTONUP
		|| msg == WM_RBUTTONDOWN || (msg == WM_TIMER && wp == 1)) {
		if (msg == WM_TIMER) {
			xPos = m_Bar.prevx;
			yPos = -1;
		}
		else {
			xPos = GET_X_LPARAM(lp);
			yPos = GET_Y_LPARAM(lp);
		}
		onBar = yPos >= bar.top && yPos < bar.bottom;
		if (!onBar&&msg==WM_RBUTTONDOWN) {
			Tablefunction(hw, WM_LBUTTONDOWN, wp, lp);
		}
		if (bottomRow < 0) {
			yPos = client.bottom + client.top - yPos - 1;
		}
		int j = 0;
		xshift = -m_Table.xshift - 1;
		while (j < m_Bar.nbar && j < NBAR) {
			x = xshift + m_Bar.dx[j];
			if (abs(xPos - x) <= 1 && !(m_Bar.mode[j] & BAR_NORESIZE)) {
				iSplitLine = j;
			}
			if (xPos >= xshift && x > xPos) {
				col = j;
				left = xPos - xshift - 1;
				if (!(m_Bar.mode[j] & BAR_SHIFTSEL)) {
					left -= g_AvWidthFont / 2;
				}
				if (left < 0) {
					left = 0;
				}
			}
			xshift = x;
			j++;
		}
	}
	switch (msg) {
		case WM_RBUTTONDOWN:
		{
			return onBar ? false : true;
		}
		case WM_HSCROLL:
		{
			newdx = -client.right;
			int i = 0;
			while (i < m_Bar.nbar) {
				newdx += m_Bar.dx[i];
				++i;
			}
			iHScrollPos = m_Table.xshift;
			switch (LOWORD(wp)) {
				case SB_LINELEFT:
					iHScrollPos -= g_AvWidthFont;
					break;
				case SB_LINERIGHT:
					iHScrollPos += g_AvWidthFont;
					break;
				case SB_PAGELEFT:
					iHScrollPos -= 8 * g_AvWidthFont;
					break;
				case SB_LEFT:
					iHScrollPos = 0;
					break;
				case SB_RIGHT:
					iHScrollPos = newdx;
					break;
				case SB_THUMBTRACK:
					iHScrollPos = MulDiv(HIWORD(wp), newdx, 0x4000);
					break;
				default:
					break;
			}
			if (iHScrollPos > newdx) {
				iHScrollPos = newdx;
			}
			if (iHScrollPos < 0) {
				iHScrollPos = 0;
			}
			if (iHScrollPos != m_Table.xshift) {
				m_Table.xshift = iHScrollPos;
				InvalidateRect(hw, nullptr, false);
			}
			return 0;
		}
		case WM_USER_SCR:
		{
			LONG style = GetWindowLong(hw, GWL_STYLE);
			if (style & WS_VSCROLL) {
				SetScrollRange(hw, SB_VERT, 0, 0x4000, false);
				ret = static_cast<int>(SendMessage(hw, WM_USER_VABS, rows, 0));
				if (ret < 0)
					ret = 0x4000 - ret;
				if (m_Table.offset != 0&& ret == 0) {
					ret = MulDiv(static_cast<int>(m_Table.offset+rows), 0x4000,
						static_cast<int>(m_Table.data.n));
				}
				if (ret != GetScrollPos(hw, SB_VERT)) {
					SetScrollPos(hw, SB_VERT, ret, true);
				}
			}
			if (style & WS_HSCROLL) {
				SetScrollRange(hw, SB_HORZ, 0, 0x4000, false);
				newdx = -client.right;
				int i = 0;
				while (i < m_Bar.nbar) {
					newdx += m_Bar.dx[i];
					++i;
				}
				int nPos;
				nPos = MulDiv(m_Table.xshift, 0x4000, newdx);
				SetScrollPos(hw, SB_HORZ, nPos, true);
			}
			return 0;
		}
		case WM_DESTROY:
		{
			if (hw == g_hWndTop)
				g_hWndTop = 0;
			if (m_Table.mode & TABLE_SAVEPOS) {

			}
			return 0;
		}
		case WM_USER_VABS:
		{
			selRow = static_cast<int>(lp + m_Table.offset);
			if (selRow > (signed int)(m_Table.data.n - wp)) {
				selRow = static_cast<int>(m_Table.data.n - wp);
			}
			if (selRow < 0) {
				selRow = 0;
			}
			m_Table.offset = selRow;
			if (selRow) {
				return MulDiv(selRow, 0x4000, static_cast<int>(m_Table.data.n - wp));
			}
			else {
				return 0;
			}
		}
		case WM_USER_VREL:
		{
			if (m_Table.data.n > wp) {
				selRow = MulDiv(static_cast<int>(m_Table.data.n - wp), 
					static_cast<int>(lp), 0x4000);
			}
			else {
				selRow = 0;
			}
			if (selRow > (signed int)(m_Table.data.n - wp)) {
				selRow = static_cast<int>(m_Table.data.n - wp);
			}
			if (selRow < 0) {
				selRow = 0;
			}
			if (lp && selRow == m_Table.offset) {
				return -1;
			}
			else {
				m_Table.offset = selRow;
				if (selRow) {
					return MulDiv(selRow, 0x4000, static_cast<int>(m_Table.data.n - wp));
				}
				else {
					return 0;
				}
			}
		}
		case WM_VSCROLL:
		{
			pressShiftKey = GetKeyState(VK_SHIFT) & 0x8000;
			switch (LOWORD(wp)) {
				case SB_LINEDOWN:
					if (pressShiftKey && m_Table.mode & TABLE_USERDEF) {
						ret = static_cast<int>(SendMessage(hw, WM_USER_VBYTE, rows, bottomRow));
					}
					else {
						ret = static_cast<int>(SendMessage(hw, WM_USER_VABS, rows, bottomRow));
					}
					break;
				case SB_LINEUP:
					if (pressShiftKey && m_Table.mode & TABLE_USERDEF) {
						ret = static_cast<int>(SendMessage(hw, WM_USER_VBYTE, rows, -bottomRow));
					}
					else {
						ret = static_cast<int>(SendMessage(hw, WM_USER_VABS, rows, -bottomRow));
					}
					break;
				case SB_PAGEUP:
					lines = rows - 1;
					if (lines <= 0)
						lines = 1;
					ret = static_cast<int>(SendMessage(hw, WM_USER_VABS, rows, bottomRow * -lines));
					break;
				case SB_PAGEDOWN:
					lines = rows - 1;
					if (lines <= 0)
						lines = 1;
					ret = static_cast<int>(SendMessage(hw, WM_USER_VABS, rows, bottomRow * lines));
					break;
				case SB_THUMBTRACK:
					iVScrollPos = HIWORD(wp);
					if (bottomRow < 0)
						iVScrollPos = 0x3ff - iVScrollPos;
					ret = static_cast<int>(::SendMessage(hw, WM_USER_VREL, rows, iVScrollPos));
					break;
				default:
					ret = -1;
					break;
			}
			if (ret >= 0) {
				if (m_Table.mode & TABLE_FASTSEL) {
					RedrawWindow(hw, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				}
				else {
					InvalidateRect(hw, NULL, false);
				}
			}
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			HCURSOR hCursor;
			if (m_Bar.captured && GetCapture() == hw) {
				switch (m_Bar.captured) {
					case CAPT_SPLIT:
						hCursor = LoadCursor(NULL, IDC_SIZEWE);
						SetCursor(hCursor);
						newdx = xPos + m_Bar.dx[m_Bar.active] - m_Bar.prevx;
						if (newdx < 3) {
							int distance = 3 - newdx;
							newdx = 3;
							xPos += distance;
						}
						if (m_Bar.dx[m_Bar.active] != newdx) {
							m_Bar.dx[m_Bar.active] = newdx;
							InvalidateRect(hw, nullptr, false);
						}
						break;
					case CAPT_BAR:
						hCursor = LoadCursor(NULL, IDC_ARROW);
						SetCursor(hCursor);
						if (!onBar, m_Bar.active != col || m_Bar.mode[m_Bar.active] & BAR_PRESSED) {
							m_Bar.mode[m_Bar.active] &= ~BAR_PRESSED;
							InvalidateRect(hw, &bar, false);
						}
						else {
							m_Bar.mode[m_Bar.active] |= BAR_PRESSED;
							InvalidateRect(hw, &bar, false);
						}
						break;
					case CAPT_DATA:
						hCursor = LoadCursor(NULL, IDC_ARROW);
						SetCursor(hCursor);
						if (yPos < client.top || yPos >= client.bottom) {
							if (yPos >= client.bottom) {
								if (m_Bar.active <= 0) {
									SetTimer(hw, 1, 50, NULL);
								}
								m_Bar.active = 1;
								if (client.bottom + 48 < yPos) {
									m_Bar.active += (yPos - client.bottom - 48) / 16;
								}
							}
							else {
								if (m_Bar.active >= 0) {
									SetTimer(hw, 1, 50, NULL);
								}
								m_Bar.active = -1;
								if (client.top - 48 > yPos) {
									m_Bar.active -= (client.top - 48 - yPos) / 16;
								}
							}
						}
						else {
							KillTimer(hw, 1);
							m_Bar.active = 0;
							if (SendMessage(hw, WM_USER_CNTS, col << 16 | LOWORD(left / g_AvWidthFont),
								(yPos - client.top) / g_AvHighFont)) {
								if (m_Table.mode & TABLE_FASTSEL) {
									RedrawWindow(hw, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
								}
								else {
									InvalidateRect(hw, NULL, false);
								}
							}
						}
						break;
				}
				m_Bar.prevx = xPos;
			}
			else if (iSplitLine < 0) {
				hCursor = LoadCursor(NULL, IDC_ARROW);
				SetCursor(hCursor);
			}
			else {
				hCursor = LoadCursor(NULL, IDC_SIZEWE);
				SetCursor(hCursor);
			}
			return 0;
		}
		case WM_LBUTTONDBLCLK: {
			bool handle = onBar || iSplitLine >= 0 ||
				SendMessage(hw, WM_USER_DBLCLK, ((uint16_t)col << 16) | (uint16_t)(left / g_AvWidthFont), (yPos - client.top) / g_AvHighFont) != 1;
			if (!handle)
				break;
		}
		case WM_LBUTTONDOWN:
		{
			SetFocus(hw);
			if (iSplitLine < 0) {
				if (!onBar || (col & 0x80000000) != 0 || m_Bar.mode[col] & (BAR_NOSORT | BAR_DISABLED)) {
					if (!onBar || (col & 0x80000000) != 0) {
						pressShiftKey = GetKeyState(VK_SHIFT) & 0x800;
						UINT uMsg = WM_USER_CNTS;
						if (!pressShiftKey) {
							uMsg = WM_USER_STS;
						}
						int value = static_cast<int>(SendMessage(hw, uMsg, 
							col << 16 | LOWORD(left / g_AvWidthFont),
							(yPos - client.top) / g_AvHighFont));
						if (value > 0) {
							if (m_Table.mode & TABLE_FASTSEL) {
								RedrawWindow(hw, nullptr, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
							}
							else {
								InvalidateRect(hw, nullptr, false);
							}
						}
						if (value >= 0) {
							SetCapture(hw);
							m_Bar.captured = CAPT_DATA;
							m_Bar.active = 0;
						}
					}
				}
				else {
					SetCapture(hw);
					m_Bar.captured = CAPT_BAR;
					m_Bar.mode[col] |= BAR_PRESSED;
					m_Bar.active = static_cast<int>(col);
					InvalidateRect(hw, &bar, false);
				}
			}
			else {
				SetCapture(hw);
				m_Bar.captured = CAPT_SPLIT;
				m_Bar.active = iSplitLine;
				m_Bar.prevx = xPos;
			}
			return 0;
		}
		case WM_LBUTTONUP:
		{
			if (GetCapture() == hw) {
				ReleaseCapture();
				if (m_Bar.captured == CAPT_BAR) {
					if (m_Bar.mode[m_Bar.active] & BAR_PRESSED) {
						m_Bar.mode[m_Bar.active] &= ~BAR_PRESSED;
						if (m_Bar.mode[m_Bar.active] & BAR_BUTTON) {
							InvalidateRect(hw, &bar, false);
							SendMessage(hw, WM_USER_BAR, m_Bar.active, 0);
						}
						else if (Sortsorteddata(m_Bar.active)) {
							InvalidateRect(hw, nullptr, false);// 重新排序了，则刷新整个窗口
						}
						else {
							InvalidateRect(hw, &bar, false);
						}
					}
				}
				else if (m_Bar.captured == CAPT_DATA) {
					KillTimer(hw, 1);
				}
				m_Bar.captured = CAPT_FREE;
			}
			return 0;
		}
		case WM_USER_CNTS:
		case WM_USER_STS:
		{
			if (m_Table.mode & TABLE_USERDEF) {
				return 0;
			}
			int selRow = static_cast<int>(lp + m_Table.offset);
			if (selRow >= m_Table.data.n) {
				selRow = static_cast<int>(m_Table.data.n - 1);
			}
			if (selRow == m_Table.data.selected && (!(m_Table.mode & TABLE_COLSEL)
				|| HIWORD(lp) == m_Table.colsel)) {
				return 0;
			}
			if (selRow > 0) {
				auto item = Getsortedbyselection(selRow);
				m_Table.data.seladdr = GetSeladdr(item);
			}
			m_Table.data.selected = selRow;
			if (m_Table.mode & TABLE_COLSEL) {
				m_Table.colsel = HIWORD(wp);
			}
			else {
				m_Table.colsel = 0;
			}
			return 1;
		}
		case WM_WINDOWPOSCHANGED:
		{
			if (*(HWND*)lp != g_hWndTop && g_hWndTop) {
				SetWindowPos(g_hWndTop, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOMOVE);
			}
			return static_cast<int>(DefMDIChildProc(g_hWndTop, WM_WINDOWPOSCHANGED, wp, lp));
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			GetSysKeyState(msg, lp, pressShiftKey, pressCtrlKey);
			int count = lp & 0x7FFF;
			if (count != 0) {
				if (count > 0x10) {
					count = 16;
				}
			}
			else {
				count = 1;
			}
			int shift;
			if (wp == VK_LEFT) {
				int colsel = m_Table.colsel;
				if (pressCtrlKey) {
					xshift = 0;
					for (int i = 0; i < m_Bar.nbar && xshift + m_Bar.dx[i] < m_Table.xshift; i++) {
						xshift += m_Bar.dx[i];
					}
				}
				else if (m_Table.mode & TABLE_COLSEL) {
					--colsel;
					xshift = m_Table.xshift;
					int width;
					do {
						width = m_Bar.dx[colsel];
						--colsel;
					} while (colsel >= 0 && width <= 3);

					if (colsel < 0) {
						colsel = m_Table.colsel;
					}
					else {
						xshift = 0;
						for (int i = 0; colsel > i; i++) {
							xshift += m_Bar.dx[i];
						}
						if (xshift > m_Table.xshift) {
							xshift = m_Table.xshift;
						}
					}
				}
				else {
					xshift = m_Table.xshift - count * g_AvWidthFont;
				}
				if (xshift < 0) {
					xshift = 0;
				}
				if (xshift != m_Table.xshift || colsel != m_Table.colsel) {
					m_Table.xshift = xshift;
					m_Table.colsel = colsel;
					InvalidateRect(hw, nullptr, NULL);
				}
				return 1;
			}
			else if (wp == VK_RIGHT) {
				int col = m_Table.colsel;
				int newdx = 0;
				if (pressCtrlKey) {
					shift = 0;
					for (int i = 0; i < m_Bar.nbar; i++) {
						if (m_Table.xshift >= newdx && i < m_Bar.nbar - 1) {
							shift += m_Bar.dx[i];
						}
						newdx += m_Bar.dx[i];
					}
				}
				else if (m_Table.mode & TABLE_COLSEL) {
					int tmp;
					do {
						tmp = m_Bar.dx[col];
						col++;
					} while (col < m_Bar.nbar && tmp <= 3);
					if (col >= m_Bar.nbar) {
						for (int i = 0; i < m_Bar.nbar; i++) {
							newdx += m_Bar.dx[i];
						}
						col = m_Table.colsel;
						shift = m_Table.xshift;
					}
					else {
						shift = 0;

						for (int i = 0; col > i; i++) {
							shift += m_Bar.dx[i];
						}
						newdx = shift + m_Bar.dx[col];
					}
				}
				else {
					for (int i = 0; i < m_Bar.nbar; i++) {
						newdx += m_Bar.dx[i];
					}
					shift = count * g_AvWidthFont + m_Table.xshift;
				}
				newdx -= client.right;
				if (shift > newdx) {
					shift = newdx;
				}
				if (shift < 0)
					shift = 0;
				if (shift != m_Table.xshift || col != m_Table.colsel) {
					m_Table.xshift = shift;
					m_Table.colsel = col;
					InvalidateRect(hw, nullptr, false);
				}
				return 1;
			}
			else {
				lines = count * bottomRow;
				switch (wp) {
					case VK_PRIOR:
					{
						int totalRow = rows - 1;
						if (totalRow <= 0) {
							totalRow = 1;
						}
						_page = lines * -totalRow;
						LPARAM lParam;
						if (pressCtrlKey) {
							lParam = 0x7FFFFFFF;
						}
						else {
							lParam = _page;
						}
						ret = static_cast<int>(SendMessage(hw, WM_USER_CHGS, rows, lParam));
						break;
					}
					case VK_NEXT:
					{
						int totalRow = rows - 1;
						if (totalRow <= 0) {
							totalRow = 1;
						}
						_page = lines * totalRow;
						LPARAM lParam;
						if (pressCtrlKey) {
							lParam = 0x7FFFFFFF;
						}
						else {
							lParam = _page;
						}
						ret = static_cast<int>(SendMessage(hw, WM_USER_CHGS, rows, lParam));
						break;
					}
					case VK_END:
					{
						ret = static_cast<int>(SendMessage(hw, WM_USER_CHGS, rows, 0x7FFFFFFE));
						break;
					}
					case VK_HOME:
					{
						ret = static_cast<int>(SendMessage(hw, WM_USER_CHGS, rows, 0x7FFFFFFF));
						break;
					}
					case VK_UP:
					{
						if (pressCtrlKey && m_Table.mode & TABLE_USERDEF) {
							ret = static_cast<int>(SendMessage(hw, WM_USER_VBYTE, rows, -lines));
						}
						else {
							ret = static_cast<int>(SendMessage(hw, WM_USER_CHGS, rows, -lines));
						}
						break;
					}
					case VK_DOWN:
					{
						if (pressCtrlKey && m_Table.mode & TABLE_USERDEF) {
							ret = static_cast<int>(SendMessage(hw, WM_USER_VBYTE, rows, lines));
						}
						else {
							ret = static_cast<int>(SendMessage(hw, WM_USER_CHGS, rows, lines));
						}
						break;
					}
					default:
						break;
				}
				if (ret >= 0) {
					if (m_Table.mode & TABLE_FASTSEL) {
						RedrawWindow(hw, nullptr, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
					}
					else {
						InvalidateRect(hw, nullptr, false);
					}
				}
				return 1;
			}
		}
		case WM_USER_CHGS:
		{
			if (lp == 0x7FFFFFFF) {
				if (bottomRow <= 0) {
					selRow = static_cast<int>(m_Table.data.n - 1);
				}
				else {
					selRow = 0;
				}
			}
			else if (lp == 0x7FFFFFFE) {
				if (bottomRow <= 0) {
					selRow = 0;
				}
				else {
					selRow = static_cast<int>(m_Table.data.n - 1);
				}
			}
			else {
				selRow = static_cast<int>(lp + m_Table.data.selected);
			}
			if (selRow >= m_Table.data.n) {
				selRow = static_cast<int>(m_Table.data.n - 1);
			}
			if (selRow < 0)
				selRow = 0;
			if (selRow == m_Table.data.selected)
				return -1;
			auto item = Getsortedbyselection(selRow);
			ulong addr = GetSeladdr(item);

			m_Table.data.selected = selRow;
			if (addr) {
				m_Table.data.seladdr = addr;
			}
			int offset = static_cast<int>(_page + m_Table.offset);
			if (selRow >= offset + wp) {
				offset = static_cast<int>(selRow - wp + 1);
			}
			if (selRow < offset)
				offset = selRow;
			if (offset > (signed int)(m_Table.data.n - wp)) {
				offset = static_cast<int>(m_Table.data.n - wp);
			}
			if (offset < 0)
				offset = 0;
			m_Table.offset = offset;
			if (offset) {
				return MulDiv(offset, 0x4000, static_cast<int>(m_Table.data.n - wp));
			}
			else {
				return 0;
			}
		}
	}
	return 1;
}


