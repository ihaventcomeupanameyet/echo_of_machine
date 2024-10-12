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





