#pragma once

// Real-time Multi-threading monitoring/managment library.

namespace MulT {
	class Monitor {
		public:
		typedef void* (*CallBack)(void* Input);
		
		int OptimalN;
		
		Monitor(CallBack CB, bool AutoLog);
		~Monitor();

		// If you don't want outputs make it 0
		void Open(int Num, void** Inputs, void** Outputs);
		/*
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
		void Log();
		void Join();

		private:
		bool AutoLog;
	};
	
}
