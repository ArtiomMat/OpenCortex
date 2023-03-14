#pragma once

#include <stdio.h>

#ifndef __INT64_TYPE__
	#error No __INT64_TYPE__? are you compiling this for a ?
#endif

namespace OAI {
	typedef char I8;
	typedef short I16;
	typedef int I32;
	typedef long long I64;

	typedef unsigned char U8;
	typedef unsigned short U16;
	typedef unsigned int U32;
	typedef unsigned long long U64;

	struct SFixed8 {
		// N is how many 2's SFixed8::Q is divided by to get the actual value of the fixed number, e.g. N=2,Q=3 <-> Value=3/2**2 = 3/4 = 0.75
		// So the smaller N is, the more percise the whole number can be.
		// Average ratio between performance of variable and non variable N is approx. 14/13. The performance difference is minimal, and coming at the advantage of defining your own N.
		static int N;
		I8 Q;

		SFixed8() {}
		SFixed8(I8 X);
		
		SFixed8& operator=(I8 X);

		SFixed8 operator+(SFixed8& O);
		SFixed8& operator+=(SFixed8 O);
		
		SFixed8 operator*(SFixed8& O);
		SFixed8& operator*=(SFixed8& O);

		float ToFloat();
	};

	typedef SFixed8 DataUnit;

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
		virtual void Run(Map* Maps, int MapsN) = 0;
		virtual void Run(U8* Arr, SFixed8* Output) = 0;
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

			char FedBufI = 1;
			SFixed8* Bufs[2];

			RunState(int BigLayerNeuronsN);
			~RunState();

			private:
			SFixed8* EntireBuf;
		};

		struct Neuron {
			SFixed8 Bias;
		}* Neurons;

		struct Wire {
			SFixed8 Weight;
		}* Wires;

		int InputUnitsN;
		int BigLayerI;

		Layer* Layers;
		int LayersN;

		void Activate(SFixed8& V, int Func);
		
		void RunLayer(RunState& State, int LI, int PrevNeuronsN);
		void RunChunk(RunState& State, int LI, int FirstI, int LastI, int PrevNeuronsN);
		public:
		// Input sources include both raw inputs and model outputs as inputs.
		// L is terminated with NeuronsModel::NullLayer (NeuronsN = 0)
		NeuronsModel(int InputUnitsN, Layer* L);
		~NeuronsModel();

		void Run(Map* Maps, int MapsN);
		void Run(U8* Arr, SFixed8* Output);

		void Fit(Map* Maps, int MapsN);
		void Fit(U8* Arr);

		// LayersNeurons is 0 terminated.
		// Returns the major part of memory needed in MB, the minor part is less than a single MB so no reason to add it.
		static int CalcMemory(int InputUnitsN, U16* LayersNeurons);
	};

	class Brain {
		
	};

	void SetRngSeed(long long seed);
	int Rng(void);
}

