#include <math.h>
#include <string.h>

#include "Local.hpp"
#include "Threads.hpp"

namespace OpenCortex {
	const TNeuralModel::TLayer TNeuralModel::TLayer::Null{0};
	
	TNeuralModel::TRunner::TRunner(TNeuralModel* NM) {
		this->NM = NM;
		
		EntireBuf = new TF32[NM->MaxUnitsN * 2];
		// for (unsigned I = 0; I <NM->MaxUnitsN; I++)
		// 	EntireBuf[I] = 0;
		Bufs[0] = EntireBuf;
		Bufs[1] = Bufs[0] + NM->MaxUnitsN;
		printf("EntireBuffer=%p\n", EntireBuf);
	}

	void TNeuralModel::TRunner::Reset() {
		TWI = 0;
		TNI = 0;
		FedBufI = 1;
	}

	TF32* TNeuralModel::TRunner::GetInputBuffer() {
		return Bufs[!FedBufI];
	}

	const TF32 TNeuralModel::TRunner::LeakyRELU_M = 0.01f;

	void TNeuralModel::TRunner::Activate(TF32& V, int Func) {
		switch (Func) {
			case RELU:
			if (V < 0)
				V = 0;
			break;
			
			case LeakyRELU:
			if (V < 0)
				V *= LeakyRELU_M;
			break;

			case Sigmoid:
			V = 1/(1+expf(-V));
			break;

			case TanH:
			Log(0, "V = %f\n", V);
			V = tanhf(V);
			Log(0, "V = %f\n", V);
			break;
		}
	}

	// Assumes input buffer was written to manually, GetInputBuffer().
	TF32* TNeuralModel::TRunner::Run() {
		// Run first layer, first copy the input array
		RunLayer(0, NM->InputUnitsN);

		// Run the rest of the layers
		for (unsigned LI = 1; LI < NM->LayersN; LI++)
			RunLayer(LI, NM->Layers[LI-1].NeuronsN);

		Reset();
		return Bufs[!FedBufI];
	}

	void TNeuralModel::TRunner::Run(TF32* Input, TF32* Output) {
		// Copy to the input buffer.
		memcpy(GetInputBuffer(), Input, NM->InputUnitsN);
		// Run first layer, first copy the input array
		RunLayer(0, NM->InputUnitsN);

		// Run the rest of the layers
		for (unsigned LI = 1; LI < NM->LayersN; LI++)
			RunLayer(LI, NM->Layers[LI-1].NeuronsN);

		Reset();
		memcpy(Output, Bufs[!FedBufI], NM->Layers[NM->LayersN-1].NeuronsN);
	}

	// So this is meant to be ran multiple times per layer, on different chunks of it, simultaneously. It's part of what makes THE G.O.A.T, THE G.O.A.T.
	// 
	void TNeuralModel::TRunner::RunChunk(unsigned LI, unsigned FirstI, unsigned LastI, unsigned PrevNeuronsN) {
		// Since we skip the FirstI neurons we create a localTWI that later is added to the global TWI after running all the chunks
		unsigned LocalTWI = TWI + (PrevNeuronsN * FirstI);
		
		for (unsigned RelNI = FirstI; RelNI <= LastI; RelNI++) {
			unsigned FinalNI = RelNI + TNI; 
			Bufs[FedBufI][RelNI] = 0;

			// if (LI == 0)
			// 	printf("BEFORE: RelNI=%f\n", Bufs[FedBufI][RelNI]);
			// WIRES || WEIGHTS
			for (unsigned RelWI = 0; RelWI < PrevNeuronsN; RelWI++) {
				unsigned FinalWI = RelWI + LocalTWI;
				Bufs[FedBufI][RelNI] += NM->Wires[FinalWI].Weight * Bufs[!FedBufI][RelWI];
				if (LI == 0 && isnanf(NM->Wires[FinalWI].Weight))
					printf("WIRES(LI=%d): RelNI(%d)+=(%d)%f*%f\n", LI, RelNI, RelWI, NM->Wires[FinalWI].Weight, Bufs[!FedBufI][RelWI]);
			}
			// BIAS
			Bufs[FedBufI][RelNI] += NM->Neurons[FinalNI].Bias;
			// if (LI == 0)
			// 	printf("BIAS: RelNI+=%f\n", NM->Neurons[FinalNI].Bias);
			// if (LI == 0)
			// 	printf("RESULT: RelNI=%f\n\n", Bufs[FedBufI][RelNI]);
			// ACTIVATION
			Activate(Bufs[FedBufI][RelNI], NM->Layers[LI].Func);

			LocalTWI += PrevNeuronsN;
		}
	}

	void TNeuralModel::TRunner::RunLayer(unsigned LI, unsigned PrevNeuronsN) {
		// This is where we hand out the RunChunk to a pro multi-threading MF, but we run one thread for now so it's just a single call to the entire layer.
		RunChunk(LI, 0, NM->Layers[LI].NeuronsN-1, PrevNeuronsN);

		TWI += PrevNeuronsN * NM->Layers[LI].NeuronsN; // Increment the total wire index
		TNI += NM->Layers[LI].NeuronsN; // Same with neurons

		FedBufI = !FedBufI; // Swap buffers
	}

	TNeuralModel::TRunner::~TRunner() {
		delete [] EntireBuf;
	}

	TNeuralModel::TNeuralModel(const char* FP) {
		Load(FP);
	}

