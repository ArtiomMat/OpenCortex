#include <math.h>
#include <string.h>

#include "Local.hpp"
#include "Threads.hpp"

namespace OAI {
	const TNeuronsModel::TLayer TNeuronsModel::TLayer::Null{0};
	
	TNeuronsModel::TRunState::TRunState(int BigLayerNeuronsN) {
		EntireBuf = new TF8 [BigLayerNeuronsN * 2];
		
		Bufs[0] = EntireBuf;
		Bufs[1] = Bufs[0] + BigLayerNeuronsN;
	}

	TNeuronsModel::TRunState::~TRunState() {
		delete [] EntireBuf;
	}

	TNeuronsModel::TNeuronsModel(const char* FP) {
		Load(FP);
	}

	TNeuronsModel::TNeuronsModel(int InputUnitsN, TLayer* L) {
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

		Neurons = new TNeuron [TotalNeurons];
		Wires = new TWire [TotalWires];
		Layers = new TLayer [LayersN];

		// Now setup all the layers
		for (unsigned I = 0; I < LayersN; I++)
			Layers[I] = L[I];
		puts("Weights:");
		for (unsigned I = 0; I < TotalWires; I++) {
			Wires[I].Weight = rand() % 255;
			printf("%f, ", Wires[I].Weight.ToFloat());
		}
		puts("\nBiases:");
		for (unsigned I = 0; I < TotalNeurons; I++) {
			Neurons[I].Bias = rand() % 255;
			printf("%f, ", Neurons[I].Bias.ToFloat());
		}
		
		puts("\n");
	}

	void TNeuronsModel::Free() {
		delete [] Neurons;
		delete [] Wires;
		delete [] Layers;
	}

	TNeuronsModel::~TNeuronsModel() {
		Free();
	}

