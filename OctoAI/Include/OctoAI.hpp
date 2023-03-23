#pragma once

#include <stdio.h>

#ifndef __INT64_TYPE__
	#error No __INT64_TYPE__? are you compiling this for a ?
#endif

namespace OAI {
	typedef __int8_t I8;
	typedef __int16_t I16;
	typedef __int32_t I32;
	typedef __int64_t I64;

	typedef __uint8_t U8;
	typedef __uint16_t U16;
	typedef __uint32_t U32;
	typedef __uint64_t U64;

	struct F8 {
		// N is how many 2's F8::Q is divided by to get the actual value of the fixed number, e.g. N=2,Q=3 <-> Value=3/2**2 = 3/4 = 0.75
		// So the smaller N is, the more percise the whole number can be.
		// Average ratio between performance of variable and non variable N is approx. 14/13. The performance difference is minimal, and coming at the advantage of defining your own N.
		static int N;
		I8 Q;

		F8() {}
		inline F8(I8 X) {
			Q = X;
		}
		
		// Do X<<N to get X.0, otherwise X represents the internal quotient.
		// F8& operator=(I8 X);

		inline F8& operator=(I8 X) {
			Q = X;
			return *this;
		}
		
		inline F8 operator+(F8& O) {
			return F8(Q + O.Q);
		}
		inline F8& operator+=(F8 O) {
			Q += O.Q;
			return *this;
		}
		inline F8 operator-(F8& O) {
			return F8(Q - O.Q);
		}
		inline F8& operator-=(F8 O) {
			Q -= O.Q;
			return *this;
		}
				// printf("\n%f*%f=", ToFloat(), O.ToFloat());
			// printf("%f\n", F8((Q * O.Q) >> N).ToFloat());
		inline F8 operator*(F8& O) {
			return F8((Q * O.Q) >> N);
		}
		inline F8& operator*=(F8& O) {
			Q = (Q * O.Q) >> N;
			return *this;
		}
		inline F8 operator/(F8& O) {
			return F8((Q / O.Q) >> N);
		}
		inline F8& operator/=(F8& O) {
			Q = (Q / O.Q) >> N;
			return *this;
		}

		float ToFloat();
	};

	class Map {
		public:

		struct Config {
			U16 Width, Height;
			U16 ChannelsNum;
		};

		U16 Width, Height;
		U8* Data = nullptr;
		U32 ChannelsNum;
		
		// Copy a Map
		Map(Map& other);
		Map(const char* fp);
		Map(U16 width, U16 height, U32 channelsNum);
		// NOTE: Data is used as a pointer, not copied.
		Map(U16 width, U16 height, U32 channelsNum, U8* data);
		
		~Map();

		void SetPixel(U16 x, U16 y, const U8* pixel);
		// Returns actual pixel location, don't fuck with this pointer please.
		U8* GetPixel(U16 x, U16 y);
		void GetPixel(U16 x, U16 y, U8* output);

		void Crop(U16 left, U16 top, U16 right, U16 bottom);
		void Rotate(float radians, U16 aroundX, U16 aroundY);
		void Resize(U16 w, U16 h);

		void ResizeSmooth(U16 w, U16 h);

		bool Load(const char* Fp);
		// Automatically determines file type by extension, if fails, saved as MAP(Custom simple format, should only use if there are channels nothing else supports).
		bool Save(const char* Fp);

		bool LoadMAP(FILE* F);
		bool LoadJPG(FILE* F);
		bool LoadPNG(FILE* F);
		bool SaveMAP(const char* Fp);
		bool SaveJPG(const char* Fp);
		bool SavePNG(const char* Fp);

		protected:
		void Allocate(U16 Width, U16 Height, U32 ChannelsNum);
		void Free();
	};

	class NeuronsModel;
	class FiltersModel;
	
	class Model {
		public:
		virtual bool Load(const char* FP) = 0;
		virtual bool Save(const char* FP) = 0;

		virtual void Run(Map* Maps, int MapsN) = 0;
		virtual void Run(F8* Arr, F8* Output) = 0;
	};

	enum Activation {
		Null,
		RELU,
		LeakyRELU,
		ELU,
		Sigmoid,
		TanH,
	};

	class NeuronsModel : public Model {
		public:
		struct Layer {
			U32 NeuronsN : 29;
			U32 Func : 3;
		};

		const static Layer NullLayer;

		private:
		class RunState {
			public:
			unsigned TWI = 0, TNI = 0;

			U8 FedBufI = 1;
			F8* Bufs[2];

			RunState(int BigLayerNeuronsN);
			~RunState();

			private:
			F8* EntireBuf;
		};

		struct Neuron {
			F8 Bias;
		}* Neurons;

		struct Wire {
			F8 Weight;
		}* Wires;

		unsigned InputUnitsN;
		unsigned BigLayerI;

		Layer* Layers;
		unsigned LayersN;

		void Activate(F8& V, int Func);
		
		void RunLayer(RunState& State, int LI, int PrevNeuronsN);
		void RunChunk(RunState& State, int LI, int FirstI, int LastI, int PrevNeuronsN);
		public:
		// NeuronsModel(const char* FP);
		// Input sources include both raw inputs and model outputs as inputs.
		// L is terminated with NeuronsModel::NullLayer (NeuronsN = 0)
		NeuronsModel(int InputUnitsN, Layer* L);
		~NeuronsModel();

		// bool Load(const char* FP);
		// bool Save(const char* FP);

		void Run(Map* Maps, int MapsN);
		void Run(F8* Input, F8* Output);

		struct FitnessGuider {
			const char* BackupDir = "_Backup";
			// 0 for no backup, not recommended.
			// Backup the model every BackupInterval batches.
			U16 BackupInterval = 5;
			U16 BatchSize;

			// 0 to skip and only use MinAvgCost
			U16 MaxEpochsN = 0;
			float MinAvgCost = 0.1F;

			float Rate = 0.1F;
			bool AutoRate = true;

			virtual void GetNextSample(F8* Input, F8* Output) = 0;
		};

		void Fit(FitnessGuider& Guider);

		// LayersNeurons is 0 terminated.
		// Returns the major part of memory needed in MB, the minor part is less than a single MB so no reason to add it.
		static int CalcMemory(int InputUnitsN, U16* LayersNeurons);
	};

	class Brain {
		
	};

	extern void SetRngSeed(long long seed);
	extern int Rng(void);
}

