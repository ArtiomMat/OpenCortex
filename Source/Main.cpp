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
	// OAI::TNeuronsModel::TLayer L[] = {
	// 	OAI::TNeuronsModel::TLayer{OAI::LeakyRELU, 5},
	// 	OAI::TNeuronsModel::TLayer{OAI::LeakyRELU, 3},
	// 	OAI::TNeuronsModel::TLayer{0, 1},
	// };
	OAI::TNeuronsModel Model("Model.m");

	Model.Fit()

	// Model.Save("Model.m");

	return 0;
}
