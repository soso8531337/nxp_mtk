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

/* Set I2C mode to polling/interrupt */
static void i2c_set_mode(I2C_ID_T id, int polling)
{
	if (!polling) {
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandler);

		/* Test for I2C0 interface */
		if (id == I2C0) {
			NVIC_EnableIRQ(I2C0_IRQn);
		}
		else {
			NVIC_EnableIRQ(I2C1_IRQn);
		}
	}
	else {
		/* Test for I2C0 interface */
		if (id == I2C0) {
			NVIC_DisableIRQ(I2C0_IRQn);
		}
		else {
			NVIC_DisableIRQ(I2C1_IRQn);
		}

		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandlerPolling);
	}
}

/* Initialize the I2C bus */
static void i2c_app_init(I2C_ID_T id, int speed)
{
	Board_I2C_Init(id);

	/* Initialize I2C */
	Chip_I2C_Init(id);
	Chip_I2C_SetClockRate(id, speed);

	/* Set default mode to interrupt */
	i2c_set_mode(id, 0);
}

/* Function to wait for I2CM transfer completion */
static void WaitForI2cXferComplete(I2CM_XFER_T *xferRecPtr)
{
	/* Test for still transferring data */
	while (xferRecPtr->status == I2CM_STATUS_BUSY) {
		/* Sleep until next interrupt */
		__WFI();
	}
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
	WaitForI2cXferComplete(&i2cmXferRec);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle I2C0 interrupt by calling I2CM interrupt transfer handler
 * @return	Nothing
 */
void I2C0_IRQHandler(void)
{
	/* Call I2CM ISR function with the I2C device and transfer rec */
	Chip_I2CM_XferHandler(LPC_I2C0, &i2cmXferRec);
}

/**
 * @brief	Main program body
 * @return	int
 */
int i2c_main(void)
{

	i2c_app_init(I2C0, SPEED_100KHZ);

	/* Loop forever */

	return 0;
}

void vs_i2c_write(unsigned char value)
{
	SetupXferRecAndExecute(I2C_ADDR_7BIT, &value, 1, NULL, 0);
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
