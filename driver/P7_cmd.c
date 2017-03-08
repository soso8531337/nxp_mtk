#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmd_log.h"
#include "ff.h"

void vs_i2c_test(char *arg)
{
//	int argc=0;
	unsigned char value;
	char *para[MAX_LOG_PARA];
	parse_param(arg, para);
	printf("i2c test value = %d\r\n",atoi(para[0]));
	if(atoi(para[1])==1){
		value = vs_sendcmd_get_value(atoi(para[0]));
		printf("value = 0x%02x\r\n",value);
	}else if(atoi(para[1])==2){
		vs_i2c_write(atoi(para[0]));
	}else if(atoi(para[1])==3){
		vs_i2c_read(&value);
	}else{
		vs_i2c_write(atoi(para[0]));
		vTaskDelay(10);
		vs_i2c_read(&value);
	}
}

void p7_help(char *arg)
{
	printf("============================================================================================\r\n");
	printf("|i2c: i2c data mode| mode-1,send data get back. 2,only send 3. only read 4. send delay read\r\n");
	printf("|help: |show help\r\n");
	printf("==========================================================================================\r\n");

}

cmd_st p7_command[]={
	{"i2c", vs_i2c_test,},
	{"help", p7_help,},
};

void add_p7_cmd(void)
{
	cmd_add_table(p7_command, sizeof(p7_command)/sizeof(p7_command[0]));
}
