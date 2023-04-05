#pragma once

#include <stdio.h>

#ifndef __INT64_TYPE__
	#error No __INT64_TYPE__? are you compiling this for a ?
#endif

namespace OpenCortex {
	typedef __int8_t TI8;
	typedef __int16_t TI16;
	typedef __int32_t TI32;
	typedef __int64_t TI64;

	typedef __uint8_t TU8;
	typedef __uint16_t TU16;
	typedef __uint32_t TU32;
	typedef __uint64_t TU64;

	typedef _Float32 TF32;
	typedef _Float64 TF64;

	// To be used with TSynapticModel
	class TF8 {
		private:
		static thread_local TI16 ExQ;
		static int N;
		
		public:
		// -1=underflow, 0=normalflow, 1=overflow.
		static thread_local int FlowState;
		TI8 Q;

		TF8() {}
		inline TF8(TI8 X) {
			Q = X;
		}

		// N is how many 2's F8::Q is divided by to get the actual value of the fixed number, e.g. N=2,Q=3 <-> Value=3/2**2 = 3/4 = 0.75
		// So the smaller N is, the more percise the whole number can be.
		// Average ratio between performance of run-time and compile-time N is approx 14(sec)/13(sec). The performance difference is minimal, and comes at the advantage of defining your own N.
		static void SetN(int _N);
		static int GetN();

		float ToFloat();

		inline bool operator!() {
			return !Q;
		}

		inline TF8& operator=(TI8 X) {
			Q = X;
			return *this;
		}

		// Returns if capped.
		inline static bool CapExQ(TI16& ExQ) {
			if (ExQ > 127) {
				ExQ = 127;
				return (FlowState=1);
			}
			else if (ExQ < -128) {
				ExQ = -128;
				return (FlowState=-1);
			}
			return (FlowState=1);
		}

		// TODO: Make it use JS and JO jump instructions instead.
		inline TF8 operator+(TF8 O) {
			ExQ = Q;
			ExQ += O.Q;

			CapExQ(ExQ);

			return TF8(ExQ);
		}
		inline TF8& operator+=(TF8 O) {
			ExQ = Q;
			ExQ += O.Q;

			CapExQ(ExQ);

			Q = ExQ;
			return *this;
		}
		inline TF8 operator-(TF8 O) {
			ExQ = Q;
			ExQ -= O.Q;

			CapExQ(ExQ);

			return TF8(ExQ);
		}
		inline TF8& operator-=(TF8 O) {
			ExQ = Q;
			ExQ -= O.Q;
			
			CapExQ(ExQ);

			Q = ExQ;
			return *this;
		}
				// printf("\n%f*%f=", ToFloat(), O.ToFloat());
			// printf("%f\n", TF8((Q * O.Q) >> N).ToFloat());
		inline TF8 operator*(TF8 O) {
			ExQ = Q;
			ExQ *= O.Q;
			ExQ >>= N;
			
			CapExQ(ExQ);

			return TF8(ExQ);
		}
		inline TF8& operator*=(TF8 O) {
			ExQ = Q;
			ExQ *= O.Q;
			ExQ >>= N;
			
			CapExQ(ExQ);

			Q = ExQ;
			return *this;
		}
		inline TF8 operator/(TF8 O) {
			ExQ = Q;
			ExQ <<= N;
			ExQ /= O.Q;
			
			CapExQ(ExQ);

			return TF8(ExQ);
		}
		inline TF8& operator/=(TF8 O) {
			ExQ = Q;
			ExQ <<= N;
			ExQ /= O.Q;

			CapExQ(ExQ);

			Q = ExQ;
			return *this;
		}

	};


	class TMap2D {
		public:
		struct TConfig {
			TU16 Width = 0, Height = 0;
			TU16 ChannelsNum = 1;
		};

		TU16 Width, Height;
		TU8* Data = nullptr;
		TU32 ChannelsNum;
		
		// Copy a Map
		TMap2D(TMap2D& other);
		TMap2D(const char* fp);
		TMap2D(TConfig& Config);
		// WARNING: Data is used, not copied.
		TMap2D(TConfig& Config, TU8* data);
		
		~TMap2D();

		void SetPixel(TU16 x, TU16 y, const TU8* pixel);
		// Returns actual pixel location, don't fuck with this pointer please.
		TU8* GetPixel(TU16 x, TU16 y);
		void GetPixel(TU16 x, TU16 y, TU8* output);

		// Values are capped no worries.
		void Contrast(float Factor);
		// Pixel P becomes P*Factor, values are capped no worries.
		void Lighten(float Factor);
		void Noise(TU8 Strength);
		void NoiseKiller(TU8 Chance);

		void Crop(TU16 left, TU16 top, TU16 right, TU16 bottom);
		void Rotate(float radians, TU16 aroundX, TU16 aroundY);
		void Resize(TU16 w, TU16 h);

		// void ResizeSmooth(TU16 w, TU16 h);

