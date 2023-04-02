#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../Include/OctoAI.hpp"

namespace OAI {
	bool CheckFileExists(const char* FP) {
		return access(FP, F_OK) == 0;
	}

	bool CheckDirExists(const char* DP) {
		DIR* Dir = opendir(DP);
		if (!Dir)
			return false;
		return true;
	}

	bool CreateDir(const char* DP) {
		return !mkdir(DP, S_IRWXU | S_IRWXG | S_IRWXO); // or just ', 0777' 
	}

	int CheckDirFilled(const char* DP) {
		DIR* Dir = opendir(DP);
		
		if (!Dir)
			return -1;

		struct dirent *DirEnt;

		bool Filled = 0;

		// It has "." and ".." in it so skip 2 first.
		for (int I = 0; (DirEnt = readdir(Dir)); I++) {
			if (I > 1) {
				Filled = 1;
				break;
			}
		}

		closedir(Dir);
		
		return Filled;
	}
}
