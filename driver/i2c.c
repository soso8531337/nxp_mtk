/*
 * @brief I2CM example
 * This example shows how to use the I2CM interface in an interrupt context
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "i2c.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

//#define I2C_REG_ADDR_7BIT               (1)

static I2CM_XFER_T  i2cmXferRec;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void i2c_delay(uint32_t time)
{
	/* In an RTOS, the thread would sleep allowing other threads to run.
	   For standalone operation, we just spin on RI timer */
	int32_t curr = (int32_t) Chip_RIT_GetCounter(LPC_RITIMER);
	int32_t final = curr + ((SystemCoreClock / 1000) * time);

	if (final == curr) return;

	if ((final < 0) && (curr > 0)) {
		while (Chip_RIT_GetCounter(LPC_RITIMER) < (uint32_t) final) {}
	}
	else {
		while ((int32_t) Chip_RIT_GetCounter(LPC_RITIMER) < final) {}
	}

	return;
}


/* Initialize the I2C bus */
static void i2c_app_init(I2C_ID_T id, int speed)
{
	Board_I2C_Init(id);

	/* Initialize I2C */
	Chip_I2C_Init(id);
	Chip_I2C_SetClockRate(id, speed);

}

/* Function to setup and execute I2C transfer request */
static void SetupXferRecAndExecute(uint8_t devAddr,
								   uint8_t *txBuffPtr,
								   uint16_t txSize,
								   uint8_t *rxBuffPtr,
								   uint16_t rxSize)
{
	/* Setup I2C transfer record */
	i2cmXferRec.slaveAddr = devAddr;
	i2cmXferRec.options = 0;
	i2cmXferRec.status = 0;
	i2cmXferRec.txSz = txSize;
	i2cmXferRec.rxSz = rxSize;
	i2cmXferRec.txBuff = txBuffPtr;
	i2cmXferRec.rxBuff = rxBuffPtr;
	Chip_I2CM_Xfer(LPC_I2C0, &i2cmXferRec);

	/* Wait for transfer completion */
//	WaitForI2cXferComplete(&i2cmXferRec);
	Chip_I2CM_XferBlocking(LPC_I2C0, &i2cmXferRec);
}

/**
 * @brief	Main program body
 * @return	int
 */
int vs_i2c_init(void)
{

	i2c_app_init(I2C0, SPEED_100KHZ);

	/* Loop forever */

	return 0;
}

void vs_i2c_write(unsigned char cmd)
{
	SetupXferRecAndExecute(I2C_ADDR_7BIT, &cmd, 1, NULL, 0);
//	WriteBoard_I2CM(value);
}
void vs_i2c_read(uint8_t *value)
{
	SetupXferRecAndExecute(I2C_ADDR_7BIT, NULL, 0, value, 1);
//	ReadBoard_I2CM(value);
}

uint8_t vs_sendcmd_get_value(uint8_t cmd)
{
	uint8_t value;

	/* Write audio power value */
	SetupXferRecAndExecute(I2C_ADDR_7BIT, &cmd, 1, &value, 1);
	return value;
}

void loop_send_only_cmd(char cmd)
{
		int i;

		for(i=0; i < I2C_RETRY_COUNT; i++)
		{
			printf("send cmd:0x%02x\r\n",cmd);
				vs_i2c_write(cmd);
				i2c_delay(50);
		}
}


int i2c_ioctl(int cmd, unsigned char *value)
{
	int ret = 0;
	
	switch(cmd){
		case IOCTL_GET_STATUS_I2C:
			if(value == NULL)
				return -1;
			*value = vs_sendcmd_get_value(I2C_STATUS_CMD);
			break;
			
		case IOCTL_GET_BATTERY_I2C:
			if(value == NULL)
				return -1;
			*value = vs_sendcmd_get_value(I2C_BATTERY_CMD);
			break;
			
		case IOCTL_POWER_RESET_I2C:
			loop_send_only_cmd(I2C_POWER_RESET_CMD);
			break;
		
		case IOCTL_BUS_TEST_I2C:
			loop_send_only_cmd(I2C_BUS_TEST_CMD);
			break;
		
		default:
			printf("have not this cmd\r\n");
			ret = -1;
			break;
	}
	return ret;
		
}