		bool Load(const char* Fp);
		// Automatically determines file type by extension, if it failes to detect file type, saved as RAW(Custom simple format, should only use if there are channels nothing else supports).
		// If determined format doesn't support the channels used it is saved
		bool Save(const char* Fp);

		bool LoadRAW(FILE* F);
		bool LoadJPG(FILE* F);
		bool LoadPNG(FILE* F);
		bool SaveRAW(const char* Fp);
		// 0<=quality<=100, 100 preserves as much details as jpeg possibly allows.
		bool SaveJPG(const char* Fp, int quality);
		bool SavePNG(const char* Fp);

		protected:
		void Allocate(TU16 Width, TU16 Height, TU32 ChannelsNum);
		void Free();
	};

	// Custom model that is aimed at emulating the brain as close as possible.
	class TSynaticModel;

	class TNeuralModel; // Neural based
	class TExpand1DModel; // Deconvolution based
	class TShrink1DModel; // Convolution based
	
	class TModel {
		public:
		virtual bool Load(const char* FP) = 0;
		virtual bool Save(const char* FP) = 0;

		virtual void Run(TMap2D* Maps, int MapsN) = 0;
		virtual void Run(TF32* Arr, TF32* Output) = 0;
	};

	enum TActFunc {
		NullActFunc,
		RELU,
		LeakyRELU,
		Sigmoid,
		TanH,
	};

	class TNeuralModel : public TModel {
		public:
		struct TLayer {
			static const TLayer Null;

			TU32 NeuronsN : 29;
			TU32 Func : 3;
		};
		const static TLayer NullLayer;

		private:
		class TRunner {
			public:
			static const TF32 LeakyRELU_M;

			// It is an index the subroutines of Run() use to keep track of where they are in terms of reading wire/neuron values.
			unsigned TWI = 0, TNI = 0;

			// The buffer that is currently being fed into from the last one
			// Essentially the one where the new activations are.
			TU8 FedBufI = 1;
			TF32* Bufs[2];

			TRunner(TNeuralModel* NM);
			~TRunner();
			
			TF32* GetInputBuffer();
			void Run(TF32* Input, TF32* OutputPtr);
			// Assumes input buffer was written to manually, GetInputBuffer().
			TF32* Run();

			private:
			void Reset();
			void RunLayer(unsigned LI, unsigned PrevNeuronsN);
			void RunChunk(unsigned LI, unsigned FirstI, unsigned LastI, unsigned PrevNeuronsN);
			void Activate(TF32& V, int Func);

			TF32* EntireBuf = nullptr;
			TNeuralModel* NM;
		};

		struct TNeuron {
			TF32 Bias;
		}* Neurons = nullptr;

		struct TWire {
			TF32 Weight;
		}* Wires = nullptr;

		TU32 InputUnitsN;
		TU32 MaxUnitsN;

		TLayer* Layers = nullptr;
		TU16 LayersN;

		void Free();
		public:
		// L includes the last layer ofc.
		// L is terminated with NeuronsModel::NullLayer (NeuronsN = 0)
		TNeuralModel(int InputUnitsN, TLayer* L);
		TNeuralModel(const char* FP);
		~TNeuralModel();

		bool Load(const char* FP);
		bool Save(const char* FP);

		[[deprecated("Here so that I can implement the ")]]
		void Run(TMap2D* Maps, int MapsN);
		void Run(TF32* Input, TF32* Output);

		struct TFitnessGuider {
			const char* BackupDirPath = "_Backup";
			// 0 for no backup, not recommended.
			// Backup the model every BackupBatchIndex+1 batches.
			// You can suck my dick, you better backup your model.
			TU16 BackupBatchIndex = 5;
			
			// Must be set.
			TU16 BatchesN = 0;
			TU8 BatchSize = 6;

			// 0 to skip and only use MinAvgCost
			TU16 MaxEpochsN = 0;
			// If you wanna manage it yourself set it to 0, 
			float MinAvgCost = 0.1F;

			float Rate = 0.1F;
			bool AutoRate = true;

			struct TEpochState {
				unsigned EpochI = 0;
				TF32 AvgCost;
			};

			// DesiredOutput is after the activation function was applied
			virtual void OnNextSample (TF32* Input, TF32* DesiredOutput) = 0;
			// Your function blocks the fitting, so be aware of that.
			// Returns if it wants to stop training or not, a special backup is made for this automatically incase you fuck up your on your side.
			virtual bool OnEpoch (TEpochState& ES) = 0;
		};

		bool Fit(TFitnessGuider& Guider);

		// LayersNeurons is 0 terminated.
		// Returns the major part of memory needed in MB, the minor part is less than a single MB so no reason to add it.
		static int CalcMemory(int InputUnitsN, TLayer* LayersNeurons);
	};

	class TBrain {
		
	};
	
	extern void SetRngSeed(long long seed);
	extern int Rng(void);
}
