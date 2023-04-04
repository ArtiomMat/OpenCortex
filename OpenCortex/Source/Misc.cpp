#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

namespace OpenCortex {
	static long long RngSeed = 1;
	void SetRngSeed(long long seed) {
		RngSeed = seed;
	}
	// Good luck getting repeating values out of this
	int Rng(void) {
		RngSeed = (RngSeed * 0x2051064DE3) >> 32;
		return (int)RngSeed;
	}

	const char* LogName = nullptr, * LastLogName = nullptr;
	void Log(char Status, const char* MsgFmt, ...) {
		va_list Args;

		if (LogName != LastLogName) {
			// 2m does underline
			printf("\x1B[7mOpenCortex::%s()\x1B[0m\n", LogName);
		}

		if (Status != 0) {
			if (Status == 1) Status = '2';
			else if (Status == -1) Status = '1';
			
			printf("\t\x1B[9%cm", Status);
		}
		else
			putc('\t', stdout);

		va_start(Args, MsgFmt);
		vprintf(MsgFmt, Args);
		va_end(Args);

		fputs("\x1B[0m", stdout);

		LastLogName = LogName;
	}

	void OldLog(char Status, const char* MsgFmt, ...) {
		va_list Args;

		printf("\x1B[4mOpenCortex::%s()\x1B[0m: ", LogName);
		
		if (Status != 0) {
			if (Status == 1) Status = '2';
			else if (Status == -1) Status = '1';
			
			printf("\x1B[9%cm", Status);
		}

		va_start(Args, MsgFmt);
		vprintf(MsgFmt, Args);
		va_end(Args);

		fputs("\x1B[0m", stdout);
	}

}
