#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdint.h>

#include <vector>

#include "MulT.hpp"

namespace MulT {
	typedef uint64_t U64;
	typedef uint32_t U32;
	typedef uint16_t U16;
	typedef uint8_t U8;


	static struct {
		static const int PlotsN = 16; // TODO: Make it aligned

		U8 PlotIndex;
		U8 PlotsFull; // If 1 then it means after PlotIndex we have the "oldest" plot, and thats how we are supposed to read Plots[]. Otherwise Plots[0] is the oldest plot.
		U64 LastUsage;
		U32 Plots[PlotsN];
	} Graph;

	struct GroupState {
		struct ThreadState {
			void* Return = nullptr; // nullptr for no return
		};

		std::vector<ThreadState> Threads;
	};

	int OptimalN;

	static int CoresN;
	static int ThreadsN;

	GroupI AddGroup(CallBack CB) {
		
	}
	
	ThreadI OpenThread(GroupI G, void* Input) {

	}

	void* GetThreadOutput(ThreadI Index) {

	}

	bool Setup() {
		FILE* F = fopen("/proc/cpuinfo", "r");

		// Searching for the cores thingy
		// We are looking for "cpu cores	: 4 or whatever"
		const char Searched[] = "cores";
		
		char C, SI;

		while ((C = fgetc(F)) != EOF) {
			// If we found the first letter we loop and check if Searched is here
			if (C == Searched[0]) {
				bool Found = true;
				
				for (SI = 1; SI < sizeof(Searched)-1; SI++) {
					C = fgetc(F);
					if (C != Searched[SI]) {
						Found = false;
						break;
					}
				}

				if (Found)
					goto _TotallyFound;
			}
		}

		return false;

		_TotallyFound:
		char StrCores[4] = {0,0,0,0};

		// Wait for numbers
		do {
			C = getc(F);
		} while (C < '0' || C > '9');

		int I;
		for (I = 0; I < sizeof(StrCores)-1; I++) {
			if (C < '0' || C > '9')
				break;

			StrCores[I] = C;

			C = getc(F);
		}

		StrCores[I] = 0;

		CoresN = atoi(StrCores);

		return true;
	}

	U64 GetUsage() {
		struct rusage Usage;
		getrusage(RUSAGE_SELF, &Usage);

		U64 Ret = Usage.ru_utime.tv_sec + Usage.ru_stime.tv_sec;
		Ret *= 1000000;
		Ret += Usage.ru_utime.tv_usec + Usage.ru_stime.tv_usec;
		
		return Ret;
	}
}


int main() {
	while (1) {
		MulT::GetUsage();
	}
	return 0;
}