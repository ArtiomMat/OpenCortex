#include "time.h"

typedef __int8_t I8;
typedef __int16_t I16;
typedef __int32_t I32;
typedef __int64_t I64;

typedef __uint8_t U8;
typedef __uint16_t U16;
typedef __uint32_t U32;
typedef __uint64_t U64;

struct Neuron {
	
};

struct Time {
	U16 Minute = 0;
	U16 Hour = 0;

	Time(U16 Minute, U16 Hour) {
		this->Minute = Minute;
		this->Hour = Hour;
		Format();
	}

	// Allowed examples: "3" = 3 hours, "1:33", "21:420", ":","" = 0, "000:2", ":33", "1:"
	// In the "21:420", "420" is formatted, so it moves to the hours.
	Time(const char* Str) {
		// Hours
		for (; *Str != ':'; Str++) {
			if (!(*Str))
				return;
			Hour*=10;
			Hour+=(*Str-'0');
		}
		// Now the minutes
		for (; *Str; Str++) {
			if (!(*Str))
				return;
			Minute*=10;
			Minute+=(*Str-'0');
		}
	}

	U32 ToMinutes() {
		return Minute + Hour*60;
	}

	void Format() {
		if (Minute < 60)
			return;
		Hour += Minute/60;
		Minute = Minute%60;
	}

	static Time Percentage(Time& T, float P) {
		unsigned M = T.ToMinutes();
		return Time(M * P, 0);
	}
};

struct Job {
	Time StartT, EndT;
	Job* SubJobs;
	int SubJobsN;
};



