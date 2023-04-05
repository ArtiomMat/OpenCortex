#pragma once

#include "../Include/OpenCortex.hpp"

namespace OpenCortex {
	extern const char* LogName;
	// -1 error, 0 nore/warning, 1 success
	extern void Log(char Status, const char* MsgFmt, ...);

	extern bool CheckFileExists(const char* FP);
	
	extern int CheckDirFilled(const char* DP);
	extern bool CreateDir(const char* DP);
}
