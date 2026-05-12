#ifndef __TYPE_EXTEND_H
#define __TYPE_EXTEND_H
#include "stm32f10x.h"
// 向量结构体，可以直观表示OLED颜色翻转区域
typedef struct vector{
	int16_t x;
	int16_t y;
} Vector;

// 浮点数向量结构体
typedef struct vectorf{
	float x;
	float y;
} VectorF;

Vector TypeExtend_vector_add_vector(Vector v1, Vector v2);
Vector TypeExtend_vector_sub_vector(Vector v1, Vector v2);
#endif
