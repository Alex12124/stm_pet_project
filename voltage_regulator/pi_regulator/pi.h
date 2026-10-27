#pragma once

typedef struct {
	float kp;
	float ki;
	float int_part;
	float delta_t;
	float max_threshold;
	float min_threshold;
} pi_reg_t;


float regulate(pi_reg_t* pi, float desired, float real);
