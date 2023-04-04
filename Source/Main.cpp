#include <time.h>
#include <byteswap.h>

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

static OAI::TU32 ImagesN, W, H;

char LoadIndex(FILE* F, unsigned I, OAI::TU8* Data) {
	if (I >= ImagesN)
		return false;
	fseek(F, W*H*I + sizeof(OAI::TU32)*4, SEEK_SET);
	fread(Data, sizeof(OAI::TU8), W*H*1, F);
	return true;
}

int main() {
	// OAI::TNeuronsModel::TLayer L[] = {
	// 	OAI::TNeuronsModel::TLayer{OAI::LeakyRELU, 5},
	// 	OAI::TNeuronsModel::TLayer{OAI::LeakyRELU, 3},
	// 	OAI::TNeuronsModel::TLayer{0, 1},
	// };
	// OAI::TNeuronsModel Model("Model.m");

	// Model.Fit()

	FILE* F = fopen("mnist.bin", "rb");
	if (!F)
		return 1;
	fseek(F, 4, SEEK_SET);
	fread(&ImagesN, sizeof(OAI::TU32), 1, F);
	fread(&W, sizeof(OAI::TU32), 1, F);
	fread(&H, sizeof(OAI::TU32), 1, F);
	ImagesN = __bswap_32(ImagesN);
	W = __bswap_32(W);
	H = __bswap_32(H);
	OAI::TU8* Data = new OAI::TU8[W*H*1];
	LoadIndex(F, 0, Data);
	
	OAI::TMap2D::TConfig Cfg{.Width=(OAI::TU16)W,.Height=(OAI::TU16)H};
	OAI::TMap2D Map(Cfg, Data);
	Map.Resize(12,12);
	Map.Save("test.png");

	return 0;
}
