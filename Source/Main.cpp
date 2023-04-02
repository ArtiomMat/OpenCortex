#include <time.h>

#include "OctoAI.hpp"

double TestF8(unsigned Iterations) {
	time_t StartT, EndT;
	double ElapsedT;

	time(&StartT);

	OAI::TF8 A, B;

	for (unsigned I = 0; I < Iterations; I++) {
		A = OAI::Rng();
		B = OAI::Rng();
		if (!B)
			continue;
		A /= B;
	}

	time(&EndT);

	ElapsedT = difftime(EndT, StartT);
	return ElapsedT;
}

double TestFloat(unsigned Iterations) {
	time_t StartT, EndT;
	double ElapsedT;

	time(&StartT);

	float A, B;

	for (unsigned I = 0; I < Iterations; I++) {
		A = (float)(OAI::Rng()%10000)/500.0f;
		B = (float)(OAI::Rng()%10000)/500.0f;
		if (B == 0.0f)
			continue;
		A /= B;
	}

	time(&EndT);

	ElapsedT = difftime(EndT, StartT);
	return ElapsedT;
}

int main() {
	// OAI::F8::N = 4;

	// OAI::F8 A, B;

	// for (unsigned I = 0; I < 166666650; I++) {
	// 	A = OAI::Rng();
	// 	B = OAI::Rng();
		
	// 	printf("A = %f\nB = %f\n\n", A.ToFloat(), B.ToFloat());
	// 	fflush(stdout);

	// 	OAI::I16 ExQ = A.Q;

	// 	ExQ <<= OAI::F8::N;
	// 	ExQ /= B.Q;

	// 	OAI::F8::CheckExQ(ExQ);

	// 	A.Q = ExQ;
	// }

	OAI::TF8::SetN(1);
	OAI::TF8::SetN(2);
	OAI::TF8::SetN(3);
	OAI::TF8::SetN(4);
	OAI::TF8::SetN(5);
	OAI::TF8::SetN(6);
	OAI::TF8::SetN(7);

	return 0;
}
