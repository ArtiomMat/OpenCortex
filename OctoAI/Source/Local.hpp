#pragma once

#include "../Include/OctoAI.hpp"

#define Printf(Fmt, Args)

namespace OAI {
	extern TF8 LeakyRELU_M;

	extern const char* LogName;
	extern void Log(const char* MsgFmt, ...);

	extern bool CheckFileExists(const char* FP);
	
	extern int CheckDirFilled(const char* DP);
	extern bool CreateDir(const char* DP);
}
