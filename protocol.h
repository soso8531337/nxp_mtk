#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/*Protocol Header*/
#define PRO_HDR				sizeof(struct scsi_head)

#define SCSI_HEAD_SIZE			sizeof(struct scsi_head)
#define SCSI_PHONE_MAGIC		 0xccddeeff
#define SCSI_DEVICE_MAGIC		 0xaabbccdd
#define SCSI_WFLAG  1 << 7


enum{
	EREAD = 1,
	EWRITE=2,
	ENODISK = 3,
	EDISKLEN = 4,
	EDISKINFO=5,
	EUPDATE=6
};

enum {
	SCSI_TEST = 0,
	SCSI_READ  = 1,//28
	SCSI_WRITE = 2 | SCSI_WFLAG,//2a
	SCSI_INQUIRY = 3,//12
	SCSI_READ_CAPACITY =4,//25
	SCSI_GET_LUN = 5,
	SCSI_INPUT = 6,
	SCSI_OUTPUT = 7,
	SCSI_UPDATE_START = 8,
	SCSI_UPDATE_DATA = 9 | SCSI_WFLAG,
	SCSI_UPDATE_END = 10,
	SCSI_FIRMWARE_INFO=11,
	SCSI_SYNC_INFO=12,
};

enum {
	PRO_OK = 0,
	PRO_BADMAGIC,
	PRO_INCOMPLTE,
	PRO_BADPACKAGE,
	PRO_NOSPACE,
};

struct scsi_head{
	int32_t head;	/*Receive OR Send*/
	int32_t wtag; /*Task ID*/
	int32_t ctrid; /*Command ID*/
	uint32_t addr; /*Offset addr*512   represent sectors */
	int32_t len;
	int16_t wlun;
	int16_t relag; /*Response Code*/
};

struct scsi_inquiry_info{
  int64_t size;
  char vendor[ 16];
  char product[ 32];
  char version[ 32];
  char serial[32];
};

typedef struct acessory_parameter
{
	char manufacture[32];
	char model_name[32];
	char sn[32];
	char fw_version[16];
	char hw_version[16];
	char cardid[8];
	char license[120];
} vs_acessory_parameter;

#endif
