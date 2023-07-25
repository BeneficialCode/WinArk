#include "pch.h"
#include "ProcessHandleTracker.h"
#include <unordered_set>
#include <assert.h>

using namespace WinSys;

bool HandleEntryInfo::operator==(const HandleEntryInfo& other) const {
	return HandleValue == other.HandleValue;
}

template<>
struct std::hash<HandleEntryInfo> {
	size_t operator()(const HandleEntryInfo& key) const {
		return (size_t)key.HandleValue ^ ((int32_t)key.ObjectTypeIndex << 16);
	}
};

struct ProcessHandleTracker::Impl {
	Impl(uint32_t pid);
	Impl(HANDLE hProcess);

	uint32_t EnumHandles(bool clearHistory);
	const std::vector<HandleEntryInfo>& GetNewHandles() const {
		return _newHandles;
	}
	const std::vector<HandleEntryInfo>& GetClosedHandles() const {
		return _closedHandles;
	}

	bool IsValid() const {
		return _hProcess.is_valid();
	}

private:
	wil::unique_handle _hProcess;
	std::vector<HandleEntryInfo> _closedHandles, _newHandles;
	// unordered_set与unordered_map区别：
	// unordered_set就是在hash表中插入value值，而这个value值就是它自己的key值
	std::unordered_set<HandleEntryInfo> _handles;
};

ProcessHandleTracker::Impl::Impl(uint32_t pid) :
	Impl::Impl(::OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid)) {
}

ProcessHandleTracker::Impl::Impl(HANDLE hProcess) : _hProcess(hProcess) {
	if (_hProcess) {
		_newHandles.reserve(16);
		_closedHandles.reserve(16);
	}
}

uint32_t ProcessHandleTracker::Impl::EnumHandles(bool clearHistory) {
	if (!_hProcess)
		return 0;

	auto rc = ::WaitForSingleObject(_hProcess.get(), 0);
	assert(rc != WAIT_FAILED);

	if (rc == WAIT_OBJECT_0) {
		_closedHandles.clear();
		_closedHandles.insert(_closedHandles.begin(), _handles.begin(), _handles.end());
		_newHandles.clear();
		_handles.clear();
		return 0;
	}

	auto size = 1 << 22;
	ULONG len;
	std::unique_ptr<BYTE[]> buffer;
	for (;;) {
		buffer = std::make_unique<BYTE[]>(size);
		auto status = ::NtQueryInformationProcess(_hProcess.get(), ProcessHandleInformation,
			buffer.get(), size, &len);
		if (status == STATUS_SUCCESS)
			break;
		if (STATUS_BUFFER_TOO_SMALL == status) {
			size = len + (1 << 12);
			continue;
		}
		return 0;
	}


	auto info = reinterpret_cast<PROCESS_HANDLE_SNAPSHOT_INFORMATION*>(buffer.get());
	if (clearHistory)
		_handles.clear();

	_newHandles.clear();
	_closedHandles.clear();
	if (_handles.empty()) {
		_newHandles.reserve(1024);
		_closedHandles.reserve(32);
	}

	if (_handles.empty()) {
		_handles.reserve(info->NumberOfHandles);
		for (ULONG i = 0; i < info->NumberOfHandles; i++) {
			const auto& entry = info->Handles[i];
			HandleEntryInfo key = { entry.HandleValue,(uint16_t)entry.ObjectTypeIndex };
			_handles.insert(key);
		}
	}
	else {
		auto oldHandles = _handles;
		for (ULONG i = 0; i < info->NumberOfHandles; i++) {
			const auto& entry = info->Handles[i];
			HandleEntryInfo key = { entry.HandleValue,(uint16_t)entry.ObjectTypeIndex };
			if (_handles.find(key) == _handles.end()) {
				// new handle
				_newHandles.push_back(key);
				_handles.insert(key);
			}
			else {
				// existing handle
				oldHandles.erase(key);
			}
		}
		for (auto& hi : oldHandles) {
			_closedHandles.push_back(hi);
			_handles.erase(hi);
		}
	}

	return static_cast<uint32_t>(_handles.size());
}

ProcessHandleTracker::ProcessHandleTracker(uint32_t pid) :_impl(new Impl(pid)) {
}

ProcessHandleTracker::ProcessHandleTracker(HANDLE hProcess) : _impl(new Impl(hProcess)) {
}

ProcessHandleTracker::~ProcessHandleTracker() = default;

bool WinSys::ProcessHandleTracker::IsValid() const {
	return _impl->IsValid();
}

uint32_t ProcessHandleTracker::EnumHandles(bool clearHistory) {
	return _impl->EnumHandles(clearHistory);
}

const std::vector<HandleEntryInfo>& ProcessHandleTracker::GetNewHandles() const {
	return _impl->GetNewHandles();
}

const std::vector<HandleEntryInfo>& ProcessHandleTracker::GetClosedHandles() const {
	return _impl->GetClosedHandles();
}



