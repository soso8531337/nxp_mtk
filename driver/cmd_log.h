#ifndef CMD_LOG_H
#define CMD_LOG_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define LOG_SERVICE_BUFLEN     64
#define MAX_LOG_PARA	8
extern void command_server_init(void);
extern char log_buf[LOG_SERVICE_BUFLEN];
extern xSemaphoreHandle log_rx_interrupt_sema;
extern int mmcsd_read(unsigned char *buff, unsigned long sector, unsigned char count);
extern int mmcsd_write(unsigned char *buff, unsigned long sector, unsigned char count);
extern void vs_i2c_write(int value);
extern void vs_i2c_read(uint8_t *value);
extern uint8_t vs_sendcmd_get_value(uint8_t cmd);

typedef struct command_item{
	char *cmd_name;
	void (*cmd_fun)(char *);
	struct command_item *next;
}cmd_st;

extern int parse_param(char *buf, char **argv);
extern void cmd_add_table(struct command_item *tbl, int len);

#endif
