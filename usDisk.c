/*
 * @brief U-Storage Project
 *
 * @note
 * Copyright(C) i4season, 2016
 * Copyright(C) Szitman, 2016
 * All rights reserved.
 */
#if defined(NXP_CHIP_18XX)
#pragma arm section code ="USB_RAM2", rwdata="USB_RAM2"
#endif
#include <stdint.h>
#include "usDisk.h"
#include "usUsb.h"
#include "usSys.h"
#include "protocol.h"
#if defined(NXP_CHIP_18XX)
#include "MassStorageHost.h"
#elif defined(LINUX)
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/un.h>
#include <sys/select.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>

#if !defined(BLKGETSIZE64)
#define BLKGETSIZE64           _IOR(0x12,114,size_t)
#endif

#endif

#define MSC_FTRANS_CLASS				0x08
#define MSC_FTRANS_SUBCLASS			0x06
#define MSC_FTRANS_PROTOCOL			0x50

#if defined(DEBUG_ENABLE1)
#define DSKDEBUG(...) do {printf("[DISK Mod]");printf(__VA_ARGS__);} while(0)
#else
#define DSKDEBUG(...)
#endif

#define STOR_DFT_PRO		"U-Storage"
#define STOR_DFT_VENDOR		"i4season"

enum{
	DISK_REOK=0,
	DISK_REPARA,
	DISK_REGEN,
	DISK_REINVAILD
};

typedef struct {
	uint8_t disknum;
	uint32_t Blocks; /**< Number of blocks in the addressed LUN of the device. */
	uint32_t BlockSize; /**< Number of bytes in each block in the addressed LUN. */
	int64_t disk_cap;
	usb_device diskdev;
}usDisk_info;

usDisk_info uDinfo;

#if defined(NXP_CHIP_18XX)
extern uint8_t DCOMP_MS_Host_NextMSInterfaceEndpoint(void* const CurrentDescriptor);
/*****************************************************************************
 * Private functions
 ****************************************************************************/

static uint8_t NXP_COMPFUNC_MSC_CLASS(void* const CurrentDescriptor)
{
	USB_Descriptor_Header_t* Header = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Header_t);

	if (Header->Type == DTYPE_Interface){
		USB_Descriptor_Interface_t* Interface = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Interface_t);
		if (Interface->Class  == MSC_FTRANS_CLASS&&
				(Interface->SubClass == MSC_FTRANS_SUBCLASS) &&
		   		(Interface->Protocol ==MSC_FTRANS_PROTOCOL)){
			return DESCRIPTOR_SEARCH_Found;
		}
	}

	return DESCRIPTOR_SEARCH_NotFound;
}

void usDisk_DeviceInit(void *os_priv)
{
	return ;
}

uint8_t usDisk_DeviceDetect(void *os_priv)
{	
	USB_StdDesDevice_t DeviceDescriptorData;
	uint8_t MaxLUNIndex;
	usb_device *usbdev = &(uDinfo.diskdev);
	
	memset(&uDinfo, 0, sizeof(uDinfo));
	/*set os_priv*/
	usUsb_Init(usbdev, os_priv);
	/*GEt device description*/
	memset(&DeviceDescriptorData, 0, sizeof(USB_StdDesDevice_t));
	if(usUsb_GetDeviceDescriptor(usbdev, &DeviceDescriptorData)){
		DSKDEBUG("usUusb_GetDeviceDescriptor Failed\r\n");
		return DISK_REGEN;
	}
	
	/*Set callback*/	
	nxp_clminface nxpcall;	
	nxpcall.callbackInterface = NXP_COMPFUNC_MSC_CLASS;
	nxpcall.callbackEndpoint= DCOMP_MS_Host_NextMSInterfaceEndpoint;
	/*Claim Interface*/
	nxpcall.bNumConfigurations = DeviceDescriptorData.bNumConfigurations;
	if(usUsb_ClaimInterface(usbdev, &nxpcall)){
		DSKDEBUG("Attached Device Not a Valid DiskDevice.\r\n");
		return DISK_REINVAILD;
	}

	if(usUsb_GetMaxLUN(usbdev, &MaxLUNIndex)){		
		DSKDEBUG("Get LUN Failed\r\n");
		return DISK_REINVAILD;
	}
	DSKDEBUG(("Total LUNs: %d - Using first LUN in device.\r\n"), (MaxLUNIndex + 1));
	if(usUsb_ResetMSInterface(usbdev)){		
		DSKDEBUG("ResetMSInterface Failed\r\n");
		return DISK_REINVAILD;
	}	
	SCSI_Sense_Response_t SenseData;
	if(usUsb_RequestSense(usbdev, 0, &SenseData)){
		DSKDEBUG("RequestSense Failed\r\n");
		return DISK_REINVAILD;
	}

	SCSI_Inquiry_t InquiryData;
	if(usUsb_GetInquiryData(usbdev, 0, &InquiryData)){
		DSKDEBUG("GetInquiryData Failed\r\n");
		return DISK_REINVAILD;
	}

	if(usUsb_ReadDeviceCapacity(usbdev, &uDinfo.Blocks, &uDinfo.BlockSize)){
		DSKDEBUG("ReadDeviceCapacity Failed\r\n");
		return DISK_REINVAILD;
	}
	uDinfo.disknum=1;
	uDinfo.disk_cap = uDinfo.BlockSize *uDinfo.Blocks;
	DSKDEBUG("Mass Storage Device Enumerated. [Num:%d Blocks:%d BlockSzie:%d Cap:%lld]\r\n",
			uDinfo.disknum, uDinfo.Blocks, uDinfo.BlockSize, uDinfo.disk_cap);
	return DISK_REOK;
}

