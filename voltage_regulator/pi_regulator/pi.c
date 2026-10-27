#include "pi.h"

float regulate(pi_reg_t* pi, float desired, float real) {
	float err = desired - real;
	float int_part_raw = pi->int_part + err * pi->delta_t;
	float out = pi->kp * err + int_part_raw;
	if ((out >= pi->min_threshold) && (out <= pi->max_threshold)) {
		pi->int_part = int_part_raw;
	} else if (out < pi->min_threshold) {
		if (err > 0) {
			pi->int_part = int_part_raw;
		}
		out = pi->min_threshold;
	} else {
		if (err < 0) {
			pi->int_part = int_part_raw;
		}
		out = pi->max_threshold;
	}

    return out;
}
