#pragma once
// Real-time Multi-threading monitoring/managment library.

#include "OctoAI.hpp"

namespace OAI {
	// If Threader gets out of scope it closes all threads it opened.
	class Threader {
		public:
		typedef void* (*CallBack)(void* Input);
		
		Threader(CallBack CB);
		~Threader();

		void Open(int ThreadsN, void** Inputs, void** Outputs);
		void Join();

		// protected:
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
		static void ThreadCall(ThreadCallArg* Arg);
		void AllocHandles(int N);
		
		U8 HandlesN = 0;
		U64* Handles = nullptr;
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
			U16 Size;
			U16 First = 0;
			U8 Full = 0;

			History(U32 Size) {
				this->Size = Size;
				Arr = new Type [Size];
			}

			~History() {
				delete [] Arr;
			}

			U16 GetLength() {
				if (Full)
					return Size;
				return First;
			}

			void Clear() {
				First = Full = 0;
			}

			// 1 <= VirtualI < Size
			Type& GetRef(U32 VirtualI) {
				if (Full)
					VirtualI += First;

				if (VirtualI >= Size)
					return Arr[VirtualI-Size];

				return Arr[VirtualI];
			}

			void Set(U32 VirtualI, Type Data) {
				GetRef(VirtualI) = Data;
			}

			void Add(Type X) {
				Arr[First++] = X;

				if (First == Size) {
					Full = 1;
					First = 0;
				}
			}

			Type& operator[](U32 VI) {
				return GetRef(VI);
			}
			History& operator+=(Type X) {
				Add(X);
				return *this;
			}
		};

		struct Digest {
			U16 UsageSlope;
			U16 ThreadsUsed;
		} LastDigest;

		// // Index != 0, assumes it is the last thread.
		// struct ExThreadCallArg : ThreadCallArg {
		// 	ExThreadCallArg(Threader* T, void* Input, void** OutputPtr, int Index=-1) : ThreadCallArg(T, Input, OutputPtr) {
		// 		this->Index = Index;
		// 	}
		// 	int Index;
		// };

		// static void* ThreadCall(void* Arg);

		// History<Digest> Digests = History<Digest>(6);
		History<U16> UsagePlots = History<U16>(16);
	};
}
