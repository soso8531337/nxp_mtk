/*
 * @brief SD/MMC benchmark example
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
//#pragma arm section code ="RW_IRAM2", rwdata="RW_IRAM2"
#include <string.h>
#include "board.h"
#include "chip.h"
#include "spifilib_api.h"
#include "flash_map.h"

#define TEST_BUFFSIZE (0x8000)
#define TICKRATE_HZ1 (1000)	/* 1000 ticks per second I.e 1 mSec / tick */

SPIFI_HANDLE_T *pSpifi;
//static unsigned char usb_buffer[0x8000]; // __attribute__((section ("USB_RAM1"), zero_init));
static flash_info spi_rw_info;
static char init_flag = 0;

//STATIC const PINMUX_GRP_T spifipinmuxing[] = {
//	{0x3, 3,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI CLK */
//	{0x3, 4,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D3 */
//	{0x3, 5,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D2 */
//	{0x3, 6,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D1 */
//	{0x3, 7,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D0 */
//	{0x3, 8,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)}	/* SPIFI CS/SSEL */
//};


/* Local memory, 32-bit aligned that will be used for driver context (handle) */
static uint32_t lmem[21];

static uint32_t CalculateDivider(uint32_t baseClock, uint32_t target)
{
	uint32_t divider = (baseClock / target);
	
	/* If there is a remainder then increment the dividor so that the resultant
	   clock is not over the target */
	if(baseClock % target) {
		++divider;
	}
	return divider;
}

static SPIFI_HANDLE_T *initializeSpifi(void)
{
	uint32_t memSize;
	SPIFI_HANDLE_T *pReturnVal;
//	SPIFI_DEV_ENUMERATOR_T ctx;
//	const char * devName;

	/* register support for the family(s) we may want to work with
	     (only 1 is required) */
    spifiRegisterFamily(spifi_REG_FAMILY_CommonCommandSet);
	
    /* Enumerate the list of supported devices */
        
//		 devName = spifiDevEnumerateName(&ctx, 1);
			
//			DEBUGOUT("Supported devices:\r\n");
//			while (devName) {
//					DEBUGOUT("\t%s\r\n", devName);  
//					devName = spifiDevEnumerateName(&ctx, 0);
//			}

	/* Get required memory for detected device, this may vary per device family */
	memSize = spifiGetHandleMemSize(LPC_SPIFI_BASE);
	if (memSize == 0) {
		/* No device detected, error */
		printf("spifiGetHandleMemSize error -%d\r\n", SPIFI_ERR_GEN);
		return NULL;
	}

	/* Initialize and detect a device and get device context */
	/* NOTE: Since we don't have malloc enabled we are just supplying
	     a chunk of memory that we know is large enough. It would be
	     better to use malloc if it is available. */
	pReturnVal = spifiInitDevice(&lmem, sizeof(lmem), LPC_SPIFI_BASE, FLASH_UBOOT_ADDR);
	if (pReturnVal == NULL) {
		printf("spifiInitDevice error -%d\r\n", SPIFI_ERR_GEN);
		return NULL;
	}

	return pReturnVal;
}

