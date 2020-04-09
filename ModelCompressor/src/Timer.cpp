#include "prech.h"
#include "Timer.h"

#include <chrono>
using namespace std::chrono;

static steady_clock::time_point start_;
static steady_clock::time_point stop_;

Timer::~Timer()
{
}

void Timer::Start() {
	start_ = high_resolution_clock::now();
}


void Timer::Time(std::string msg, long *time) {
	auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_);
	if (!msg.empty()) {
		std::cout << msg << " : " << duration.count() << " in milliseconds" << std::endl;
	}
	if (time) *time = duration.count();
}