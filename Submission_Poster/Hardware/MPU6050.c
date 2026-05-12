#include "MPU6050.h"
#include "MyI2C.h"
#include <math.h>
#include "MPU6050_Reg.h"

MPU6050_Data mpu;					//全局输出结构体，ISR 写入，主循环只读

static int16_t AX, AY, AZ, GX, GY, GZ;	//读取MPU6050的原始数据，ISR内部使用
static float AngleGyro,AngleGyro_1;		//由陀螺仪积分得到的角度值，ISR内部使用

#define MPU6050_ADDRESS		0xD0		//MPU6050的I2C从机地址

/**
  * 函    数：MPU6050写寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(Data);				//发送要写入寄存器的数据
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_Stop();						//I2C终止
}

/**
  * 函    数：MPU6050读寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 返 回 值：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;
	
	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答
	
	MyI2C_Start();						//I2C重复起始
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	MyI2C_ReceiveAck();					//接收应答
	Data = MyI2C_ReceiveByte();			//接收指定寄存器的数据
	MyI2C_SendAck(1);					//发送应答，给从机非应答，终止从机的数据输出
	MyI2C_Stop();						//I2C终止
	
	return Data;
}

/**
  * 函    数：MPU6050连续读多个寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 参    数：DataArray 输出参数，用于存储多个寄存器值的数组
  * 参    数：Count 指定读取寄存器的数量
  * 返 回 值：无
  */
void MPU6050_ReadRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count)
{
	uint8_t i;
	
	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答
	
	MyI2C_Start();						//I2C重复起始
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	MyI2C_ReceiveAck();					//接收应答
	for (i = 0; i < Count; i ++)		//循环Count次，连续读取多个字节，MPU6050内部地址指针会自动自增
	{
		DataArray[i] = MyI2C_ReceiveByte();	//接收指定寄存器的数据，存入数组第i个数据
		if (i < Count - 1)				//未读取到最后一个字节
		{
			MyI2C_SendAck(0);			//正常发送应答，从机后续会继续输出下一个字节数据
		}
		else							//读取到了最后一个字节
		{
			MyI2C_SendAck(1);			//给非应答，从机后续将不会输出数据，主机收回总线控制权
		}
	}
	MyI2C_Stop();						//I2C终止
}

/**
  * 函    数：MPU6050初始化
  * 参    数：无
  * 返 回 值：无
  */
void MPU6050_Init(void)
{
	MyI2C_Init();									//先初始化底层的I2C
	
	/*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);		//采样率分频寄存器，配置采样率
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			//配置寄存器，配置DLPF
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}

/**
  * 函    数：MPU6050获取ID号
  * 参    数：无
  * 返 回 值：MPU6050的ID号
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		//返回WHO_AM_I寄存器的值
}

/**
  * 函    数：MPU6050获取数据（旧版）
  * 参    数：AccX AccY AccZ 加速度计X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 参    数：GyroX GyroY GyroZ 陀螺仪X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 返 回 值：无
  */
//void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
//						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
//{
//	uint8_t DataH, DataL;								//定义数据高8位和低8位的变量
//	
//	/*每个数据都通过MPU6050_ReadReg读取单个字节实现，效率不高*/
//	DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);		//读取加速度计X轴的高8位数据
//	DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);		//读取加速度计X轴的低8位数据
//	*AccX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);		//读取加速度计Y轴的高8位数据
//	DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);		//读取加速度计Y轴的低8位数据
//	*AccY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);		//读取加速度计Z轴的高8位数据
//	DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);		//读取加速度计Z轴的低8位数据
//	*AccZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);		//读取陀螺仪X轴的高8位数据
//	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);		//读取陀螺仪X轴的低8位数据
//	*GyroX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);		//读取陀螺仪Y轴的高8位数据
//	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);		//读取陀螺仪Y轴的低8位数据
//	*GyroY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);		//读取陀螺仪Z轴的高8位数据
//	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);		//读取陀螺仪Z轴的低8位数据
//	*GyroZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//}

/**
  * 函    数：MPU6050获取数据（新版）
  * 参    数：AccX AccY AccZ 加速度计X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 参    数：GyroX GyroY GyroZ 陀螺仪X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 返 回 值：无
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t Data[14];
	
	/*通过MPU6050_ReadRegs连续读取多个数据，效率更高*/
	MPU6050_ReadRegs(MPU6050_ACCEL_XOUT_H, Data, 14);	//从MPU6050_ACCEL_XOUT_H开始，连续读取14个字节，存入Data数组里
	
	/*数据拼接，通过输出参数返回*/
	*AccX = (Data[0] << 8) | Data[1];		//Data[0]和Data[1]为加速度计X轴数据
	*AccY = (Data[2] << 8) | Data[3];		//Data[2]和Data[3]为加速度计Y轴数据
	*AccZ = (Data[4] << 8) | Data[5];		//Data[4]和Data[5]为加速度计Z轴数据
	
											//Data[6]和Data[7]为温度数据，此处暂时不用
	
	*GyroX = (Data[8] << 8) | Data[9];		//Data[8]和Data[9]为陀螺仪X轴数据
	*GyroY = (Data[10] << 8) | Data[11];	//Data[10]和Data[11]为陀螺仪Y轴数据
	*GyroZ = (Data[12] << 8) | Data[13];	//Data[12]和Data[13]为陀螺仪Z轴数据
}