	TNeuralModel::TNeuralModel(int InputUnitsN, TLayer* L) {
		LogName = "TNeuralModel::TNeuralModel";

		this->InputUnitsN = InputUnitsN;

		unsigned MaxUnitsLayerI = -1;
		MaxUnitsN = InputUnitsN; // Number of units in the biggest layer.
		if (L[0].NeuronsN > InputUnitsN) {
			MaxUnitsLayerI++;
			MaxUnitsN = L[0].NeuronsN;
		}

		unsigned TotalNeuronsN = L[0].NeuronsN;
		unsigned TotalWiresN = L[0].NeuronsN * InputUnitsN;

		for (LayersN = 1; L[LayersN].NeuronsN; LayersN++) {
			int I = LayersN;

			TotalNeuronsN += L[I].NeuronsN;
			
			TotalWiresN += L[I].NeuronsN * L[I-1].NeuronsN;
			
			if (L[I].NeuronsN > MaxUnitsN)
				MaxUnitsLayerI = I;
		}

		Neurons = new TNeuron [TotalNeuronsN];
		Wires = new TWire [TotalWiresN];
		Layers = new TLayer [LayersN];

		// Now setup all the layers
		for (unsigned I = 0; I < LayersN; I++)
			Layers[I] = L[I];
		for (unsigned I = 0; I < TotalWiresN; I++) {
			float Divisor;
			while (!(Divisor = (float)(Rng() % 9999)));
			Wires[I].Weight = (float)(Rng() % 10) / Divisor;
			// printf("%f, ", Wires[I].Weight.ToFloat());
		}
		for (unsigned I = 0; I < TotalNeuronsN; I++) {
			float Divisor;
			while (!(Divisor = (float)(Rng() % 9999)));
			Neurons[I].Bias = (float)(Rng() % 10) / Divisor;
			// printf("%f\n", Neurons[I].Bias);
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

	void TNeuralModel::Run(TF32* Arr, TF32* Output) {
		TRunner State(this);

		State.Run(Arr, Output);
	}

	void TNeuralModel::Run(TMap2D* Maps, int MapsN) {
		TF32* Arr = new TF32[InputUnitsN];
		int Offset = 0;

		// First we get the size of the array
		for (int MapI = 0; MapI < MapsN; MapI++) {
			int N = Maps[MapI].ChannelsNum*Maps[MapI].Height*Maps[MapI].Width;
			// memcpy(Arr+Offset, Maps[MapI].Data, N);
			for (int PixelI = 0; PixelI < N; PixelI++)
				Arr[PixelI] = Maps[MapI].Data[PixelI]/255.0f;
			Offset += N;
		}

		Run(Arr, NULL);

		delete [] Arr;
	}

	int TNeuralModel::CalcMemory(int InputsN, TLayer* Layers) {
		unsigned long long Bytes = Layers[0].NeuronsN * (sizeof(TNeuron) + sizeof(TWire)*InputsN);

		for (int I = 1; Layers[I].NeuronsN; I++)
			Bytes += Layers[I].NeuronsN * (sizeof(TNeuron) + sizeof(TWire)*Layers[I-1].NeuronsN);
		
		return Bytes/1000000;
	}

	bool TNeuralModel::Save(const char* FP) {
		if (CheckFileExists(FP))
			return false;
		
		FILE* F = fopen(FP, "wb");
		if (!F)
			return false;
		
		fwrite(&InputUnitsN, sizeof(InputUnitsN), 1, F);
		fwrite(&MaxUnitsN, sizeof(MaxUnitsN), 1, F);
		
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
		fread(&MaxUnitsN, sizeof(MaxUnitsN), 1, F);
		
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

		Log(1, "Successful setup. fitting starting.\n");
		// Log(1, "Go make a cup of Shoko or some shit.\n");

		TRunner State(this);

		unsigned OutputSize = Layers[LayersN-1].NeuronsN;
		
		TF32* OutputBuffers = new TF32[OutputSize*3];
		
		TF32* WantedOutput		= OutputBuffers+OutputSize*0;
		TF32* Output			= OutputBuffers+OutputSize*1;
		TF32* AvgCosts			= OutputBuffers+OutputSize*2;

		TFitnessGuider::TEpochState ES;

		// Epochs
		while (true) {
			// Batches
			static unsigned TrackedBatchI = 0;
			static unsigned BackupI = 0;
			for (unsigned BatchI = 0; BatchI < Guider.BatchesN; BatchI++, TrackedBatchI++) {
				// Samples
				for (unsigned SampleI = 0; SampleI < Guider.BatchSize; SampleI++) {
					// Running the model
					Guider.OnNextSample(State.GetInputBuffer(), WantedOutput);
					Output = State.Run();
					
					// Adding to the average costs
					for (unsigned I = 0; I < OutputSize; I++) {
						// C=WO-O formula
						TF32 C = (Output[I] - WantedOutput[I]);
						AvgCosts[I] += C;
						// Log(0, "C%d = %f = %f - %f\n", I, C, Output[I], WantedOutput[I]);
					}

				}
				if (TrackedBatchI >= 1) {
					return true;
				}

				// WELCOME CUNTS! THE FITTING SECTION.
				for (unsigned I = 0; I < OutputSize; I++) {
					AvgCosts[I] /= Guider.BatchSize;
					Log(0, "C%d = %f\n", I, AvgCosts[I]);
					AvgCosts[I] = 0;

					// TODO: Rewrite the running logic to save the network's activations :3.
					// We need it to calculate the back propogations.
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
				break;	if (ES.EpochI > Guider.MaxEpochsN)
				
				ES.EpochI++;
			}
		}

		delete [] OutputBuffers;

		return true;
	}

}