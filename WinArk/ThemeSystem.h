#pragma once

struct t_scheme { // 颜色主题
	std::string	name;          // 主题名字
	int  textcolor;            // 文本颜色
	int  hitextcolor;          // 文本高亮颜色
	int  lowcolor;             // 辅助文本颜色
	int  bkcolor;              // 背景颜色
	int  selbkcolor;           // 选中时的背景颜色
	int  linecolor;            // 分割线的颜色
	int  auxcolor;             // 辅助对象的颜色
	int  condbkcolor;          // 条件断点的颜色
};



enum {
	Black,
	Crimson,
	DarkGreen,
	DarkYellow,
	DarkBlue,
	DeepMagenta,
	DarkCyan,	// 深青色
	LightGray,
	DarkGray,
	Red,
	Green,
	Yellow,
	Blue,
	Magenta,
	Cyan,	// 蓝绿色
	White,
	MoneyGreen,// 浅绿色
	HackerGreen,
	Custom1,
	Custom2,
	Custom3
};

void InitColorSys();
void InitFontSys();
void InitPenSys();
void InitBrushSys();
void InitSchemSys();