#elif defined(LINUX)

#ifndef BLKROSET
#define BLKROSET   _IO(0x12,93)
#define BLKROGET   _IO(0x12,94)
#define BLKRRPART  _IO(0x12,95)
#define BLKGETSIZE _IO(0x12,96)
#define BLKFLSBUF  _IO(0x12,97)
#define BLKRASET   _IO(0x12,98)
#define BLKRAGET   _IO(0x12,99)
#define BLKSSZGET  _IO(0x12,104)
#define BLKBSZGET  _IOR(0x12,112,size_t)
#define BLKBSZSET  _IOW(0x12,113,size_t)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)
#endif

#define BLKRASIZE			(1024)
#define SYS_CLA_BLK 	"/sys/class/block"
#define SYS_BLK		"/sys/block"

usb_device disk_phone;
char dev[256];
int diskFD = -1;

static int disk_chk_proc(char *dev)
{
	FILE *procpt = NULL;
	int ma, mi, sz;
	char line[128], ptname[64], devname[256] = {0};

	if ((procpt = fopen("/proc/partitions", "r")) == NULL) {
		DSKDEBUG("Fail to fopen(proc/partitions)\r\n");
		return 0;		
	}
	while (fgets(line, sizeof(line), procpt) != NULL) {
		memset(ptname, 0, sizeof(ptname));
		if (sscanf(line, " %d %d %d %[^\n ]",
				&ma, &mi, &sz, ptname) != 4)
				continue;
		if(!strcmp(ptname, dev)){
			DSKDEBUG("Partition File Found %s\r\n", dev);
			sprintf(devname, "/dev/%s", dev);
			if(access(devname, F_OK)){
				mknod(devname, S_IFBLK|0644, makedev(ma, mi));
			}	
			fclose(procpt);
			return 1;
		}
	}

	fclose(procpt);
	return 0;
}

static int blockdev_readahead(char *devname, int readahead)
{
	int fd;
	long larg;
	
	if(!devname){
		return -1;
	}
	fd = open(devname, O_RDWR);
	if (fd < 0) {
		return -1;
	}
	if(ioctl(fd, BLKRAGET, &larg) == -1){	
		DSKDEBUG("Get ReadAhead Error[%s]...\r\n", devname);
		close(fd);
		return -1;
	}
	if(ioctl(fd, BLKRASET, readahead) == -1){
		DSKDEBUG("Set ReadAhead Error[%s]...\r\n", devname);
		close(fd);
		return -1;
	}
	DSKDEBUG("Set %s ReadAhead From %ld To %d...\r\n", 
				devname, larg, readahead);
	close(fd);

	return 0;
}

