#pragma once

// 遵循"最小化原则"，非分页内存比较昂贵，所以默认是分页内存
void* _cdecl operator new(size_t size, POOL_TYPE type, ULONG tag = 0);

// size 是编译器确定的
// placement new
void* _cdecl operator new(size_t size, void* p);

// https://stackoverflow.com/questions/10397826/overloading-operator-new-with-wdk
void _cdecl operator delete(void* p, size_t);

void _cdecl operator delete(void* p);

void* _cdecl operator new[](size_t size, POOL_TYPE type, ULONG tag);

void _cdecl operator delete[](void* p, size_t);
