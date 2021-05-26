#pragma once

#include "IHexControl.h"

class InMemoryBuffer : public IBufferManager {
public:
	InMemoryBuffer();

	// Inherited via IBufferManager
	virtual uint32_t GetData(int64_t offset, uint8_t * buffer, uint32_t count) override;
	virtual void Insert(int64_t offset, const uint8_t * data, uint32_t count) override;
	virtual void Delete(int64_t offset, uint32_t count) override;
	virtual void SetData(int64_t offset, const uint8_t * data, uint32_t count) override;
	virtual int64_t GetSize() const override;
	virtual uint8_t* GetRawData(int64_t offset) override;

private:
	std::vector<uint8_t> _buffer;
};

