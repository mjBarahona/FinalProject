#pragma once

class Timer
{
public:
	Timer() = delete;
	~Timer();
	static void Start();
	

	static void Time(std::string msg, long* time = nullptr);
private:
};

