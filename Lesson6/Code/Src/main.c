/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "STM32F103x8.h"
#include "stm32f103x8_gpio_driver.h"
#include "lcd.h"
#include "keypad.h"
#include "stm32f103x8_EXTI_driver.h"
#include "stm32f103x8_USART_driver.h"
#include "stm32f103x8_SPI_driver.h"
#include "stm32f103x8_I2C_driver.h"
#include "I2C_Slave_EEPROM.h"
#include "core_cm3.h"

#define OS_SET_PSP(add)        __asm volatile ("mov r0,%0 \n\t msr PSP,r0 \n\r" : : "r" (add))
#define OS_SWITCH_SP_TO_PSP    __asm volatile ("mrs r0,CONTROL \n\t orr r0,r0,#0x2 \n\t msr CONTROL,r0 \n\t")
#define OS_SWITCH_SP_TO_MSP    __asm volatile ("mrs r0,CONTROL \n\t mov r1,#0x5 \n\t and r0,r0,r1 \n\t msr CONTROL,r3 \n\t")
#define OS_GEN_SWI             __asm volatile ("SVC #0x03")

#define Switch_CPUAccessLevel_TO_PAL   __asm(  "mrs r3,CONTROL \n\t lsr r3,r3,#0x1 \n\t lsl r3,r3,#0x1 \n\t msr CONTROL,r3 \n\t")
#define Switch_CPUAccessLevel_TO_NPAL  __asm(  "mrs r3,CONTROL \n\t orr r3,r3,#0x1 \n\t msr CONTROL,r3 \n\t")

#define MAIN_STACK_SIZE  512
#define TASKA_STACK_SIZE 100
#define TASKB_STACK_SIZE 100


 uint8_t IRQ_Flag = 0;
 uint8_t TaskA_FLag = 0;
 uint8_t TaskB_Flag = 1;

 uint32_t Glob_u32xPSR = 6;
 uint32_t Glob_u32ControlReg =6;

extern int _estack; //start of the stack

//MAIN STACK Pointer
uint32_t  _S_MSP = &_estack;
uint32_t  _E_MSP;

//Process Stack Pointer For Task A
unsigned int _S_PSP_TA;
unsigned int _E_PSP_TA;

//Process Stack Pointer For Task B
unsigned int _S_PSP_TB;
unsigned int _E_PSP_TB;


typedef enum Cpu_AccessLevel
{
	PRIVILEGED,   // 0
	UNPRIVILEGED  // 1
}Cpu_AccessLevel_t;

void Switch_CPU_AccessLevel(Cpu_AccessLevel_t Level)
{
	switch(Level)
	{
	case PRIVILEGED: //set CPU To privileged Mode
		__asm( "mrs r3,CONTROL \n\t"
			   "lsr r3,r3,#0x01 \n\t"
			   "lsl r3,r3,#0x01 \n\t"
			   "msr CONTROL,r3");
		break;

	case UNPRIVILEGED: //set CPU To unprivileged Mode
		__asm( "mrs r3,CONTROL \n\t"
			   "orr r3,#0x01 \n\t"
			   "msr CONTROL,r3");
		break;
	}


}

void EXTI9_CALLBACK(void)
{
	IRQ_Flag  =  ~IRQ_Flag;
	TaskA_FLag = ~TaskA_FLag;
	TaskB_Flag = ~TaskB_Flag;
}

//void SVC_Handler()
//{
//	Switch_CPUAccessLevel_TO_PAL; //switch to privileged
//}

int TaskA (int a , int b , int c )
{
	return a + b + c;
}

