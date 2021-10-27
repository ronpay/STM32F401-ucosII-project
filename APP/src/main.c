#include "ucos_ii.h"
#include "stm32f4xx.h"

#include "hm10.h"
#include "hmc5883l.h"
#include "motor.h"
#include "mpu6050.h"
#include "oled.h"
#include "receiver.h"
#include "sysTick.h"
#include "inv_mpu.h"
#include "data_transfer.h"

OS_EVENT *SensorSem;
OS_EVENT *ReceiverSem;

extern u8 hm_flag;
double Duty[6];

//GY86 所需要的数组
float Acel_g[3];
short Acel[3];
float Gyro_g[3];
short Gyro[3];
float Temp;
short Me[3];
float pitch, roll, yaw;

#define INIT_TASK_PRIO 5
#define HM10_TASK_PRIO 30
#define RECEIVER_TASK_PRIO 15
#define GY86_TASK_PRIO 7
#define MOTOR_TASK_PRIO 10
#define OLED_TASK_PRIO 25
#define TEST_TASK_PRIO 62
#define DATA_TRANSFER_TASK_PRIO 62

#define INIT_STK_SIZE 128
#define HM10_STK_SIZE 128
#define RECEIVER_STK_SIZE 128
#define GY86_STK_SIZE 128
#define MOTOR_STK_SIZE 128
#define OLED_STK_SIZE 128
#define TEST_STK_SIZE 128
#define DATA_TRANSFER_STK_SIZE 128

OS_STK INIT_TASK_STK[INIT_STK_SIZE];
OS_STK HM10_TASK_STK[HM10_STK_SIZE];
OS_STK RECEIVER_TASK_STK[RECEIVER_STK_SIZE];
OS_STK GY86_TASK_STK[GY86_STK_SIZE];
OS_STK MOTOR_TASK_STK[MOTOR_STK_SIZE];
OS_STK OLED_TASK_STK[OLED_STK_SIZE];
OS_STK TEST_TASK_STK[TEST_STK_SIZE];
OS_STK DATA_TRANSFER_TASK_STK[DATA_TRANSFER_STK_SIZE];

void INIT_TASK(void *pdata){
	HM10_Config();
	
	OLED_Config();
    OLED_Init();
	
	OLED_CLS();
    OLED_ShowStr(0, 4, (unsigned char*)"Loding........", 2);
    Delay_s(1);
//  OLED_CLS();
	
//	Motor_Config();
//	Motor_Unlock();
	
//	Receiver_Config();
	
	MPU6050_Config();
	
//    MPU6050_Init();
	Delay_s(2);
    int ret=mpu_dmp_init();
	if(ret!=0){
		printf("ret:%d\n",ret);
	}
	Delay_s(1);
	MPU_HMC_Init();
	OLED_CLS();
	//GY86_Init();

    // OSTaskDel(INIT_TASK_PRIO);
}

void DATA_TRANSFER_TASK(void *pdata){
    INT8U err;
    while(1){
		ANO_DT_Send_Status(roll,pitch,yaw,0);
		ANO_DT_Send_Senser(Acel_g[0],Acel_g[1],Acel_g[2],Gyro_g[0],Gyro_g[1],Gyro_g[2],0);
		ANO_DT_Send_Senser2(Me[0],Me[1],Me[2],0,0,0,0);
	
		OSTimeDly(30);
    }
}

//void HM10_TASK(void *pdata){
//	INT8U err;
//	while(1){
//		if(hm_flag == '0'){
//			OSSemPend(SensorSem,1000,&err);
//			
//            printf("Euler angles: %3f %3f %3f\n", pitch, roll, yaw);
//			printf("Acceleration: %8f%8f%8f\n", Acel_g[0], Acel_g[1], Acel_g[2]);
//			printf("Gyroscope:    %8f%8f%8f\n", Gyro_g[0], Gyro_g[1], Gyro_g[2]);
//			printf("Temperature:  %8.2f\n", Temp);
//			printf("MagneticField:%8d%8d%8d\n", Me[0], Me[1], Me[2]);
//			
//			OSSemPost(SensorSem);
//		}
//		OSSemPend(ReceiverSem,1000,&err);
//		for (int i = 0; i < 6; i++) { 
//		  		if (Duty[i] > 0.01) { 
//				printf("CH%i:%.2f %% \n", i + 1, Duty[i]/100);
//		  	}
//		}
//		OSSemPost(ReceiverSem);
//		OSTimeDly(200);
//	}
//}

void RECEIVER_TASK(void *pdata){
	INT8U err;
	err=err;
}

