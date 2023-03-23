#include <time.h>

#include "OctoAI.hpp"

int main() {
	// OAI::F8::N = 4;
	
	/*OAI::NeuronsModel::Layer L[] = 
	{
		OAI::NeuronsModel::Layer{2, OAI::LeakyRELU},
		OAI::NeuronsModel::Layer{1, OAI::LeakyRELU},
		OAI::NeuronsModel::NullLayer
	};

	OAI::NeuronsModel M(2, L);
	OAI::F8 Input[] = {2<<1,2<<2};
	OAI::F8 Output[1];
	M.Run(Input, Output);*/

	// _Float16 A = 2, B = 1.55;

	OAI::F8 A = -1<<(OAI::F8::N+1), B = 2<<(OAI::F8::N);

	printf("%f\n", (A*B).ToFloat());

	return 0;
}
