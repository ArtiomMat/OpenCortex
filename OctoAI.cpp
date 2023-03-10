#include <string.h>

#include "OctoAI.hpp"

namespace OAI {
	int SFixed8::N = 3;

	float SFixed8::ToFloat() {
		return (float)Q/(1<<N);
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

	NeuronsModel::NeuronsModel(int InputUnitsN, Layer* L) {
		_BigLayerI = 0;

		unsigned TotalNeurons = L[0].NeuronsN;
		unsigned TotalWires = L[0].NeuronsN * InputUnitsN;

		// Already at least 2 layers because of the first check.
		for (_LayersN = 1; L[_LayersN].NeuronsN; _LayersN++) {
			int I = _LayersN;

			TotalNeurons += L[I].NeuronsN;
			
			TotalWires += L[I].NeuronsN * L[I-1].NeuronsN;
			
			if (L[I].NeuronsN > L[_BigLayerI].NeuronsN)
				_BigLayerI = I;
		}

		_Neurons = new _Neuron [TotalNeurons];
		_Wires = new _Wire [TotalWires];
		_Layers = new Layer [_LayersN];

		// Now setup all the layers
		for (int I = 0; I < _LayersN; I++)
			_Layers[I] = L[I];
		for (int I = 0; I < TotalWires; I++)
			_Wires[I].Weight = Rng();
		for (int I = 0; I < TotalNeurons; I++)
			_Neurons[I].Bias = Rng();
	}

	NeuronsModel::~NeuronsModel() {
		delete [] _Layers;
	}

	void NeuronsModel::Run(Map* Maps, int MapsN) {
		U8* Arr = new U8[_InputUnitsN];
		int Offset = 0;

		// First we get the size of the array
		for (int I = 0; I < MapsN; I++) {
			int N = Maps[I].ChannelsNum*Maps[I].Height*Maps[I].Width;
			memcpy(Arr+Offset, Maps[I].Data, N);
			Offset += N;
		}

		Run(Arr);

		delete [] Arr;
	}

	static SFixed8 _LeakyRELU_M = 1; // Essentially get the minimum value.
	void NeuronsModel::_Activate(SFixed8& V, int Func) {
		switch (Func) {
			case RELU:
			if (V.Q < 0)
				V = 0;
			break;
			
			case LeakyRELU:
			if (V.Q < 0)
				V *= _LeakyRELU_M;
		}
	}

	void NeuronsModel::_RunLayer(char& FedBufI, int& TNI, int& TWI, SFixed8* Bufs[2], int LI, int PrevNeuronsN) {

		for (int RNI = 0; RNI < _Layers[LI].NeuronsN; RNI++) {
			int NI = RNI + TNI;
			Bufs[FedBufI][RNI] = 0;

			// WEIGHTS
			for (int RWI = 0; RWI < PrevNeuronsN; RWI++) {
				int WI = TWI + RWI;
				Bufs[FedBufI][RNI] += _Wires[WI].Weight * Bufs[!FedBufI][RWI];
			}

			TWI += PrevNeuronsN;

			// BIAS
			Bufs[FedBufI][RNI] += _Neurons[NI].Bias;

			// Activation
			_Activate(Bufs[FedBufI][RNI], _Layers[LI].Func);
		}

		TNI += _Layers[LI].NeuronsN;
		FedBufI = !FedBufI; // Swap buffers
	}

	void NeuronsModel::_RunFirstLayer(SFixed8* Bufs[2]) {
		// TODO: I beleive this entire ordeal is optimizable
		char FedBufI = 1;
		int TNI = 0, TWI = 0;
		_RunLayer(FedBufI, TNI, TWI, Bufs, 0, _InputUnitsN);
	}

	void NeuronsModel::_RunChunk(int TNI, int TWI, SFixed8* Bufs[2], int FirstLI, int LastLI) {
		char FedBufI = 1;// Index of the fed buffer, they are switched every time
		for (int LI = FirstLI; LI < LastLI; LI++)
			_RunLayer(FedBufI, TNI, TWI, Bufs, LI, _Layers[LI-1].NeuronsN);
	}

	void NeuronsModel::Run(U8* Arr) {
		SFixed8* Bufs[2];
		Bufs[0] = new SFixed8 [_Layers[_BigLayerI].NeuronsN * 2];
		Bufs[1] = Bufs[0] + _Layers[_BigLayerI].NeuronsN;

		

		delete [] Bufs;
	}

	int NeuronsModel::CalcMemory(int InputsN, U16* LayersNeurons) {
		unsigned long long Bytes = LayersNeurons[0] * (sizeof(_Neuron) + sizeof(*_Wires)*InputsN);

		for (int I = 1; LayersNeurons[I]; I++)
			Bytes += LayersNeurons[I] * (sizeof(_Neuron) + sizeof(*_Wires)*LayersNeurons[I-1]);
		
		return Bytes/1000000;
	}
}

#include <time.h>

int main() {
	
	// OAI::SFixed8::N = 4;

	OAI::SFixed8 A = -80, B = 1;

	printf("%f * %f = %f\n", A.ToFloat(), B.ToFloat(), (A*B).ToFloat());

	return 0;
}