void TIM1_UP_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		/*定时中断函数1ms自动执行一次*/

		/*进入中断函数后，立刻清标志位*/
		/*如果中断函数退出前，标志位又置1了，说明中断函数执行时间超过了定时时间（1ms）*/
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

		/*在中断里读取MPU6050，可以保证读取间隔严格为1ms*/
		/*但要保证MPU6050_GetData执行时间不超过1ms*/
		MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);

		/*校准陀螺仪Y轴零漂*/
		/*此值需实测确定，不同的设备零漂一般不同*/
		/*实测方法是，在完全静止时，观察OLED显示的GY值，即为零漂值*/
		/*然后在此处将零漂值减去，使得完全静止时，GY值为0*/
		GY -= 40;
		GX += 15;

		/*由加速度计计算得到角度值*/
		/*atan2计算反正切，得到角度（弧度制）， / 3.14159 * 180可将弧度制转为角度值*/
		mpu.AngleAcc = -atan2(AX, AZ) / 3.14159 * 180;
		mpu.AngleAcc_1 = atan2(AY,AZ) / 3.14159 * 180;

		/*由陀螺仪积分得到角度值*/
		/*互补滤波下，角度积分要在上次滤波后的Angle上进行*/
		/*公式中32768是int16_t变量的最大值，2000是陀螺仪配置的满量程2000度每秒，0.001是定时时间1ms*/
		AngleGyro = mpu.Pitch + GY / 32768.0 * 2000 * 0.001;
		AngleGyro_1 = mpu.Roll + GX / 32768.0 * 2000 * 0.001;

		/*执行互补滤波*/
		float Alpha = 0.001;		//互补滤波系数，值越大，越偏向于加速度计，值越小，越偏向于陀螺仪
		mpu.Pitch = Alpha * mpu.AngleAcc + (1 - Alpha) * AngleGyro;		//互补滤波计算，得到稳定的角度值
		mpu.Roll = Alpha * mpu.AngleAcc_1 + (1 - Alpha) * AngleGyro_1;		//互补滤波计算，得到稳定的角度值

		/*中断函数退出前，再次检查标志位*/
		if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
		{
			/*标志位又置1了，说明中断函数执行时间超过了定时时间（1ms）*/
			/*置TimerErrorFlag为1，表示定时中断错误*/
			mpu.TimerErrorFlag = 1;

			/*清标志位，避免中断连续触发，导致主函数完全无法执行*/
			TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		}

		/*中断函数退出前，读取计数器的值，此值可用于测量中断函数的具体执行时间*/
		mpu.TimerCount = TIM_GetCounter(TIM1);
	}
}


