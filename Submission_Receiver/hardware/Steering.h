#ifndef __STEERING_H__
#define __STEERING_H__
#include "global.h"
#include "TypeExtend.h"

void steer_init(void);
void steer_vector_add(Vector vector);
void steer_vector_sub(Vector vector);
void steer_set_vector(Vector vector);
void steer_check_bluetooth_signal(void);

#endif
