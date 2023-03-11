#pragma once

// Real-time Multi-threading monitoring/managment library.

namespace MulT {
	typedef void* (*CallBack)(void* Input);
	typedef unsigned char GroupI;

	typedef unsigned short ThreadI;

	// Toptimal amount of threads possible, This is what makes MulT the GOAT.
	// At first it's definately better than one thread, but MAY be worse than the real optimal, over time it improves.
	extern int OptimalN;

	bool Setup();
	// Wait for a specific group of threads to all return.
	void WaitFor(GroupI G);
	// Wait for a specific thread to return.
	void WaitFor(ThreadI G);
	/*
	Logs the CPU usage after last log and may perform some calculations.
	If you didn't already log "last log" is just since Setup().
	Returns weather or not OptimalN changed.

	!!!RULES OF THUMB!!!
	- Run Log after all threads stopped, you can do that with WaitFor(). Calculations assume all your measured threads died at the time of the log.

	- Do not Log in multiple places, a log is meant to happen once every tick/frame or whatever your metric is, every cycle.
	
	- Log either in the beginning of the cycle or it's end, explained below.
	
	- Log is not instant, it doesn't actually just log your CPU usage, it also may attempt to determine the next optimal number of threads possible, which could is a little for loop over the logs and a calculation. Watch out for the updates in OptimalN.
	*/
	bool Log();

	GroupI AddGroup(CallBack CB);

	ThreadI OpenThread(GroupI G, void* Input);
	void* GetThreadOutput(ThreadI Index);
}