static int init_spi_info(void)
{
	uint32_t spifiBaseClockRate;
	uint32_t maxSpifiClock;
	uint32_t loopBytes;
	uint32_t subblock_size;
SPIFI_ERR_T errCode;

	/* Setup SPIFI FLASH pin muxing (QUAD) */
//	Chip_SCU_SetPinMuxing(spifipinmuxing, sizeof(spifipinmuxing) / sizeof(PINMUX_GRP_T));

	/* SPIFI base clock will be based on the main PLL rate and a divider */
	spifiBaseClockRate = Chip_Clock_GetClockInputHz(CLKIN_MAINPLL);

	DEBUGOUT("SPIFI clock rate %d\r\n", Chip_Clock_GetClockInputHz(CLKIN_IDIVE));

	/* Initialize the spifi library. This registers the device family and detects the part */
	pSpifi = initializeSpifi();
	if(pSpifi == NULL){
		printf("initializesPifi error\r\n");
		return -1;
	}
	/* Get some info needed for the application */
	maxSpifiClock = spifiDevGetInfo(pSpifi, SPIFI_INFO_MAXCLOCK);

#if 0
	/* Get some info needed for the application */
	maxSpifiClock = spifiDevGetInfo(pSpifi, SPIFI_INFO_MAXCLOCK);

	/* Get info */
	DEBUGOUT("Device Identified   = %s\r\n", spifiDevGetDeviceName(pSpifi));
	DEBUGOUT("Capabilities        = 0x%x\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_CAPS));
	DEBUGOUT("Device size         = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_DEVSIZE));
	DEBUGOUT("Max Clock Rate      = %d\r\n", maxSpifiClock);
	DEBUGOUT("Erase blocks        = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_ERASE_BLOCKS));
	DEBUGOUT("Erase block size    = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_ERASE_BLOCKSIZE));
	DEBUGOUT("Erase sub-blocks    = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_ERASE_SUBBLOCKS));
	DEBUGOUT("Erase sub-blocksize = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_ERASE_SUBBLOCKSIZE));
	DEBUGOUT("Write page size     = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_PAGESIZE));
	DEBUGOUT("Max single readsize = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_MAXREADSIZE));
	DEBUGOUT("Current dev status  = 0x%x\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_STATUS));
	DEBUGOUT("Current options     = %d\r\n", spifiDevGetInfo(pSpifi, SPIFI_INFO_OPTIONS));
#endif
	/* Setup SPIFI clock to at the maximum interface rate the detected device
	   can use. This should be done after device init. */
	Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, CalculateDivider(spifiBaseClockRate, maxSpifiClock));

	DEBUGOUT("SPIFI final Rate    = %d\r\n", Chip_Clock_GetClockInputHz(CLKIN_IDIVE));
	DEBUGOUT("\r\n");

	/* start by unlocking the device */
	DEBUGOUT("Unlocking device...\r\n");
	errCode = spifiDevUnlockDevice(pSpifi);
	if (errCode != SPIFI_ERR_NONE) {
		printf("unlockDevice error -%d\r\n", errCode);
		return -errCode;
	}
	loopBytes = spifiDevGetInfo(pSpifi, SPIFI_INFO_PAGESIZE);
	subblock_size = spifiDevGetInfo(pSpifi, SPIFI_INFO_ERASE_SUBBLOCKSIZE);  // get block size
	spi_rw_info.subblock_size = subblock_size;
	spi_rw_info.loopBytes = loopBytes;
	spi_rw_info.maxSpifiClock = maxSpifiClock;
	spi_rw_info.spifiBaseClockRate = spifiBaseClockRate;

	init_flag=1;
	DEBUGOUT("init end.\r\n");
	return 0;
}

int spi_read(uint32_t flash_addr, unsigned char *buff, unsigned int size)
{
	int loopBytes;
	uint32_t tmp_addr = flash_addr;
	int tmp_size = size, i;
	SPIFI_ERR_T errCode;
	uint32_t *read_addr;
	loopBytes = spi_rw_info.loopBytes;
	
	printf("read from 0x%x\r\n",flash_addr);
	if(init_flag == 1){
			for(i=0; tmp_size>0; i++){
			read_addr = (uint32_t *)&buff[i*loopBytes];
			if(tmp_size < spi_rw_info.loopBytes)
				loopBytes = tmp_size;
			
			errCode = spifiDevRead(pSpifi, tmp_addr, read_addr, loopBytes);   // write one page every time
			if (errCode != SPIFI_ERR_NONE) {
				printf("WriteBlock ox%x error,errcode = %d\r\n", tmp_addr, errCode);
				return -errCode;
			}
				
			tmp_addr = tmp_addr + loopBytes;
			tmp_size = tmp_size -loopBytes;
		}
	}else{
			memcpy((void *)buff, (void *)flash_addr, size);
	}
	return 0;
}
static int spi_write(uint32_t flash_addr, unsigned char *buff, unsigned int size)
{
	int loopBytes;
	uint32_t tmp_addr = flash_addr, subBlock_num, erase_addr, n;
	int tmp_size = size, i;
	SPIFI_ERR_T errCode;
	uint32_t *write_addr;
	loopBytes = spi_rw_info.loopBytes;
	
	subBlock_num = spifiGetSubBlockFromAddr(pSpifi,flash_addr);
	erase_addr = spifiGetAddrFromSubBlock(pSpifi, subBlock_num);
	if(erase_addr < flash_addr){
		n = subBlock_num + 1;
	}else{
		n = subBlock_num;
	}
	printf("flash_addr=0x%x, erase_addr = 0x%x,n = %d, sub_num = %d\r\n",flash_addr, erase_addr, n, subBlock_num);
	subBlock_num = spifiGetSubBlockFromAddr(pSpifi,flash_addr+size);
	printf("subBlock_num = %d\r\n",subBlock_num);
	
	for(n=n; n<=subBlock_num; n++){
		spifiDevEraseSubBlock(pSpifi, n);
	}
//	spifiEraseByAddr(pSpifi, flash_addr,flash_addr+size);

	for(i=0; tmp_size>0; i++){
		printf(".");
		write_addr = (uint32_t *)&buff[i*loopBytes];
		if(tmp_size < spi_rw_info.loopBytes)
			loopBytes = tmp_size;
		
		errCode = spifiDevPageProgram(pSpifi, tmp_addr, write_addr, loopBytes);   // write one page every time
		if (errCode != SPIFI_ERR_NONE) {
			printf("WriteBlock ox%x error,errcode = %d\r\n", tmp_addr, errCode);
			return -errCode;
		}
		
		tmp_addr = tmp_addr + loopBytes;
		tmp_size = tmp_size -loopBytes;
	}
	printf("\r\n");
	return 0;
}
int check_write_data(unsigned char *s_data, unsigned int size, unsigned int flash_addr)
{
	unsigned char tmp[1024];
	int i,j=0, tmp_size = size;
	int loop_read = 1024, ret;
	unsigned int offset = 0;
	for(i=0; tmp_size > 0;i++){
		if(loop_read > tmp_size){
			loop_read = tmp_size;
		}
		printf("check 0x%x\r\n", flash_addr + offset);
		memset(tmp, 0, 1024);
		ret = spi_read(flash_addr+offset, tmp, loop_read);
		if(ret != 0){
			printf("read error\r\n");
			return -1;
		}
		for(j=0; j<loop_read; j++){
			if(tmp[j] != s_data[offset + j])
				printf("flash != usb addr = 0x%x\r\n", flash_addr+offset+j);
		}
		offset = offset + loop_read;
		tmp_size = tmp_size - loop_read;
	}
	return 0;
}
int fw_upgrade(unsigned char *buff, unsigned int size, int end_flag)
{
	static int flag = 0;
	static uint32_t flash_addr = FLASH_UPGRADE_IMAGE;
	int ret;
	uint32_t subBlock_num;
	
	if(flag == 0){
		flag = 1;
		ret = init_spi_info();
		if(ret != 0){
			printf("spi_info error \r\n");
			return -1;
		}
		printf("start erase data\r\n");
		subBlock_num = (FLASH_SYS_DATA-FLASH_UBOOT_ADDR)/spi_rw_info.subblock_size;
		spifiDevEraseSubBlock(pSpifi,subBlock_num);
	}
	
	printf("start write data\r\n");
	ret = spi_write(flash_addr, buff, size);
	if(ret != 0){
		printf("write error\r\n");
		return -1;
	}
	check_write_data(buff, size, flash_addr);
	
	flash_addr = flash_addr + size;
	if(end_flag == 1){
		flash_addr = FLASH_UPGRADE_IMAGE;
		flag = 0;
		spifiDevDeInit(pSpifi);
		init_flag = 0;
	}
	return 0;
}

int set_flage(void)
{
	int ret = 0;
	image_info sys_data;
	SPIFI_ERR_T errCode;
	
	init_spi_info();
	memset(&sys_data, 0, sizeof(sizeof(struct __image_info)));
	sys_data.flag = 1;
	sys_data.offset = FLASH_UPGRADE_IMAGE;
	printf("start write sys data\r\n");

	errCode = spifiDevPageProgram(pSpifi, FLASH_SYS_DATA, (uint32_t *)(&sys_data), sizeof(struct __image_info));
	if (errCode != SPIFI_ERR_NONE) {
			printf("Write sysdata error\r\n");
			ret = -1;
	}
	spifiDevDeInit(pSpifi);
	init_flag = 0;
	return ret;
}

