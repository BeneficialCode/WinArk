#pragma once

struct CriticalRegion {
public:
	void Enter();
	void Leave();
};