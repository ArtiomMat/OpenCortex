#include <math.h>
#include <string.h>

#include "Local.hpp"
#include "Threads.hpp"

namespace OAI {
	const NeuronsModel::Layer NeuronsModel::NullLayer{0};
	
	NeuronsModel::RunState::RunState(int BigLayerNeuronsN) {
		EntireBuf = new F8 [BigLayerNeuronsN * 2];
		
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
		for (unsigned I = 0; I < LayersN; I++)
			Layers[I] = L[I];
		puts("Weights:");
		for (unsigned I = 0; I < TotalWires; I++) {
			Wires[I].Weight = rand() % 2;
			printf("%f, ", Wires[I].Weight.ToFloat());
		}
		puts("\nBiases:");
		for (unsigned I = 0; I < TotalNeurons; I++) {
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

	F8 LeakyRELU_M = 1; // The smallest possible F8 value.
	void NeuronsModel::Activate(F8& V, int Func) {
		
		switch (Func) {
			case RELU:
			if (V.Q < 0)
				V = 0;
			break;
			
			case LeakyRELU:
			if (V.Q < 0)
				V *= LeakyRELU_M;
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

	void NeuronsModel::Run(F8* Arr, F8* Output) {
		RunState State(Layers[BigLayerI].NeuronsN);

		// Run first layer, first copy the input array
		memcpy(State.Bufs[!State.FedBufI], Arr, InputUnitsN);
		RunLayer(State, 0, InputUnitsN);

		// Run the rest of the layers
		for (unsigned LI = 1; LI < LayersN; LI++)
			RunLayer(State, LI, Layers[LI-1].NeuronsN);

		memcpy(Output, State.Bufs[!State.FedBufI], Layers[LayersN-1].NeuronsN);
	}

	void NeuronsModel::Run(Map* Maps, int MapsN) {
		F8* Arr = new F8[InputUnitsN];
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

	bool NeuronsModel::Save(const char* FP) {
		if (CheckFileExists(FP))
			return false;
		
		FILE* F = fopen(FP, "wb+");
		if (!F)
			return false;
		
		
		fwrite(&InputUnitsN, sizeof(InputUnitsN), 1, F);
		fwrite(&BigLayerI, sizeof(BigLayerI), 1, F);
		
		fwrite(&LayersN, sizeof(LayersN), 1, F);

		fwrite(Layers, sizeof(Layer), LayersN, F);
		

		fclose(F);
	}

	bool NeuronsModel::Load(const char* FP) {

	}

	bool NeuronsModel::Fit(FitnessGuider& Guider) {
		LogName = "NeuronsModel::Fit"; // Sheesh. That's nasty. no reflection in C++ though :(

		if (!Guider.BatchesN) {
			Log("BatchesN is 0.\n");
			return false;
		}

		// If the directory is not empty, very bad!
		int BackupDirFilled = CheckDirFilled(Guider.BackupDirPath);
		if (BackupDirFilled == -1) { // Doesn't exist
			if (!CreateDir(Guider.BackupDirPath)) {
				Log("'%s' doesn't exist and I can't create it. create it.\n", Guider.BackupDirPath);
				return false;
			}
		}
		else if (BackupDirFilled == 1) {
			Log("'%s' isn't empty. Empty it.\n", Guider.BackupDirPath);
			return false;
		}

		RunState State(Layers[BigLayerI].NeuronsN);
		unsigned OutputSize = Layers[LayersN-1].NeuronsN;
		F8* WantedOutput = new F8[OutputSize];
		F8* Output = nullptr;
		struct {
			U8 AddedElementsN; // How many elements are inside Added
			F8 Added; // The added value to the sum;
			F8 Average;
		} OutputCostAvgs;

		FitnessGuider::EpochState ES;

		// Epochs
		while (true) {
			// Batches
			static unsigned TrackedBatchI = 0;
			static unsigned BackupI = 0;
			for (unsigned BatchI = 0; BatchI < Guider.BatchesN; BatchI++, TrackedBatchI++) {
				// Samples
				for (unsigned SampleI = 0; SampleI < Guider.BatchSize; SampleI++) {
					Guider.OnNextSample(State.Bufs[!State.FedBufI], WantedOutput);
					// Run first layer
					RunLayer(State, 0, InputUnitsN);
					// Run the rest of the layers
					for (unsigned LI = 1; LI < LayersN; LI++)
						RunLayer(State, LI, Layers[LI-1].NeuronsN);

					Output = State.Bufs[!State.FedBufI];

					for (unsigned I = 0; I < OutputSize; I++) {
						F8 Cost = (Output[I] - WantedOutput[I]);
						Cost *= Cost;
					}
				}
				// Check if it's time to backup.
				if (TrackedBatchI >= Guider.BackupBatchIndex) {
					TrackedBatchI = 0;
				}
			}
			if (Guider.OnEpoch(ES)) // Notify guider
				break;
			// The epoch logic, no worries about it.
			if (Guider.MaxEpochsN) {
				static unsigned EpochI = 1;
				if (EpochI > Guider.MaxEpochsN)
					break;
				EpochI++;
			}
		}
		delete [] WantedOutput;

		return true;
	}

}