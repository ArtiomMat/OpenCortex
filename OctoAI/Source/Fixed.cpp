#include "Local.hpp"

namespace OAI {
	int F8::N = 4;

	float F8::ToFloat() {
		return ((float)Q)/(1<<N);
	}
	
}
