#include <time.h>

#include "OctoAI.hpp"

int main() {
	// OAI::F8::N = 4;
	
	OAI::NeuronsModel::Layer L[] = 
	{
		OAI::NeuronsModel::Layer{2, OAI::LeakyRELU},
		OAI::NeuronsModel::Layer{1, OAI::LeakyRELU},
		OAI::NeuronsModel::NullLayer
	};

	OAI::NeuronsModel M(2, L);
	OAI::F8 Input[] = {2<<1,2<<2};
	OAI::F8 Output[1];
	M.Run(Input, Output);

	printf("%f\n", Output[0].ToFloat());

	return 0;
}
