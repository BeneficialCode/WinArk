#pragma once

#include "IHexControl.h"

class EditCommandBase abstract {
public:
	EditCommandBase(const CString& name, IBufferManager* mgr, int64_t offset, int64_t size);
	virtual ~EditCommandBase() = default;

	const CString& GetName() const;
	int64_t GetOffset() const {
		return _offset;
	}

	int64_t GetSize() const {
		return _size;
	}

	int32_t GetSize32Bit() const {
		return static_cast<int32_t>(_size);
	}

	IBufferManager* GetBuffer() {
		return _mgr;
	}

	virtual bool Undo() = 0;
	virtual bool Execute() = 0;

private:
	CString _name;
	int64_t _offset, _size;
	IBufferManager* _mgr;
};

