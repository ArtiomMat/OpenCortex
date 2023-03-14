#pragma once
// Real-time Multi-threading monitoring/managment library.

#include "OctoAI.hpp"

namespace OAI {
	// The threader doesn't keep track of the opened threads, if it gets out of scope you fucked up.
	class Threader {
		public:
		typedef void* (*CallBack)(void* Input);
		
		Threader(CallBack CB);
		~Threader();

		void Open(int N, void** Inputs, void** Outputs);
		void Join();

		private:
		CallBack CB;
		struct ThreadCallArg {
			ThreadCallArg(Threader* T, void* Input, void** OutputPtr) {
				this->T = T;
				this->Input = Input;
				this->OutputPtr = OutputPtr;
			}
			Threader* T;
			void* Input;
			void** OutputPtr;
		};
		static void* ThreadCall(void* Arg);

		U16 ThreadsN;
		U16 DoneN;

		int PipeFd[2];
	};
	
	// A monitor is a threader but smarter.
	// It converges to an optimal number of threads for your program.
	class Monitor : public Threader {
		public:
		// The optimal number of threads.
		int OptimalN;
		
		Monitor(CallBack CB);
		~Monitor();

		// Returns in essense, how optimal this->OptimalN is.
		// This Open has no N since it is determined automatically.
		// You can extract this 
		float Open(void** Inputs, void** Outputs);

		private:

		template <typename Type>
		class History {
			public:
			Type* Arr;
			U32 Size;
			U32 First = 0;

			History(U32 Size) {
				this->Size = Size;
				Arr = new Type [Size];
			}

			~History() {
				delete [] Arr;
			}

			// 1 <= VirtualI < Size
			Type& GetRef(U32 VirtualI) {
				VirtualI += First;

				if (VirtualI >= Size)
					return Arr[VirtualI-Size];

				return Arr[VirtualI];
			}

			void Set(U32 VirtualI, Type Data) {
				GetRef(VirtualI) = Data;
			}

			void Add(Type& X) {
				Arr[First++] = X;

				if (First == Size)
					First = 0;
			}

			Type& operator[](U32 VI) {
				return GetRef(VI);
			}
			History& operator+=(Type X) {
				Add(X);
				return *this;
			}
		};

		void Digest();
		void CalcOptimalN();

		struct Summary {
			U16 UsageSlope;
			U16 ThreadsUsed;
		};

		History<Summary> Digests = History<Summary>(6);
		History<U16> PlotsNow = History<U16>(16);
	};
	
}
