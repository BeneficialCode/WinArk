#include "pch.h"
#include "GuardedRegion.h"

void GuardedRegion::Enter() {
	KeEnterGuardedRegion();
}

void GuardedRegion::Leave() {
	KeLeaveGuardedRegion();
}