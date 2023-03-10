#pragma once
// Multi-threading monitoring/managment library.

namespace MulT {

	bool Setup();
	// To automatically determine cores use Setup()
	void Setup(int Cores);
	// Value between 0 and 1
	// On linux can be greater, since some processes may be waiting.
	float GetUsage();
}
