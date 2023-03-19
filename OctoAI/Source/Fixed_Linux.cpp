#include "Local.hpp"

namespace OAI {
	int F8::N = 0;

	F8::F8(I8 X) {
		Q = X;
	}
	F8& F8::operator=(I8 X) {
		Q = X;
		return *this;
	}
	
	float F8::ToFloat() {
		return ((float)Q)/(1<<N);
	}

	F8 F8::operator+(F8& O) {
		return F8(Q + O.Q);
	}
	F8& F8::operator+=(F8 O) {
		Q += O.Q;
		return *this;
	}
	
	F8 F8::operator*(F8& O) {
		printf("\n%f*%f=", ToFloat(), O.ToFloat());
		printf("%f\n", F8((Q * O.Q) >> N).ToFloat());
		return F8((Q * O.Q) >> N);
	}
	F8& F8::operator*=(F8& O) {
		Q = (Q * O.Q) >> N;
		return *this;
	}
}
