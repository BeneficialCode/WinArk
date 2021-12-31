#pragma once

template<typename TRegion>
struct AutoEnter {
	AutoEnter(TRegion& region) :_region(region) {
		_region.Enter();
	}

	~AutoEnter() {
		_region.Leave();
	}
private:
	TRegion& _region;
};