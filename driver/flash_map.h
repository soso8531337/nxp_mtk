#ifndef __FLASH_MAP_H__
#define __FLASH_MAP_H__

#include <stdint.h>
#define FLASH_UBOOT_ADDR			(0x14000000)
#define FLASH_SYS_DATA				(0x14009000)
#define FLASH_DEF_IMAGE				(0x1400B000)
#define FLASH_UPGRADE_IMAGE		(0x14020000)
#define FLASH_OTHER_DATA			(0x14050000)

typedef struct __flash_info{
	unsigned int spifiBaseClockRate;
	unsigned int maxSpifiClock;
	unsigned int loopBytes;
	unsigned int subblock_size;
}flash_info;

typedef struct __image_info{
	 unsigned char flag;
	 unsigned int offset;
 }image_info;
 
 /*
  *success return 0.
 */
 extern int spi_read(uint32_t flash_addr, unsigned char *buff, unsigned int size);
 extern int set_flage(void);
 
 /*
  *if buff is last data, set end_flag=1,other set end_flag = 0
 */
 extern int fw_upgrade(unsigned char *buff, unsigned int size, int end_flag);
 
 #endif