void usDisk_DeviceInit(void *os_priv)
{
	struct dirent *dent;
	DIR *dir;
	struct stat statbuf;
	char sys_dir[1024] = {0};

	/*Get Block Device*/
	if(stat(SYS_CLA_BLK, &statbuf) == 0){
		strcpy(sys_dir, SYS_CLA_BLK);
	}else{
		if(stat(SYS_BLK, &statbuf) == 0){
			strcpy(sys_dir, SYS_BLK);
		}else{
			DSKDEBUG("SYS_CLASS can not find block\r\n");
			memset(sys_dir, 0, sizeof(sys_dir));
			return ;
		}
	}
		
	dir = opendir(sys_dir);
	if(dir == NULL){
		DSKDEBUG("Opendir Failed\r\n");
		return ;
	}	
	while((dent = readdir(dir)) != NULL){
		char devpath[512], linkbuf[1024] = {0};
		int len;
		char devbuf[128] = {0};
		int fd = -1;
				
		if(strstr(dent->d_name, "sd") == NULL || strlen(dent->d_name) != 3){
			if(strstr(dent->d_name, "mmcblk") == NULL || 
				strlen(dent->d_name) != 7){
				continue;
			}
		}		
		if(disk_chk_proc(dent->d_name) == 0){
			DSKDEBUG("Partition Not Exist %s\r\n", dent->d_name);
			continue;
		}
		len = strlen(sys_dir) + strlen(dent->d_name) + 1;
		sprintf(devpath, "%s/%s", sys_dir, dent->d_name);
		devpath[len] = '\0';
		if(readlink(devpath, linkbuf, sizeof(linkbuf)-1) < 0){
			DSKDEBUG("ReadLink %s Error:%s\r\n", linkbuf, strerror(errno));
			continue;
		}

		sprintf(devbuf, "/dev/%s", dent->d_name);
		/*Check dev can open*/
		if((fd = open(devbuf, O_RDONLY)) < 0){
			DSKDEBUG("Open [%s] Failed:%s\r\n", 
					devbuf, strerror(errno));
			continue;
		}else{
			close(fd);
		}
		/*preread*/
		usDisk_DeviceDetect((void*)devbuf);
		DSKDEBUG("ADD Device [%s] To Storage List\r\n", dent->d_name);
		break;
	}

	closedir(dir);
}
uint8_t usDisk_DeviceDetect(void *os_priv)
{
	unsigned char sense_b[32] = {0};
	unsigned char rcap_buff[8] = {0};
	unsigned char cmd[] = {0x25, 0, 0, 0 , 0, 0};
	struct sg_io_hdr io_hdr;
	unsigned int lastblock, blocksize;
	int dev_fd;
	int64_t disk_cap = 0;

	if(os_priv == NULL){
		return DISK_REGEN;
	}
	memset(&uDinfo, 0, sizeof(uDinfo));
	strcpy(dev, os_priv);
	disk_phone.os_priv = dev;
	memcpy(&uDinfo.diskdev, &disk_phone, sizeof(usb_device));

	/*Set readahead parameter*/
	if(blockdev_readahead(dev, BLKRASIZE) <  0){
		DSKDEBUG("SetReadAhead %s Failed\r\n", dev);
	}
	/*open diskFD*/
	close(diskFD);
	diskFD = open(dev, O_RDWR);
	if(diskFD < 0){
			DSKDEBUG("Open diskFD %s Failed\r\n", dev);
	}
	DSKDEBUG("Open diskFD %s %d Successful\r\n", dev, diskFD);

	dev_fd= open(dev, O_RDWR | O_NONBLOCK);
	if (dev_fd < 0 && errno == EROFS)
		dev_fd = open(dev, O_RDONLY | O_NONBLOCK);
	if (dev_fd<0){
		DSKDEBUG("Open %s Failed:%s", dev, strerror(errno));
		return DISK_REGEN; 
	}

	memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = sizeof(cmd);
	io_hdr.dxferp = rcap_buff;
	io_hdr.dxfer_len = 8;
	io_hdr.mx_sb_len = sizeof(sense_b);
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.cmdp = cmd;
	io_hdr.sbp = sense_b;
	io_hdr.timeout = 20000;

	if(ioctl(dev_fd, SG_IO, &io_hdr)<0){
		DSKDEBUG("IOCTRL error:%s[Used BLKGETSIZE64]!", strerror(errno));
		if (ioctl(dev_fd, BLKGETSIZE64, &disk_cap) != 0) {			
			DSKDEBUG("Get Disk Capatiy Failed");
		}		
		DSKDEBUG("Disk Capacity = %lld Bytes", disk_cap);
		close(dev_fd);
		uDinfo.disk_cap = disk_cap;
		uDinfo.BlockSize = 512;
		uDinfo.Blocks = disk_cap/uDinfo.BlockSize;
		uDinfo.disknum=1;
		return DISK_REOK;
	}

	/* Address of last disk block */
	lastblock =  ((rcap_buff[0]<<24)|(rcap_buff[1]<<16)|
	(rcap_buff[2]<<8)|(rcap_buff[3]));

	/* Block size */
	blocksize =  ((rcap_buff[4]<<24)|(rcap_buff[5]<<16)|
	(rcap_buff[6]<<8)|(rcap_buff[7]));

	/* Calculate disk capacity */
	uDinfo.Blocks= (lastblock+1);
	uDinfo.BlockSize= blocksize;	
	uDinfo.disk_cap  = (lastblock+1);
	uDinfo.disk_cap *= blocksize;
	uDinfo.disknum=1;
	DSKDEBUG("Disk Blocks = %u BlockSize = %u Disk Capacity=%lld\n", 
			uDinfo.Blocks, uDinfo.BlockSize, uDinfo.disk_cap);
	close(dev_fd);

	return DISK_REOK;
}
#endif

