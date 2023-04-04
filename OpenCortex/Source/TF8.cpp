#include "Local.hpp"

namespace OpenCortex {
	int TF8::N = 4;
	thread_local TI16 TF8::ExQ;
	thread_local int TF8::FlowState = 0;

	float TF8::ToFloat() {
		return ((float)Q)/(1<<N);
	}
	
	int TF8::GetN() {
		return N;
	}

	void TF8::SetN(int NewN) {
		N = NewN;

		// Sets LeakyRELU_M regardless of N, as much as percision allows.
		// Simple equation: x/(2^N) = y/(2^M)
		int y = 3, M = 4; // 3/16
		if (N-M <= 0)
			LeakyRELU_M = 1;
		else
			LeakyRELU_M = 1*(y<<(N-M)); // x = y*2^(N-M)
	}
}
