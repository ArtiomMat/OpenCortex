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
static FILE* ImgF = fopen("mnist.img", "rb");
static FILE* LblF = fopen("mnist.lbl", "rb");

char LoadImage(unsigned I, OAI::TU8* Data) {
	if (I >= ImagesN)
		return false;
	fseek(ImgF, W*H*I + sizeof(OAI::TU32)*4, SEEK_SET);
	fread(Data, sizeof(OAI::TU8), W*H*1, ImgF);
	return true;
}

char LoadLabel(unsigned I) {
	if (I >= ImagesN)
		return -1;
	char Label;
	fseek(LblF, I + sizeof(OAI::TU32)*2, SEEK_SET);
	fread(&Label, sizeof(OAI::TU8), 1, LblF);
	return Label;
}

int main() {
	// OAI::TNeuronsModel::TLayer L[] = {
	// 	OAI::TNeuronsModel::TLayer{OAI::LeakyRELU, 5},
	// 	OAI::TNeuronsModel::TLayer{OAI::LeakyRELU, 3},
	// 	OAI::TNeuronsModel::TLayer{0, 1},
	// };
	// OAI::TNeuronsModel Model("Model.m");

	// Model.Fit()

	if (!ImgF || !LblF)
		return 1;
	
	fseek(ImgF, 4, SEEK_SET);
	fseek(LblF, 4, SEEK_SET);
	fread(&ImagesN, sizeof(OAI::TU32), 1, LblF);
	fread(&ImagesN, sizeof(OAI::TU32), 1, ImgF);
	fread(&W, sizeof(OAI::TU32), 1, ImgF);
	fread(&H, sizeof(OAI::TU32), 1, ImgF);
	
	ImagesN = __bswap_32(ImagesN);
	W = __bswap_32(W);
	H = __bswap_32(H);
	
	OAI::TU8* Data = new OAI::TU8[W*H*1];
	
	LoadImage(2, Data);
	printf("Label: %d\n", LoadLabel(2));

	OAI::TMap2D::TConfig Cfg{.Width=(OAI::TU16)W,.Height=(OAI::TU16)H};
	OAI::TMap2D Map(Cfg, Data);
	Map.Resize(12,12);
	// Map.Noise(30);
	Map.Save("test.png");

	return 0;
}
