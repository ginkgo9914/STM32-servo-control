#ifndef __TYPE_EXTEND_H
#define __TYPE_EXTEND_H
#include "stm32f10x.h"
// 向量结构体 (int16_t x, int16_t y)
typedef struct vector{
	int16_t x;
	int16_t y;
} Vector;

// 浮点数向量结构体 (float x, float y)
typedef struct vectorf{
	float x;
	float y;
} VectorF;

Vector TypeExtend_vector_add_vector(Vector v1, Vector v2);
Vector TypeExtend_vector_sub_vector(Vector v1, Vector v2);
#endif
