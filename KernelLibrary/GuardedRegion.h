#pragma once

struct GuardedRegion {
public:
	void Enter();
	void Leave();
};