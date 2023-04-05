#include <time.h>
#include <byteswap.h>

#include "OpenCortex.hpp"

static const int N2 = 5958, N0 = 5923;

static OpenCortex::TU32 ImagesN, W, H;
static FILE* ImgF = fopen("mnist.img", "rb");
static FILE* LblF = fopen("mnist.lbl", "rb");

char LoadImage(unsigned I, OpenCortex::TU8* Data) {
	if (I >= ImagesN)
		return false;
	fseek(ImgF, W*H*I + sizeof(OpenCortex::TU32)*4, SEEK_SET);
	fread(Data, sizeof(OpenCortex::TU8), W*H*1, ImgF);
	return true;
}

char LoadLabel(unsigned I) {
	if (I >= ImagesN)
		return -1;
	char Label;
	fseek(LblF, I + sizeof(OpenCortex::TU32)*2, SEEK_SET);
	fread(&Label, sizeof(OpenCortex::TU8), 1, LblF);
	return Label;
}

void SetupLblF() {
	fseek(LblF, 4, SEEK_SET);
	fread(&ImagesN, sizeof(OpenCortex::TU32), 1, LblF);
	ImagesN = __bswap_32(ImagesN);
}

void SetupImgF() {
	fseek(ImgF, 4, SEEK_SET);
	fread(&ImagesN, sizeof(OpenCortex::TU32), 1, ImgF);
	fread(&W, sizeof(OpenCortex::TU32), 1, ImgF);
	fread(&H, sizeof(OpenCortex::TU32), 1, ImgF);

	// The seems to be big endian, x86 is little endian ha.
	ImagesN = __bswap_32(ImagesN);
	W = __bswap_32(W);
	H = __bswap_32(H);
}

class TFG : public OpenCortex::TNeuralModel::TFitnessGuider {
	public:
	OpenCortex::TU16 BatchSize = 15;
	OpenCortex::TU16 BackupBatchIndex = 20;

	int I = 0;

	TFG(int ImagesN) {
		this->BatchesN = ImagesN/BatchSize;
	}

	void OnNextSample(OpenCortex::TF32* Input, OpenCortex::TF32* DesiredOutput) {
		char Label;
		for (Label = LoadLabel(I); Label != 0 && Label != 2; I++)
			Label = LoadLabel(I);

		fseek(ImgF, W*H*I + sizeof(OpenCortex::TU32)*4, SEEK_SET);
		for (unsigned J = 0; J < W*H; J++) {
			int C = fgetc(ImgF);
			Input[J] = C/255.0f;
		}

		for (int J = 0; J < 10; J++) {
			DesiredOutput[J] = (J == Label) ? 1.0f : -1.0f; // TanH used
		}
	}

	bool OnEpoch(TEpochState& State) {
		I = 0; // Reset I
		return true;
	}
};


int main() {

	if (!ImgF || !LblF) {
		printf("Missing mnist database files.");
		return 1;
	}

	SetupImgF();
	SetupLblF();
	
	// int C2 = 0, C0 = 0;
	// for (unsigned I = 0; I < ImagesN; I++) {
	// 	char L = LoadLabel(I);
	// 	if (L == 2)
	// 		C2++;
	// 	else if (L == 0)
	// 		C0++;
	// }

	// printf("%d 2's and %d 0's\n", C2, C0);
	// return 0;

	// OAI::TMap2D::TConfig Cfg{.Width=(OAI::TU16)W,.Height=(OAI::TU16)H};
	// OAI::TMap2D Map(Cfg, Data);
	// Map.Resize(12,12);
	// // Map.Noise(30);
	// Map.Save("test.png");

	OpenCortex::TNeuralModel::TLayer L[] = {
		OpenCortex::TNeuralModel::TLayer{W*H, OpenCortex::LeakyRELU},
		
		OpenCortex::TNeuralModel::TLayer{W*H/2, OpenCortex::LeakyRELU},
		OpenCortex::TNeuralModel::TLayer{W*H/4, OpenCortex::LeakyRELU},

		OpenCortex::TNeuralModel::TLayer{1, OpenCortex::TanH},
		OpenCortex::TNeuralModel::TLayer::Null,
	};
	OpenCortex::TNeuralModel Model(W*H, L);
	
	// printf("%d\n", OpenCortex::TNeuralModel::CalcMemory(W*H, L));

	TFG Guider(ImagesN);
	OpenCortex::TF32* I = new OpenCortex::TF32[W*H];
	OpenCortex::TF32 O;
	Model.Run(I, &O);
				puts("."); fflush(stdout);

	return 0;
}
