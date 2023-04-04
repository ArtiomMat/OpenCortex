#pragma once
// Real-time Multi-threading monitoring/managment library.

#include "OpenCortex.hpp"

namespace OpenCortex {
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
		
		TU8 HandlesN = 0;
		TU64* Handles = nullptr;
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
			TU16 Size;
			TU16 First = 0;
			TU8 Full = 0;

			History(TU32 Size) {
				this->Size = Size;
				Arr = new Type [Size];
			}

			~History() {
				delete [] Arr;
			}

			TU16 GetLength() {
				if (Full)
					return Size;
				return First;
			}

			void Clear() {
				First = Full = 0;
			}

			// 1 <= VirtualI < Size
			Type& GetRef(TU32 VirtualI) {
				if (Full)
					VirtualI += First;

				if (VirtualI >= Size)
					return Arr[VirtualI-Size];

				return Arr[VirtualI];
			}

			void Set(TU32 VirtualI, Type Data) {
				GetRef(VirtualI) = Data;
			}

			void Add(Type X) {
				Arr[First++] = X;

				if (First == Size) {
					Full = 1;
					First = 0;
				}
			}

			Type& operator[](TU32 VI) {
				return GetRef(VI);
			}
			History& operator+=(Type X) {
				Add(X);
				return *this;
			}
		};

		struct Digest {
			TU16 UsageDeltaAvg = 0;
			TU16 OptimalN = 0;
		} Digest;


		// History<U16> UsageDeltas = History<U16>(16);
	};
}
