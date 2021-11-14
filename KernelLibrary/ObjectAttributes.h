#pragma once

enum class ObjectAttributesFlags {
	None = 0,
	KernelHandle = OBJ_KERNEL_HANDLE,// 表示使用内核句柄，即句柄存在于系统句柄中
	Caseinsensive = OBJ_CASE_INSENSITIVE,// 表示内核对象的名字不区分大小写
	OpenIf = OBJ_OPENIF,
	ForeceAccessCheck = OBJ_FORCE_ACCESS_CHECK,
	Permanent = OBJ_PERMANENT,
	Exclusive = OBJ_EXCLUSIVE,
	Inherite = OBJ_INHERIT,	// 表示内核对象的句柄可以被继承
};
DEFINE_ENUM_FLAG_OPERATORS(ObjectAttributesFlags);

// 描述了需要打开或创建的内核对象属性
struct ObjectAttributes :OBJECT_ATTRIBUTES {
	// rootDirectory表示对象的根目录句柄
	// name 表示对象的路径或名字，与rootDirectory共同组成了一个完整的对象全路径名字
	// flags 表示对象打开或创建时的具体属性
	ObjectAttributes(PUNICODE_STRING name, ObjectAttributesFlags flags = ObjectAttributesFlags::None,
		HANDLE rootDirectory = nullptr, PSECURITY_DESCRIPTOR sd = nullptr);
};