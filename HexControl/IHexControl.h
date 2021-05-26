#pragma once

struct IBufferManager {
	virtual uint32_t GetData(int64_t offset, uint8_t* buffer, uint32_t count) = 0;
	virtual void Insert(int64_t offset, const uint8_t* data, uint32_t count) = 0;
	virtual void Delete(int64_t offset, uint32_t count) = 0;
	virtual void SetData(int64_t offset, const uint8_t* data, uint32_t count) = 0;
	virtual int64_t GetSize() const = 0;
	virtual uint8_t* GetRawData(int64_t offset) = 0;
	virtual ~IBufferManager() = default;
};

struct IHexControl {
	virtual void SetBufferManager(IBufferManager* mgr) = 0;
	virtual void SetReadOnly(bool readonly) = 0;
	virtual bool IsReadOnly() const = 0;
	virtual void SetAllowExtension(bool allow) = 0;
	virtual bool IsAllowExtension() const = 0;
	virtual bool CanUndo() const = 0;
	virtual bool CanRedo() const = 0;
	virtual bool Undo() = 0;
	virtual bool Redo() = 0;
	virtual void SetSize(int64_t size) = 0;
	virtual bool SetDataSize(int32_t size) = 0;
	virtual int32_t GetDataSize() const = 0;
	virtual bool SetBytesPerLine(int32_t bytesPerLine) = 0;
	virtual int32_t GetBytesPerLine() const = 0;
	virtual void Copy(int64_t offset = -1, int32_t size = 0) = 0;
	virtual void Paste(int64_t offset = -1) = 0;
};
