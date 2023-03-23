#include <math.h>
#include <string.h>

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
}
