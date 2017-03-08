#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cmd_log.h"

xSemaphoreHandle log_rx_interrupt_sema = NULL;
char log_buf[LOG_SERVICE_BUFLEN];

struct command_item cmd_head;
struct command_item *cmd_tail;

int parse_param(char *buf, char **argv)
{
	char *tmp;
	int n=0, argc=0;
	
	if(strlen(buf)==0)
		return 0;
	
	tmp = buf;
	while(1){
		for(n=0;;n++){
			if(*tmp == '\0')
				return argc;
			if((*tmp != ' ')&&(*tmp != '\t'))
				break;
			tmp = tmp+1;
		}
		argv[argc++] = tmp;

		tmp = strchr(tmp, ' ');
		if(tmp == NULL){
			return argc;
		}else{
			*tmp = '\0';
			tmp = tmp+1;
		}
	}
}

void cmd_add_table(struct command_item *tbl, int len)
{
	int n=0;

	for(n=0; n<len; n++){
		cmd_tail->next = &tbl[n];
		cmd_tail = &tbl[n];
	}

	cmd_tail->next = NULL;

}

void cmd_achieve(char *cmd, char *para)
{
	struct command_item *cmd_tmp;
	
	cmd_tmp = cmd_head.next;

	while(cmd_tmp != NULL){
		if(strcmp(cmd, cmd_tmp->cmd_name)==0){
			cmd_tmp->cmd_fun(para);
			break;
		}
		cmd_tmp = cmd_tmp->next;
	}
	
}
void log_service(void *pvParameters)
{
	char *cmd, *parameter, *tmp;
	int n=0;
	printf("Start LOG SERVICE MODE\n\r");
	printf("\n\r# ");     
	
	while(1){
		while(xSemaphoreTake(log_rx_interrupt_sema, portMAX_DELAY) != pdTRUE);
		for(n=0;;n++){
			tmp = &log_buf[n];
			if((*tmp != ' ')&&(*tmp != '\t'))
				break;
		}
		cmd=tmp;

		tmp = strchr(log_buf, ' ');
		if(tmp == NULL){
			parameter = NULL;
		}else{
			parameter = tmp+1;
			*tmp = '\0';
		}
		cmd_achieve(cmd, parameter);
	}
}

void start_log_service(void)
{
	printf("creat log\r\n");
	if(xTaskCreate(log_service, (char const*)"log_service", 512, NULL, tskIDLE_PRIORITY + 2UL, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}

extern void add_p7_cmd(void);
void command_server_init(void)
{
	vSemaphoreCreateBinary(log_rx_interrupt_sema);
	xSemaphoreTake(log_rx_interrupt_sema, 1/portTICK_RATE_MS);
	memset(&cmd_head, 0, sizeof(struct command_item));
	cmd_head.next = NULL;
	cmd_tail = &cmd_head;
	add_p7_cmd();
	
	start_log_service();
}