int TaskB (int a , int b , int c ,int d)
{
	return a + b + c + d;
}
void MAIN_OS()
{
	_E_MSP = _S_MSP - MAIN_STACK_SIZE; //size of main stack pointer
	_S_PSP_TA = _E_MSP - 8; //Leave space between main and PSP 8 bytes.
	_E_PSP_TA = _S_PSP_TA - TASKA_STACK_SIZE;

	_S_PSP_TB = _E_PSP_TA - 8; //Leave space between stacks of tasks.
	_E_PSP_TB = _S_PSP_TB - TASKB_STACK_SIZE;

	while(1)
	{
		__asm("NOP \n\t");
		if(TaskA_FLag)
		{
			// Set PSP Register = _S_PSP_TA
			OS_SET_PSP(_S_PSP_TA);
			// SP -> PSP
			OS_SWITCH_SP_TO_PSP;
			// Switch from Privileged to UnPrivileged
			Switch_CPUAccessLevel_TO_NPAL;
			//Or --> Switch_CPUAccessLevel(unprivileged);

			TaskA_FLag =TaskA(1,2,3);

			// Switch from UnPrivileged to Privileged
			OS_GEN_SWI;
			// SP -> MSP
			OS_SWITCH_SP_TO_MSP;
		}
		else if(TaskB_Flag)
		{
			// Set PSP Register = _S_PSP_TB
			OS_SET_PSP(_S_PSP_TB);
			// SP -> PSP
			OS_SWITCH_SP_TO_PSP;
			// Switch from Privileged to UnPrivileged
			Switch_CPUAccessLevel_TO_PAL;

			//Or --> Switch_CPUAccessLevel(unprivileged);

			TaskB_Flag=TaskB(1,2,3,4);

			// Switch from UnPrivileged to Privileged
			OS_GEN_SWI;
			// SP -> MSP
			OS_SWITCH_SP_TO_MSP;
		}
	}
}

enum SVC_ID
{
	ADD,
	SUB,
	MUL
};

void myFunc() //0x080080B4
{
	__asm("nop\n \t nop\n \t nop\n \t nop\n \t nop\n \t nop");

}

void OS_SVC_services(uint32_t* StackFramePointer)
{
	uint8_t svcNumber = *(uint8_t*)((uint8_t*)(StackFramePointer[6])-2); //svc number

	uint32_t val1 = StackFramePointer[0];
	uint32_t val2 = StackFramePointer[1];

	switch(svcNumber)
	{
	case 1:
		StackFramePointer[0] = val1 + val2;

		break;

	case 2:
		StackFramePointer[0] = val1 - val2;
		break;

	case 3:
		StackFramePointer[0] = val1 * val2;
		break;
	}


}
__attribute ((naked)) void SVC_Handler()
{
	__asm(  "tst lr, #04 \n\t "
			"ITE EQ \n\t"
			"mrseq r0, msp\n\t"
			"mrsne r0, psp\n\t"
			"B OS_SVC_services");

}

uint32_t OS_Svc_Set(uint32_t a, uint32_t b, uint32_t Svc_Id)
{
	switch(Svc_Id)
	{
	case ADD:
		__asm("SVC #01");
		break;

	case SUB:
		__asm("SVC #02");
		break;

	case MUL:
		__asm("SVC #03");
		break;
	}
}

int main(void)
{
//	//int in1, in2, out  = 0;
//	RCC_GPIOB_CLK_EN();
//	RCC_AFIO_CLK_EN();
//
//	//Set EXTI Configuration
//	EXTI_PinConfig_t EXTIConfig;
//	EXTIConfig.EXTI_PIN = EXTI9PB9;
//	EXTIConfig.Trigger_Case = EXTI_Trigger_RISING;
//	EXTIConfig.P_IRQ_CallBack = EXTI9_CALLBACK;
//	EXTIConfig.IRQ_EN = EXTI_IRQ_Enable;
//	//MCAL_EXTI_GPIO_Init(&EXTIConfig);
//    MCAL_EXTI_GPIO_Init(&EXTIConfig);
//
////    Switch_CPU_AccessLevel(UNPRIVILEGED);
////    Switch_CPU_AccessLevel(PRIVILEGED);
//
//    MAIN_OS();

 uint8_t addResult = OS_Svc_Set(3, 2, ADD);
 uint8_t subResult = OS_Svc_Set(3, 2, SUB);
 uint8_t mulResult = OS_Svc_Set(3, 2, MUL);

	IRQ_Flag = 1;
	while (1)
	{
		if(IRQ_Flag)
		{
			IRQ_Flag = 0;
		}
		else { /* Misra */ }
	}


}
