// STD
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// System shit
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

#include "OctoAI_Threads.hpp"
#include "OctoAI.hpp"

namespace OAI {
	static int CoresN = 0;

	bool Setup() {
		FILE* F = fopen("/proc/cpuinfo", "r");

		// Searching for the cores thingy
		// We are looking for "cpu cores	: 4 or whatever"
		const char Searched[] = "cores";

		char C, SI;
		bool TotallyFound = false;
		
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

				if (Found) {
					TotallyFound = true;
					break;
				}
			}
		}

		if (TotallyFound) {
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
		}
		else
			CoresN = 2;

		fclose(F);
		return TotallyFound;
	}

	U64 GetUsageTime() {
		struct rusage Usage;
		getrusage(RUSAGE_SELF, &Usage);

		U64 Ret = Usage.ru_utime.tv_sec + Usage.ru_stime.tv_sec;
		Ret *= 1000000;
		Ret += Usage.ru_utime.tv_usec + Usage.ru_stime.tv_usec;

		return Ret;
	}

	Threader::Threader(CallBack CB) {
		this->CB = CB;
		ThreadsN = CoresN;
		// TODO: Could use mutexes and conds instead
		if (pipe(PipeFd))
			puts("Threader::Threader(): Failed to create pipe.");
	}

	Threader::~Threader() {
		close(PipeFd[0]);
		close(PipeFd[1]);
	}

	void* Threader::ThreadCall(void* _Arg) {
		ThreadCallArg* Arg = (ThreadCallArg*)_Arg;

		void* Output = Arg->T->CB(Arg->Input);

		// Signal that we finished
		char B;
		puts("Write");
		fflush(stdout);
		write(Arg->T->PipeFd[1], &B, 1);
		// Arg->T->DoneN++;

		if (Arg->OutputPtr)
			*Arg->OutputPtr = Output;

		// Cleanup
		delete Arg;

		return nullptr;
	}

	void Threader::Open(int ThreadsN, void** Inputs, void** Outputs) {
		DoneN = 0;
		this->ThreadsN = ThreadsN;
		pthread_t PT;
		for (int I = 0; I < ThreadsN; I++) {
			ThreadCallArg* Input = new ThreadCallArg(this, Inputs[I], &Outputs[I]);
			pthread_create(&PT, nullptr, ThreadCall, Input);
		}
	}

	void Threader::Join() {
		char B;
		for (int I = 0; I < ThreadsN; I++) {
			read(PipeFd[0], &B, 1);
			puts("Read");
		}
	}

	/*
	NOTE TO SELF:
	Logs the CPU usage after last log and may perform some calculations.
	If you didn't already log "last log" is just since Setup().
	Returns weather or not OptimalN changed.

	!!!RULES OF THUMB!!!
	TODO: Remove this and just fatalize program if all threads didn't stop
	- Run Log after all threads stopped, you can do that with WaitFor(). Calculations assume all your measured threads died at the time of the log.

	- Do not Log in multiple places, a log is meant to happen once every tick/frame or whatever your metric is, every cycle.

	- Log either in the beginning of the cycle or it's end, explained below.

	- Log is not instant, it doesn't actually just log your CPU usage, it also may attempt to determine the next optimal number of threads possible, which could is a little for loop over the logs and a calculation. Watch out for the updates in OptimalN.
	*/
	void Digest() {

	}

	Monitor::Monitor(CallBack CB) : Threader(CB) {
		if (!CoresN)
			Setup();
		
		OptimalN = CoresN;
	}

	float Monitor::Open(void** Inputs, void** Outputs) {
		Threader::Open(1, Inputs, Outputs);
	}
}

void* Func(void* Input) {
	int x = 0;
	int y = 1000000000;
	while (x < y)
		x++;
	
	return (void*)1;
}

int main() {
	OAI::Threader T(Func);
	void* Inputs[] = {nullptr,nullptr,nullptr};
	T.Open(3, Inputs, Inputs);
	puts("Done.");
	return 0;

}