#pragma once

struct ExecutiveResource {
	NTSTATUS Init();
	void Delete();

	void Lock();
	void Unlock();

	void LockShared();
	void UnlockShared();

private:
	ERESOURCE m_res;
	bool m_CritRegion;
};