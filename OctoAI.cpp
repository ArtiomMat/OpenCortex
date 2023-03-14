#include <math.h>
#include <string.h>

#include "OctoAI.hpp"
#include "OctoAI_Threads.hpp"

namespace OAI {
	const NeuronsModel::Layer NeuronsModel::NullLayer{0};
	int SFixed8::N = 0;

	float SFixed8::ToFloat() {
		return ((float)Q)/(1<<N);
	}

	SFixed8::SFixed8(I8 X) {
		Q = X;
	}
	SFixed8& SFixed8::operator=(I8 X) {
		Q = X;
		return *this;
	}
	
	SFixed8 SFixed8::operator+(SFixed8& O) {
		return SFixed8(Q + O.Q);
	}
	SFixed8& SFixed8::operator+=(SFixed8 O) {
		Q += O.Q;
		return *this;
	}
	
	SFixed8 SFixed8::operator*(SFixed8& O) {
		printf("\n%f*%f=", ToFloat(), O.ToFloat());
		printf("%f\n", SFixed8((Q * O.Q) >> N).ToFloat());
		return SFixed8((Q * O.Q) >> N);
	}
	SFixed8& SFixed8::operator*=(SFixed8& O) {
		Q = (Q * O.Q) >> N;
		return *this;
	}

	static long long RngSeed = 1;
	void SetRngSeed(long long seed) {
		RngSeed = seed;
	}
	// Good luck getting repeating values out of this
	int Rng(void) {
		RngSeed = (RngSeed * 0x2051064DE3) >> 32;
		return (int)RngSeed;
	}

	NeuronsModel::RunState::RunState(int BigLayerNeuronsN) {
		EntireBuf = new SFixed8 [BigLayerNeuronsN * 2];
		
		Bufs[0] = EntireBuf;
		Bufs[1] = Bufs[0] + BigLayerNeuronsN;
	}

	NeuronsModel::RunState::~RunState() {
		delete [] EntireBuf;
	}

	NeuronsModel::NeuronsModel(int InputUnitsN, Layer* L) {
		BigLayerI = 0;

		this->InputUnitsN = InputUnitsN;

		unsigned TotalNeurons = L[0].NeuronsN;
		unsigned TotalWires = L[0].NeuronsN * InputUnitsN;

		// Already at least 2 layers because of the first check.
		for (LayersN = 1; L[LayersN].NeuronsN; LayersN++) {
			int I = LayersN;

			TotalNeurons += L[I].NeuronsN;
			
			TotalWires += L[I].NeuronsN * L[I-1].NeuronsN;
			
			if (L[I].NeuronsN > L[BigLayerI].NeuronsN)
				BigLayerI = I;
		}

		Neurons = new Neuron [TotalNeurons];
		Wires = new Wire [TotalWires];
		Layers = new Layer [LayersN];

		// Now setup all the layers
		for (int I = 0; I < LayersN; I++)
			Layers[I] = L[I];
		puts("Weights:");
		for (int I = 0; I < TotalWires; I++) {
			Wires[I].Weight = rand() % 2;
			printf("%f, ", Wires[I].Weight.ToFloat());
		}
		puts("\nBiases:");
		for (int I = 0; I < TotalNeurons; I++) {
			Neurons[I].Bias = rand() % 2;
			printf("%f, ", Neurons[I].Bias.ToFloat());
		}
		
		puts("\n");
	}

	NeuronsModel::~NeuronsModel() {
		delete [] Neurons;
		delete [] Wires;
		delete [] Layers;
	}

	static SFixed8 _LeakyRELU_M = 1; // Essentially get the minimum value.
	void NeuronsModel::Activate(SFixed8& V, int Func) {
		switch (Func) {
			case RELU:
			if (V.Q < 0)
				V = 0;
			break;
			
			case LeakyRELU:
			if (V.Q < 0) {
				printf("\n\nDUDE: %f\n\n", V.ToFloat());
				V *= _LeakyRELU_M;
			}
			break;
		}
	}

	void NeuronsModel::RunChunk(RunState& State, int LI, int FirstI, int LastI, int PrevNeuronsN) {
		// Since we skip the FirstI layers we create a localTWI that later is added to the global TWI after running all the chunks
		unsigned LocalTWI = State.TWI + (PrevNeuronsN * FirstI);

		for (int RNI = FirstI; RNI <= LastI; RNI++) {
			int NI = RNI + State.TNI;
			State.Bufs[State.FedBufI][RNI] = 0;

			// WEIGHTS
			for (int RWI = 0; RWI < PrevNeuronsN; RWI++) {
				int WI = RWI + LocalTWI;
				State.Bufs[State.FedBufI][RNI] += Wires[WI].Weight * State.Bufs[!State.FedBufI][RWI];
			}

			LocalTWI += PrevNeuronsN;

			// BIAS
			State.Bufs[State.FedBufI][RNI] += Neurons[NI].Bias;

			// Activation
			Activate(State.Bufs[State.FedBufI][RNI], Layers[LI].Func);
		}
	}

	void NeuronsModel::RunLayer(RunState& State, int LI, int PrevNeuronsN) {
		RunChunk(State, LI, 0, Layers[LI].NeuronsN-1, PrevNeuronsN);

		State.TWI += PrevNeuronsN * Layers[LI].NeuronsN; // Increment the total wire index
		State.TNI += Layers[LI].NeuronsN; // Same with neurons
		State.FedBufI = !State.FedBufI; // Swap buffers
	}

	void NeuronsModel::Run(U8* Arr, SFixed8* Output) {
		RunState State(Layers[BigLayerI].NeuronsN);

		// Run first layer, first copy the input array
		memcpy(State.Bufs[!State.FedBufI], Arr, InputUnitsN);
		RunLayer(State, 0, InputUnitsN);

		// Run the rest of the layers
		for (int LI = 1; LI < LayersN; LI++)
			RunLayer(State, LI, Layers[LI-1].NeuronsN);

		memcpy(Output, State.Bufs[!State.FedBufI], Layers[LayersN-1].NeuronsN);
	}

	void NeuronsModel::Run(Map* Maps, int MapsN) {
		U8* Arr = new U8[InputUnitsN];
		int Offset = 0;

		// First we get the size of the array
		for (int I = 0; I < MapsN; I++) {
			int N = Maps[I].ChannelsNum*Maps[I].Height*Maps[I].Width;
			memcpy(Arr+Offset, Maps[I].Data, N);
			Offset += N;
		}

		Run(Arr, NULL);

		delete [] Arr;
	}

	int NeuronsModel::CalcMemory(int InputsN, U16* LayersNeurons) {
		unsigned long long Bytes = LayersNeurons[0] * (sizeof(Neuron) + sizeof(*Wires)*InputsN);

		for (int I = 1; LayersNeurons[I]; I++)
			Bytes += LayersNeurons[I] * (sizeof(Neuron) + sizeof(*Wires)*LayersNeurons[I-1]);
		
		return Bytes/1000000;
	}
}

#include <time.h>

int main() {
	
	// OAI::SFixed8::N = 4;

	OAI::NeuronsModel::Layer L[] = 
	{
		OAI::NeuronsModel::Layer{2, OAI::LeakyRELU},
		OAI::NeuronsModel::Layer{1, OAI::LeakyRELU},
		OAI::NeuronsModel::NullLayer
	};

	OAI::NeuronsModel M(2, L);
	OAI::U8 Input[] = {2<<1,2<<2};
	OAI::SFixed8 Output[1];
	M.Run(Input, Output);

	printf("%f\n", Output[0].ToFloat());

	return 0;
}
