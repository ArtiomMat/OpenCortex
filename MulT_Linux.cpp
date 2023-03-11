#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdint.h>

#include "MulT.hpp"

namespace MulT {
	typedef uint32_t U32;
	typedef uint16_t U16;
	typedef uint8_t U8;

	static int OptimalN;
	static int ThreadsN;

	static struct _Graph {
		static const int PlotsN = 16; // TODO: Make it aligned

		U8 PlotIndex;
		U8 PlotsFull; // If 1 then it means after PlotIndex we have the "oldest" plot, and thats how we are supposed to read Plots[]. Otherwise Plots[0] is the oldest plot.
		U16 Plots[PlotsN];
		U32 MeasureTick;
	} Graph;
	static int CoresN;

	static void Thread() {
		
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

	float GetUsage() {
		struct rusage Usage;
    	getrusage(RUSAGE_SELF, &Usage);

    	printf("CPU Time Used: %ld.%06ld seconds\n",
           Usage.ru_utime.tv_sec + Usage.ru_stime.tv_sec,
           Usage.ru_utime.tv_usec + Usage.ru_stime.tv_usec);
	}
}


int main() {
	while (1) {
		MulT::GetUsage();
		
	}
	return 0;
}