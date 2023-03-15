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
		HandlesN = CoresN;
	}

	Threader::~Threader() {
		for (int I = 0; I < HandlesN; I++)
			pthread_cancel((pthread_t)Handles[I]);
	}

	void* Threader::ThreadCall(void* _Arg) {
		ThreadCallArg* Arg = (ThreadCallArg*)_Arg;

		void* Output = Arg->T->CB(Arg->Input);

		if (Arg->OutputPtr)
			*Arg->OutputPtr = Output;

		// Cleanup
		delete Arg;

		return nullptr;
	}

	U64 Threader::OpenSingle(void*(*Call)(void* Arg), void* _Input, void** OutputPtr) {
		pthread_t Handle;
		
		ThreadCallArg* Input = new ThreadCallArg(this, _Input, OutputPtr);
		pthread_create(&Handle, nullptr, Call, Input);
		
		return (U64)Handle;
	}

	void Threader::Open(int HandlesN, void** Inputs, void** Outputs) {
		if (this->HandlesN != HandlesN) {
			this->HandlesN = HandlesN;
			Handles = new U64 [HandlesN];
		}

		for (int I = 0; I < HandlesN; I++) {
			Handles[I] = OpenSingle(ThreadCall, Inputs[I], Outputs+I);
		}
	}

	void Threader::Join() {
		// TODO: Maybe refactor so that instead of nullptr Outputs[I] is there.
		for (int I = 0; I < HandlesN; I++)
			pthread_join((pthread_t)Handles[I], nullptr);
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
		U64 StartUsage = GetUsageTime();
		Threader::OpenSingle(ThreadCall, Inputs[0], Outputs);
	}


}

void* Func(void* Input) {
	int x = 0;
	int y = 100;
	while (x < y)
		x++;
	puts("Done.");
	fflush(stdout);
	
	return (void*)1;
}

int main() {
	OAI::Threader T(Func);
	void* Inputs[] = {nullptr,nullptr,nullptr};
	T.Open(3, Inputs, Inputs);
	T.Join();
	return 0;
}