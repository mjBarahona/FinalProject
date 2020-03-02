#pragma once

class Timer
{
public:
	Timer() = delete;
	~Timer();
	static void Start();
	
	// Pass it to required unit. Predefined units 
	// are nanoseconds, microseconds, milliseconds, 
	// seconds, minutes, hours.

	static void Time(std::string msg);
private:
};

