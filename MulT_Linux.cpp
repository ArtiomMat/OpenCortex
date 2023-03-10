#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>

#include "MulT.hpp"

namespace MulT {

	static int Cores;

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

		Cores = atoi(StrCores);

		return true;
	}

	float GetUsage() {
		struct rusage usage;
    	getrusage(RUSAGE_SELF, &usage);

    	printf("CPU Time Used: %ld.%06ld seconds\n",
           usage.ru_utime.tv_sec + usage.ru_stime.tv_sec,
           usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
	}
}


int main() {
	while (1) {
		MulT::GetUsage();
		
	}
	return 0;
}