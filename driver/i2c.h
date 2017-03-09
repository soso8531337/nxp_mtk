#ifndef __I2C_H__
#define __I2C_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SPEED_100KHZ            (100000)

#define I2C_ADDR_7BIT		(0x0A)

enum{
	IOCTL_GET_STATUS_I2C = 0,
	IOCTL_GET_BATTERY_I2C,
	IOCTL_POWER_RESET_I2C,
	IOCTL_BUS_TEST_I2C,
};

#define I2C_STATUS_CMD				0x01
#define I2C_BATTERY_CMD 			0x10
#define I2C_POWER_RESET_CMD		0X60
#define I2C_BUS_TEST_CMD     0x5a

#define I2C_BUS_TEST_RETURN     0x55
#define I2C_RETRY_COUNT 5

extern int i2c_ioctl(int cmd, unsigned char *value);
extern int vs_i2c_init(void);
#endif