void GY86_TASK(void *pdata){
	INT8U err;
	while(1){
		OSSemPend(SensorSem,1000,&err);
		
		MPU6050ReadAcc(Acel);
		for(int i=0;i<3;i++){
			// Acel_g[i]=(float)Acel[i]/8192;
            Acel_g[i] = Acel[i] / 1673.469f;
        }
		MPU6050ReadGyro(Gyro);
		for(int i=0;i<3;i++){
            Gyro_g[i] = Gyro[i] / 131.2f;
        }
		
//		MPU6050_ReturnTemp(&Temp);
		HMC5884LReadMe(Me);
		for(int i=0;i<3;i++){
            Gyro_g[i] = Gyro_g[i];
        }
		
        mpu_dmp_get_data(&pitch,&roll,&yaw);

		OSSemPost(SensorSem);
		
		OSTimeDly(15);
	}
}

void MOTOR_TASK(void *pdata){
	INT8U err;
	while(1){
		OSSemPend(ReceiverSem,1000,&err);
		for (int i = 0; i < 4; i++) {
			Motor_Set(Duty[i], i + 1);
		}
		OSSemPost(ReceiverSem);
		OSTimeDly(50);
	}
}

void OLED_TASK(void *pdata){
	INT8U err;
	while(1){
		OSSemPend(SensorSem,1000,&err);
		
		OLED_Show_3num(Acel_g[0]*100, Acel_g[1]*100, Acel_g[2]*100, 1);
		OLED_Show_3num(Gyro[0], Gyro[1], Gyro[2], 0);
		OLED_ShowNum(24, 7, Temp, 2, 12);
		OLED_Show_3num(Me[0], Me[1], Me[2], 2);
		
		OSSemPost(SensorSem);
		
		OSSemPend(ReceiverSem,1000,&err);
		
		OLED_Show_3num((int)Duty[0]/100, (int)Duty[1]/100, (int)Duty[2]/100, 3);
		OLED_Show_3num((int)Duty[3]/100,(int)Duty[4]/100, (int)Duty[5]/100, 4);
		
		OSSemPost(ReceiverSem);
		
		OSTimeDly(500);
	}
}

void TEST_TASK(void *pdata){
	int x;
	while(1){
		x++;
		printf("%i\n",x);
		printf("begin\n");
		printf("end\n");
		OSTimeDly(1000);
	}
}


//使能systick中断为1ms
void SystemClock_Config()
{	
	 SysTick_Config(SystemCoreClock/1000);
}


int main()
{	
	SystemClock_Config();
	RCC_ClocksTypeDef get_rcc_clock;
	RCC_GetClocksFreq(&get_rcc_clock);
//	SysTick_Init();

    INIT_TASK(NULL);
//	while(1){
//		printf("next:\n");
//		mpu_dmp_get_data(&pitch,&roll,&yaw);
//		printf("Euler angles: %3f %3f %3f\n", pitch, roll, yaw);
//		Delay_ms(100);
//	}
	Delay_s(1);
    OSInit();

//    SensorSem=OSSemCreate(1);
//	ReceiverSem=OSSemCreate(1);

//	OSTaskCreate(HM10_TASK, (void *)0, (void *)&HM10_TASK_STK[HM10_STK_SIZE - 1], HM10_TASK_PRIO);
//	OSTaskCreate(INIT_TASK, (void *)0, (void *)&INIT_TASK_STK[INIT_STK_SIZE - 1], INIT_TASK_PRIO);
//	OSTaskCreate(RECEIVER_TASK, (void *)0, (void *)&RECEIVER_TASK_STK[RECEIVER_STK_SIZE - 1], RECEIVER_TASK_PRIO);
	OSTaskCreate(GY86_TASK, (void *)0, (void *)&GY86_TASK_STK[GY86_STK_SIZE - 1], GY86_TASK_PRIO);
//	OSTaskCreate(MOTOR_TASK, (void *)0, (void *)&MOTOR_TASK_STK[MOTOR_STK_SIZE - 1], MOTOR_TASK_PRIO);
//	OSTaskCreate(OLED_TASK, (void *)0, (void *)&OLED_TASK_STK[OLED_STK_SIZE - 1], OLED_TASK_PRIO);
//	OSTaskCreate(TEST_TASK, (void *)0, (void *)&TEST_TASK_STK[TEST_STK_SIZE - 1], TEST_TASK_PRIO);
	OSTaskCreate(DATA_TRANSFER_TASK, (void *)0, (void *)&DATA_TRANSFER_TASK_STK[DATA_TRANSFER_STK_SIZE - 1], DATA_TRANSFER_TASK_PRIO);
	
	
	OSStart();
	
}
