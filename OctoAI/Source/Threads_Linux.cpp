// STD
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// System shit
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

#include "Threads.hpp"
#include "OctoAI.hpp"

namespace OAI {
	static int CoresN = 0;

	bool Setup() {
		FILE* F = fopen("/proc/cpuinfo", "r");

		// Searching for the cores thingy
		// We are looking for "cpu cores	: 4 or whatever"
		const char Searched[] = "cores";

		U8 C, SI;
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

			unsigned I;
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
		// FIXME: Leak, if we cancel too early not all threads will deallocate their Args D:
		for (int I = 0; I < HandlesN; I++)
			pthread_cancel((pthread_t)Handles[I]);
		
		delete [] Handles;
	}

	void* Threader::ThreadCall(void* _Arg) {
		ThreadCallArg* ArgPtr = (ThreadCallArg*)_Arg;
		ThreadCall(ArgPtr);
		return nullptr;
	}

	void Threader::ThreadCall(ThreadCallArg* ArgPtr) {
		void* Output = ArgPtr->T->CB(ArgPtr->Input);

		if (ArgPtr->OutputPtr)
			*ArgPtr->OutputPtr = Output;

		// Cleanup
		delete ArgPtr;
	}

	void Threader::AllocHandles(int N) {
		delete [] Handles;
		if (HandlesN != N) {
			HandlesN = N;
			Handles = new U64 [HandlesN];
		}
	}

	void Threader::Open(int HandlesN, void** Inputs, void** Outputs) {
		AllocHandles(HandlesN);

		for (int I = 0; I < HandlesN; I++) {
			ThreadCallArg* Input = new ThreadCallArg(this, Inputs[I], Outputs+I);
			pthread_create((pthread_t*)(Handles+I), nullptr, ThreadCall, Input);
		}
	}

	void Threader::Join() {
		// TODO: Maybe refactor so that instead of nullptr Outputs[I] is there.
		for (int I = 0; I < HandlesN; I++) {
			pthread_join((pthread_t)Handles[I], nullptr);
			puts("OK");
			fflush(stdout);
		}
			puts("DONE");
			fflush(stdout);
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

	Monitor::~Monitor() {
		Threader::~Threader();
	}

	float Monitor::Open(void** Inputs, void** Outputs) {
		static const int MaxLogsN = 4;
		static int LogsN = 0;
		static U16 UsageDeltaAvg = 0;

		U64 StartUsage = GetUsageTime();
		Threader::Open(OptimalN, Inputs, Outputs);
		Join();
		U64 EndUsage = GetUsageTime();

		UsageDeltaAvg += EndUsage - StartUsage;

		// We begin the calculation
		if (LogsN >= MaxLogsN) {
			// Finilize the average
			UsageDeltaAvg /= MaxLogsN;

			// If Digest.OptimalN is 0 we just started. We want to only compare against actual statistics.
			if (Digest.OptimalN) {
				int Boost = Digest.UsageDeltaAvg - UsageDeltaAvg;
				// The boost needs to be proportional to the difference in threads used
				Boost /= (OptimalN - Digest.OptimalN);
			}
			
			Digest.UsageDeltaAvg = UsageDeltaAvg;
			Digest.OptimalN = OptimalN;

			UsageDeltaAvg = LogsN = 0;
		}

		return 1.0F;
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
	OAI::Monitor T(Func);
	void* Inputs[] = {nullptr,nullptr,nullptr};
	T.Open(Inputs, Inputs);
	T.Join();
	return 0;
}