#pragma once



inline float linear_inter(float goal, float current, float dt) {
	float difference = goal - current;

	if (difference > dt) {
		return current + dt;
	}
	if (difference < -dt) {
		return current - dt;
	}
	return goal;
}


inline float exp_inter(float goal, float current, float dt) {
	float difference = goal - current;

	// constants for moving and stopping - based on feedback
	const float accel_rate = 0.12f; 
	const float decel_rate = 0.08f;

	float alpha = (abs(goal) > abs(current)) ? accel_rate : decel_rate;
	return current + difference * (1.0f - exp(-alpha * (dt * 100.0f)));
}



