#pragma once
#include <cstdint>

class Time {
public:
	void start();
	void tick();

	float deltaSeconds() const { return dt_; }
	double totalSeconds() const { return total_; }

private:
	uint64_t freq_{ 0 };
	uint64_t last_{ 0 };
	float dt_{ 0.0f };
	double total_{ 0.0 };
};