	TF8 LeakyRELU_M = 1; // The smallest possible F8 value.
	void TNeuronsModel::Activate(TF8& V, int Func) {
		
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

	void TNeuronsModel::RunChunk(TRunState& State, int LI, int FirstI, int LastI, int PrevNeuronsN) {
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

	void TNeuronsModel::RunLayer(TRunState& State, int LI, int PrevNeuronsN) {
		RunChunk(State, LI, 0, Layers[LI].NeuronsN-1, PrevNeuronsN);

		State.TWI += PrevNeuronsN * Layers[LI].NeuronsN; // Increment the total wire index
		State.TNI += Layers[LI].NeuronsN; // Same with neurons
		State.FedBufI = !State.FedBufI; // Swap buffers
	}

	void TNeuronsModel::Run(TF8* Arr, TF8* Output) {
		TRunState State(Layers[BigLayerI].NeuronsN);

		// Run first layer, first copy the input array
		memcpy(State.Bufs[!State.FedBufI], Arr, InputUnitsN);
		RunLayer(State, 0, InputUnitsN);

		// Run the rest of the layers
		for (unsigned LI = 1; LI < LayersN; LI++)
			RunLayer(State, LI, Layers[LI-1].NeuronsN);

		memcpy(Output, State.Bufs[!State.FedBufI], Layers[LayersN-1].NeuronsN);
	}

	void TNeuronsModel::Run(TMap* Maps, int MapsN) {
		TF8* Arr = new TF8[InputUnitsN];
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

	int TNeuronsModel::CalcMemory(int InputsN, TU16* LayersNeurons) {
		unsigned long long Bytes = LayersNeurons[0] * (sizeof(TNeuron) + sizeof(*Wires)*InputsN);

		for (int I = 1; LayersNeurons[I]; I++)
			Bytes += LayersNeurons[I] * (sizeof(TNeuron) + sizeof(*Wires)*LayersNeurons[I-1]);
		
		return Bytes/1000000;
	}

	bool TNeuronsModel::Save(const char* FP) {
		if (CheckFileExists(FP))
			return false;
		
		FILE* F = fopen(FP, "wb");
		if (!F)
			return false;
		
		fwrite(&InputUnitsN, sizeof(InputUnitsN), 1, F);
		fwrite(&BigLayerI, sizeof(BigLayerI), 1, F);
		
		fwrite(&LayersN, sizeof(LayersN), 1, F);

		fwrite(Layers, sizeof(TLayer), LayersN, F);
		
		TU32 TotalNeuronsN = Layers[0].NeuronsN;
		TU32 TotalWiresN = Layers[0].NeuronsN * InputUnitsN;

		// Already at least 2 layers because of the first check.
		for (unsigned LayerI = 1; Layers[LayerI].NeuronsN; LayerI++) {
			TotalNeuronsN += Layers[LayerI].NeuronsN;
			TotalWiresN += Layers[LayerI].NeuronsN * Layers[LayerI-1].NeuronsN;
		}
		printf("%u\n", TotalWiresN);
		fflush(stdout);
		
		// TotalNeuronsN and TotalWiresN so load doesn't have to worry.
		fwrite(&TotalNeuronsN, sizeof(TotalNeuronsN), 1, F);
		fwrite(Neurons, sizeof(TNeuron), TotalNeuronsN, F);

		fwrite(&TotalWiresN, sizeof(TotalWiresN), 1, F);
		fwrite(Wires, sizeof(TWire), TotalWiresN, F);

		fclose(F);

		return true;
	}

	bool TNeuronsModel::Load(const char* FP) {
		Free();

		FILE* F = fopen(FP, "rb");
		if (!F)
			return false;
		
		fread(&InputUnitsN, sizeof(InputUnitsN), 1, F);
		fread(&BigLayerI, sizeof(BigLayerI), 1, F);
		
		fread(&LayersN, sizeof(LayersN), 1, F);
		Layers = new TLayer[LayersN];
		fread(Layers, sizeof(TLayer), LayersN, F);
		
		TU32 TotalNeuronsN, TotalWiresN;

		// TotalNeuronsN and TotalWiresN so load doesn't have to worry.
		fread(&TotalNeuronsN, sizeof(TotalNeuronsN), 1, F);
		Neurons = new TNeuron[TotalNeuronsN];
		fread(Neurons, sizeof(TNeuron), TotalNeuronsN, F);

		fread(&TotalWiresN, sizeof(TotalWiresN), 1, F);
		printf("%u\n", TotalWiresN);
		fflush(stdout);
		Wires = new TWire[TotalWiresN];
		fread(Wires, sizeof(TWire), TotalWiresN, F);

		puts("Weights:");
		for (unsigned I = 0; I < TotalWiresN; I++) {
			printf("%f, ", Wires[I].Weight.ToFloat());
		}
		puts("\nBiases:");
		for (unsigned I = 0; I < TotalNeuronsN; I++) {
			printf("%f, ", Neurons[I].Bias.ToFloat());
		}

		fclose(F);
		return true;
	}

	/*
		Some notes:
		We are dealing with fucking 8 bit fixed points. So it makes sense we modify the general formula a little.

		TODO: Experiment with hybrid function, C=MSE at D<1, C=MAE at D>1
		* C = |WO-O|, instead of C=(WO-O)^2.
	*/
	bool TNeuronsModel::Fit(TFitnessGuider& Guider) {
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

		TRunState State(Layers[BigLayerI].NeuronsN);
		unsigned OutputSize = Layers[LayersN-1].NeuronsN;
		TF8* WantedOutput = new TF8[OutputSize];
		TF8* Output = new TF8[OutputSize];
		struct Cost {
			// U16& BatchesN = Guider.BatchesN;
			TU8 AddedN; // How many elements are inside Added
			TF8 Added; // The added value to the sum;
			
			TF8 Avg;

			void Add(TF8 Element) {
				static TI16 ExQ = Added.Q;
				ExQ += Element;
			}
		}* AvgCosts;

		TFitnessGuider::TEpochState ES;

		// Epochs
		while (true) {
			// Batches
			static unsigned TrackedBatchI = 0;
			// static unsigned BackupI = 0;
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

					//
					for (unsigned I = 0; I < OutputSize; I++) {
						// C=|WO-O|  formula
						TF8 C = (Output[I] - WantedOutput[I]);
						// C *= C;
						if (C.Q < 0)
							C.Q = -C.Q;
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
				if (ES.EpochI > Guider.MaxEpochsN)
					break;
				ES.EpochI++;
			}
		}

		delete [] WantedOutput;
		delete [] Output;

		return true;
	}

}