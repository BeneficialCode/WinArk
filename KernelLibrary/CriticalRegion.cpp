#include "pch.h"
#include "CriticalRegion.h"

void CriticalRegion::Enter() {
	KeEnterCriticalRegion();
}

void CriticalRegion::Leave() {
	KeLeaveCriticalRegion();
}