uint8_t usDisk_DeviceDisConnect(void)
{
	memset(&uDinfo, 0, sizeof(uDinfo));
	return DISK_REOK;
}
uint8_t usDisk_DiskReadSectors(void *buff, uint32_t secStart, uint32_t numSec)
{
	if(!buff || !uDinfo.disknum){
		DSKDEBUG("DiskReadSectors Failed[DiskNum:%d].\r\n", uDinfo.disknum);
		return DISK_REPARA;
	}
	if(usUsb_DiskReadSectors(&uDinfo.diskdev, 
			buff, secStart,numSec, uDinfo.BlockSize)){
		DSKDEBUG("DiskReadSectors Failed[DiskNum:%d secStart:%d numSec:%d].\r\n", 
						uDinfo.disknum, secStart, numSec);
		return DISK_REGEN;
	}
	
	DSKDEBUG("DiskReadSectors Successful[DiskNum:%d secStart:%d numSec:%d].\r\n", 
					uDinfo.disknum, secStart, numSec);
	return DISK_REOK;
}

uint8_t usDisk_DiskWriteSectors(void *buff, uint32_t secStart, uint32_t numSec)
{
	if(!buff || !uDinfo.disknum){
		DSKDEBUG("DiskWriteSectors Failed[DiskNum:%d].\r\n", uDinfo.disknum);
		return DISK_REPARA;
	}
	
	if(usUsb_DiskWriteSectors(&uDinfo.diskdev, 
				buff, secStart,numSec, uDinfo.BlockSize)){
		DSKDEBUG("DiskWriteSectors Failed[DiskNum:%d secStart:%d numSec:%d].\r\n", 
						uDinfo.disknum, secStart, numSec);
		return DISK_REGEN;
	}
	
	DSKDEBUG("DiskWriteSectors Successful[DiskNum:%d secStart:%d numSec:%d].\r\n", 
					uDinfo.disknum, secStart, numSec);
	return DISK_REOK;	
}

uint8_t usDisk_DiskNum(void)
{
	return uDinfo.disknum;
}

uint8_t usDisk_DiskInquiry(struct scsi_inquiry_info *inquiry)
{
	if(!inquiry){
		DSKDEBUG("usDisk_DiskInquiry Parameter Error\r\n");
		return DISK_REPARA;
	}
	memset(inquiry, 0, sizeof(struct scsi_inquiry_info));
	/*Init Other Info*/
	inquiry->size = uDinfo.disk_cap;
	strcpy(inquiry->product, STOR_DFT_PRO);
	strcpy(inquiry->vendor, STOR_DFT_VENDOR);
	strcpy(inquiry->serial, "1234567890abcdef");
	strcpy(inquiry->version, "1.0");

	return DISK_REOK;	
}

#if defined(NXP_CHIP_18XX)
#pragma arm section code, rwdata
#endif 


