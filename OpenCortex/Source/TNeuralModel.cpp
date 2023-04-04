#include <math.h>
#include <string.h>

#include "Local.hpp"
#include "Threads.hpp"

namespace OpenCortex {
	const TNeuralModel::TLayer TNeuralModel::TLayer::Null{0};
	
	TNeuralModel::TRunState::TRunState(int BigLayerNeuronsN) {
		EntireBuf = new TF8 [BigLayerNeuronsN * 2];
		
		Bufs[0] = EntireBuf;
		Bufs[1] = Bufs[0] + BigLayerNeuronsN;
	}

	void TNeuralModel::TRunState::Reset() {
			TWI = TNI = 0;
			FedBufI = 1;
	}

	TNeuralModel::TRunState::~TRunState() {
		delete [] EntireBuf;
	}

	TNeuralModel::TNeuralModel(const char* FP) {
		Load(FP);
	}

	TNeuralModel::TNeuralModel(int InputUnitsN, TLayer* L) {
		LogName = "TNeuralModel::TNeuralModel";

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
		for (unsigned I = 0; I < TotalWires; I++) {
			Wires[I].Weight = rand() % 255;
			// printf("%f, ", Wires[I].Weight.ToFloat());
		}
		for (unsigned I = 0; I < TotalNeurons; I++) {
			Neurons[I].Bias = rand() % 255;
			// printf("%f, ", Neurons[I].Bias.ToFloat());
		}

		Log(1, "Setup done.\n");
	}

	void TNeuralModel::Free() {
		delete [] Neurons;
		delete [] Wires;
		delete [] Layers;
	}

	TNeuralModel::~TNeuralModel() {
		Free();
	}

	TF8 LeakyRELU_M = 1; // The smallest possible F8 value.
	void TNeuralModel::Activate(TF8& V, int Func) {
		
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

	void TNeuralModel::RunChunk(TRunState& State, int LI, int FirstI, int LastI, int PrevNeuronsN) {
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

	void TNeuralModel::RunLayer(TRunState& State, int LI, int PrevNeuronsN) {
		RunChunk(State, LI, 0, Layers[LI].NeuronsN-1, PrevNeuronsN);

		State.TWI += PrevNeuronsN * Layers[LI].NeuronsN; // Increment the total wire index
		State.TNI += Layers[LI].NeuronsN; // Same with neurons
		State.FedBufI = !State.FedBufI; // Swap buffers
	}

	void TNeuralModel::Run(TF8* Arr, TF8* Output) {
		TRunState State(Layers[BigLayerI].NeuronsN);

		// Run first layer, first copy the input array
		memcpy(State.Bufs[!State.FedBufI], Arr, InputUnitsN);
		RunLayer(State, 0, InputUnitsN);

		// Run the rest of the layers
		for (unsigned LI = 1; LI < LayersN; LI++)
			RunLayer(State, LI, Layers[LI-1].NeuronsN);

		memcpy(Output, State.Bufs[!State.FedBufI], Layers[LayersN-1].NeuronsN);
	}

	void TNeuralModel::Run(TMap2D* Maps, int MapsN) {
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

	int TNeuralModel::CalcMemory(int InputsN, TU16* LayersNeurons) {
		unsigned long long Bytes = LayersNeurons[0] * (sizeof(TNeuron) + sizeof(*Wires)*InputsN);

		for (int I = 1; LayersNeurons[I]; I++)
			Bytes += LayersNeurons[I] * (sizeof(TNeuron) + sizeof(*Wires)*LayersNeurons[I-1]);
		
		return Bytes/1000000;
	}

	bool TNeuralModel::Save(const char* FP) {
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
		
		// TotalNeuronsN and TotalWiresN so load doesn't have to worry.
		fwrite(&TotalNeuronsN, sizeof(TotalNeuronsN), 1, F);
		fwrite(Neurons, sizeof(TNeuron), TotalNeuronsN, F);

		fwrite(&TotalWiresN, sizeof(TotalWiresN), 1, F);
		fwrite(Wires, sizeof(TWire), TotalWiresN, F);

		fclose(F);

		return true;
	}

	bool TNeuralModel::Load(const char* FP) {
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

		fclose(F);
		return true;
	}

	/*
		Some notes:
		We are dealing with fucking 8 bit fixed points. So it makes sense we modify the general formula a little.

		TODO: Experiment with hybrid function, C=MSE at D<1, C=MAE at D>1
		* C = |WO-O|, instead of C=(WO-O)^2.
	*/
	bool TNeuralModel::Fit(TFitnessGuider& Guider) {
		// I know, it's nasty. no reflection in C++ though.
		LogName = "TNeuralModel::Fit";
		
		// Check if BatchSize is ok to compute with.
		if (!(TF8(127) / TF8(Guider.BatchSize<<TF8::GetN()))) {
			Log(-1, "BatchSize is too big, MaxTF8/it = 0.\n", Guider.BatchSize);
		}

		if (!Guider.BatchesN) {
			Log(-1, "BatchesN = 0. You've got %d samples per batch, so split it up.\n", Guider.BatchSize);
			return false;
		}

		// If the directory is not empty, very bad!
		int BackupDirFilled = CheckDirFilled(Guider.BackupDirPath);
		if (BackupDirFilled == -1) { // Doesn't exist
			if (!CreateDir(Guider.BackupDirPath)) {
				Log(-1, "'%s' doesn't exist, I can't create it. Create it.\n", Guider.BackupDirPath);
				return false;
			}
		}
		else if (BackupDirFilled == 1) {
			Log(-1, "'%s' isn't empty. Empty it.\n", Guider.BackupDirPath);
			return false;
		}

		Log(1, "Successful setup. training starting.\n");
		// Log(1, "Go make a cup of Shoko or some shit.\n");

		return true;

		TRunState State(Layers[BigLayerI].NeuronsN);
		unsigned OutputSize = Layers[LayersN-1].NeuronsN;
		TF8* WantedOutput = new TF8[OutputSize];
		TF8* Output = new TF8[OutputSize];
		// TF8* AvgCosts = new TF8[OutputSize];

		TFitnessGuider::TEpochState ES;

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

					// Derermining the average costs.
					for (unsigned I = 0; I < OutputSize; I++) {
						// C=|WO-O| formula
						TF8 C = (Output[I] - WantedOutput[I]);
						if (C.Q < 0)
							C.Q = -C.Q;
						// Adding to the average
						// TODO
					}
				}
				// Check if it's time to backup.
				if (TrackedBatchI >= Guider.BackupBatchIndex) {
					static char FP[256];
					sprintf(FP, "%s/Backup%d", Guider.BackupDirPath, BackupI++);
					Save(FP);
					TrackedBatchI = 0;
				}
			}
			if (Guider.OnEpoch(ES)) // Notify guider
				break;
			// The epoch logic, no worries about it.
			if (Guider.MaxEpochsN) {
				Log(0, "Epoch %u finished with C=%.3f. C improved %.3f%%", ES.EpochI);
				if (ES.EpochI > Guider.MaxEpochsN)
					break;
				ES.EpochI++;
			}
		}

		delete [] WantedOutput;
		delete [] Output;
		// delete [] AvgCosts;

		return true;
	}

}