/*
 * @brief UART interrupt example with ring buffers
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
#include "chip.h"
#include "board.h"
#include "string.h"
#include "cmd_log.h"
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Transmit and receive ring buffers */
STATIC RINGBUFF_T txring, rxring;

/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 128	/* Send */
#define UART_RRB_SIZE 64	/* Receive */

/* Transmit and receive buffers */
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];

#if (defined(BOARD_NGX_XPLORER_1830) || defined(BOARD_NGX_XPLORER_4330))
/* Use UART0 for NGX boards */
#define LPC_UARTX       LPC_USART0
#define UARTx_IRQn      USART0_IRQn
#define UARTx_IRQHandler UART0_IRQHandler
#endif

#if defined(BOARD_NXP_LPCXPRESSO_4337)
/* Use UART0 for LPC-Xpresso boards */
#define LPC_UARTX       LPC_USART0
#define UARTx_IRQn      USART0_IRQn
#define UARTx_IRQHandler UART0_IRQHandler
#endif

#if (defined(BOARD_KEIL_MCB_1857) || defined(BOARD_KEIL_MCB_4357))
/* Use UART3 for Keil boards */
#define LPC_UARTX       LPC_USART3
#define UARTx_IRQn      USART3_IRQn
#define UARTx_IRQHandler UART3_IRQHandler
#endif
#if (defined(BOARD_HITEX_EVA_1850) || defined(BOARD_HITEX_EVA_4350))
/* Use UART0 for Hitex boards */
#define LPC_UARTX       LPC_USART0
#define UARTx_IRQn      USART0_IRQn
#define UARTx_IRQHandler UART0_IRQHandler
#endif
#if defined(BOARD_NXP_LPCLINK2_4370)
#define LPC_UARTX       LPC_USART2
#define UARTx_IRQn      USART2_IRQn
#define UARTx_IRQHandler UART2_IRQHandler
#endif

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

static uint8_t cmd_flag=0;
/**
 * @brief	UART interrupt handler using ring buffers
 * @return	Nothing
 */
char rebuf[LOG_SERVICE_BUFLEN];
void UARTx_IRQHandler(void)
{
	uint8_t key;
	static unsigned int n=0;
	int bytes;
	char flag[3];
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART_IRQRBHandler(LPC_UARTX, &rxring, &txring);
	
	bytes = Chip_UART_ReadRB(LPC_UARTX, &rxring, &key, 1);
	
//	Chip_UART_SendRB(LPC_UARTX, &txring, (const uint8_t *) &key, 1);

	if((bytes > 0)&&(n<UART_RRB_SIZE)){
		if(key == '\r'){
			flag[0] = '\r';
			flag[1] = '\n';
			flag[2] = '#';
			if(n<2){
				Chip_UART_SendRB(LPC_UARTX, &txring, (const uint8_t *)flag, 3);
			}else{
				Chip_UART_SendRB(LPC_UARTX, &txring, (const uint8_t *)flag, 2);
				memset(log_buf, 0, sizeof(log_buf));
				strncpy(log_buf, rebuf, strlen(rebuf));
				cmd_flag = 1;
			}
			n = 0;
			memset(rebuf, 0, sizeof(rebuf));
		}else if((key==8)||(key==127)){
			if(n > 0)
			{
				rebuf[n-1] = '\0';
				
				flag[0] = '\b';
				flag[1] = ' ';
				flag[2] = '\b';
				Chip_UART_SendRB(LPC_UARTX, &txring, (const uint8_t *)flag, 3);
				n--;
			}
		}else{
			rebuf[n] = key;
			rebuf[n+1] = '\0';
			
			if (Chip_UART_SendRB(LPC_UARTX, &txring, (const uint8_t *) &key, 1) != 1) {
				Board_LED_Toggle(0);/* Toggle LED if the TX FIFO is full */
			}
			n++;

		}

	}
}

void vs_cmd_exec(void *pvParameters)
{
	
	while(1){
		if(cmd_flag == 1){
			xSemaphoreGive(log_rx_interrupt_sema);
			cmd_flag = 0;
		}else{
			vTaskDelay(20);
		}
	}
}

/**
 * @brief	Main UART program body
 * @return	Always returns 1
 */
int consol_init(void)
{

	Board_Init();
	Board_UART_Init(LPC_UARTX);

	/* Setup UART for 115.2K8N1 */
	Chip_UART_Init(LPC_UARTX);
	Chip_UART_SetBaud(LPC_UARTX, 115200);
	Chip_UART_ConfigData(LPC_UARTX, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UARTX, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UARTX);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UARTX, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UARTX, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(UARTx_IRQn, 1);
	NVIC_EnableIRQ(UARTx_IRQn);
	command_server_init();
	xTaskCreate(vs_cmd_exec, "vTaskcmd", 256, NULL, (tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);
	return 1;
}
