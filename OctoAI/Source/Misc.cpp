#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

namespace OAI {
	static long long RngSeed = 1;
	void SetRngSeed(long long seed) {
		RngSeed = seed;
	}
	// Good luck getting repeating values out of this
	int Rng(void) {
		RngSeed = (RngSeed * 0x2051064DE3) >> 32;
		return (int)RngSeed;
	}

	char* LogName = nullptr;
	void Log(const char* MsgFmt, ...) {
		va_list Args;

		printf("OAI::%s(): ", LogName);

		va_start(Args, MsgFmt);
		printf(MsgFmt, Args);
		va_end(Args);
	}

}
