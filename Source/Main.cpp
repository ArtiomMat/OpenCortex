#include <time.h>
#include <byteswap.h>

#include "OctoAI.hpp"

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

void SetupLblF() {
	fseek(LblF, 4, SEEK_SET);
	fread(&ImagesN, sizeof(OAI::TU32), 1, LblF);
	ImagesN = __bswap_32(ImagesN);
}

void SetupImgF() {
	fseek(ImgF, 4, SEEK_SET);
	fread(&ImagesN, sizeof(OAI::TU32), 1, ImgF);
	fread(&W, sizeof(OAI::TU32), 1, ImgF);
	fread(&H, sizeof(OAI::TU32), 1, ImgF);

	// The seems to be big endian, x86 is little endian ha.
	ImagesN = __bswap_32(ImagesN);
	W = __bswap_32(W);
	H = __bswap_32(H);
}

class TFG : public OAI::TNeuronsModel::TFitnessGuider {
	public:
	OAI::TU16 BatchSize = 6;

	int I = 0;

	TFG(int ImagesN) {
		this->BatchesN = ImagesN/10;
	}

	void OnNextSample(OAI::TF8* Input, OAI::TF8* DesiredOutput) {
		// We have to do some tricks to make it -X to X-h range in TF8.

		fseek(ImgF, W*H*I + sizeof(OAI::TU32)*4, SEEK_SET);
		for (unsigned J = 0; J < W*H; J++) {
			int C = fgetc(ImgF);
			C -= 128;
			Input[J].Q = C;
		}

		int Label = LoadLabel(I);
		for (int J = 0; J < 10; J++) {
			DesiredOutput[J].Q = (J == Label) ? 127 : -128;
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
	

	// OAI::TMap2D::TConfig Cfg{.Width=(OAI::TU16)W,.Height=(OAI::TU16)H};
	// OAI::TMap2D Map(Cfg, Data);
	// Map.Resize(12,12);
	// // Map.Noise(30);
	// Map.Save("test.png");

	OAI::TNeuronsModel::TLayer L[] = {
		OAI::TNeuronsModel::TLayer{W*H, OAI::LeakyRELU},
		
		OAI::TNeuronsModel::TLayer{W*H/2, OAI::LeakyRELU},
		OAI::TNeuronsModel::TLayer{W*H/4, OAI::LeakyRELU},

		OAI::TNeuronsModel::TLayer{10, OAI::LeakyRELU},
		OAI::TNeuronsModel::TLayer::Null,
	};
	OAI::TNeuronsModel Model(W*H, L);
	
	TFG Guider(ImagesN);
	
	// printf("%d\n", Guider.BatchesN);

	Model.Fit(Guider);

	return 0;
}
