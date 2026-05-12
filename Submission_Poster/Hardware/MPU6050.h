#ifndef __MPU6050_H
#define __MPU6050_H
#include "global.h"
/* MPU6050 计算结果结构体，ISR 写入，主循环只读 */
typedef struct {
	uint8_t  TimerErrorFlag;
	uint16_t TimerCount;
	float    AngleAcc;
	float    AngleAcc_1;
	float    Pitch;
	float    Roll;
} MPU6050_Data;

// 暴露地址
extern MPU6050_Data mpu;

void MPU6050_Init(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);

#endif
