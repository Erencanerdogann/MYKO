#pragma once

#include <windows.h>
#include <stdio.h>
#include "resource.h"

class thyke_Test
{
public:
	// S114: gifResId = IDB_LOADING/SAFE/ERROR, minMs = pencere min acik kalma suresi, launchGame = bitince KO basla
	int SetupBanner(int gifResId = IDB_LOADING, DWORD minMs = 6000, bool launchGame = true);

	thyke_Test() {}
	~thyke_Test() {}
};
