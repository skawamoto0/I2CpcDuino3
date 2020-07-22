/* 
	Copyright (C) 2014 Suguru Kawamoto
	This software is released under the MIT License.
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define I2C_HIGH 1
#define I2C_LOW 0
#define I2C_FREQ 10000

long LastI2CClockTimestamp;
long I2CClockInterval;
int GPIOSCLIn;
int GPIOSCLOut;
int GPIOSDAIn;
int GPIOSDAOut;

void Init()
{
	char Temp[4];
	LastI2CClockTimestamp = 0;
	I2CClockInterval = 1000000000 / (I2C_FREQ * 4);
	GPIOSCLIn = open("/sys/devices/virtual/misc/gpio/mode/gpio10", O_RDWR);
	lseek(GPIOSCLIn, 0, SEEK_SET);
	strncpy(Temp, "8", 4);
	write(GPIOSCLIn, &Temp, 4);
	close(GPIOSCLIn);
	GPIOSCLIn = open("/sys/devices/virtual/misc/gpio/pin/gpio10", O_RDWR);
	GPIOSCLOut = open("/sys/devices/virtual/misc/gpio/mode/gpio11", O_RDWR);
	lseek(GPIOSCLOut, 0, SEEK_SET);
	strncpy(Temp, "1", 4);
	write(GPIOSCLOut, &Temp, 4);
	close(GPIOSCLOut);
	GPIOSCLOut = open("/sys/devices/virtual/misc/gpio/pin/gpio11", O_RDWR);
	GPIOSDAIn = open("/sys/devices/virtual/misc/gpio/mode/gpio12", O_RDWR);
	lseek(GPIOSDAIn, 0, SEEK_SET);
	strncpy(Temp, "8", 4);
	write(GPIOSDAIn, &Temp, 4);
	close(GPIOSDAIn);
	GPIOSDAIn = open("/sys/devices/virtual/misc/gpio/pin/gpio12", O_RDWR);
	GPIOSDAOut = open("/sys/devices/virtual/misc/gpio/mode/gpio13", O_RDWR);
	lseek(GPIOSDAOut, 0, SEEK_SET);
	strncpy(Temp, "1", 4);
	write(GPIOSDAOut, &Temp, 4);
	close(GPIOSDAOut);
	GPIOSDAOut = open("/sys/devices/virtual/misc/gpio/pin/gpio13", O_RDWR);
}

unsigned char GetSCL()
{
	char Temp[4];
	lseek(GPIOSCLIn, 0, SEEK_SET);
	read(GPIOSCLIn, &Temp, 4);
	return (Temp[0] == '0') ? I2C_LOW : I2C_HIGH;
}

void SetSCL(unsigned char Status)
{
	char Temp[4];
	lseek(GPIOSCLOut, 0, SEEK_SET);
	strncpy(Temp, (Status == I2C_LOW) ? "0" : "1", 4);
	write(GPIOSCLOut, &Temp, 4);
}

unsigned char GetSDA()
{
	char Temp[4];
	lseek(GPIOSDAIn, 0, SEEK_SET);
	read(GPIOSDAIn, &Temp, 4);
	return (Temp[0] == '0') ? I2C_LOW : I2C_HIGH;
}

void SetSDA(unsigned char Status)
{
	char Temp[4];
	lseek(GPIOSDAOut, 0, SEEK_SET);
	strncpy(Temp, (Status == I2C_LOW) ? "0" : "1", 4);
	write(GPIOSDAOut, &Temp, 4);
}

void WaitForQuarterClock()
{
	struct timespec tp;
	do
	{
		clock_gettime(CLOCK_MONOTONIC, &tp);
	}
	while(tp.tv_nsec / I2CClockInterval == LastI2CClockTimestamp);
	LastI2CClockTimestamp = tp.tv_nsec / I2CClockInterval;
}

void WaitForHalfClock()
{
	WaitForQuarterClock();
	WaitForQuarterClock();
}

void i2c_init()
{
	Init();
}

void i2c_stop()
{
	WaitForQuarterClock();
	if(GetSCL() != I2C_HIGH || GetSDA() != I2C_LOW)
	{
		SetSCL(I2C_LOW);
		WaitForQuarterClock();
		SetSDA(I2C_LOW);
		WaitForQuarterClock();
		SetSCL(I2C_HIGH);
		while(GetSCL() == I2C_LOW)
		{
			WaitForQuarterClock();
		}
		WaitForHalfClock();
	}
	SetSDA(I2C_HIGH);
	WaitForQuarterClock();
}

void i2c_start()
{
	WaitForQuarterClock();
	if(GetSCL() != I2C_HIGH || GetSDA() != I2C_HIGH)
	{
		SetSCL(I2C_LOW);
		WaitForQuarterClock();
		SetSDA(I2C_HIGH);
		WaitForQuarterClock();
		SetSCL(I2C_HIGH);
		while(GetSCL() == I2C_LOW)
		{
			WaitForQuarterClock();
		}
		WaitForHalfClock();
	}
	SetSDA(I2C_LOW);
	WaitForQuarterClock();
	SetSCL(I2C_LOW);
	WaitForQuarterClock();
}

void i2c_rep_start()
{
	return i2c_start();
}

unsigned char i2c_write(unsigned char data)
{
	unsigned char r;
	unsigned char i;
	if(GetSCL() != I2C_LOW)
	{
		SetSCL(I2C_LOW);
		WaitForQuarterClock();
	}
	for(i = 0; i < 8; i++)
	{
		SetSDA((data & (0x80 >> i)) ? I2C_HIGH : I2C_LOW);
		WaitForQuarterClock();
		SetSCL(I2C_HIGH);
		while(GetSCL() == I2C_LOW)
		{
			WaitForQuarterClock();
		}
		WaitForHalfClock();
		SetSCL(I2C_LOW);
		WaitForQuarterClock();
	}
	SetSDA(I2C_HIGH);
	WaitForQuarterClock();
	SetSCL(I2C_HIGH);
	while(GetSCL() == I2C_LOW)
	{
		WaitForQuarterClock();
	}
	WaitForHalfClock();
	r = (GetSDA() == I2C_LOW) ? 0 : 1;
	SetSCL(I2C_LOW);
	WaitForQuarterClock();
	return r;
}

unsigned char i2c_read(unsigned char ack)
{
	unsigned char r;
	unsigned char i;
	if(GetSCL() != I2C_LOW)
	{
		SetSCL(I2C_LOW);
		WaitForQuarterClock();
	}
	SetSDA(I2C_HIGH);
	r = 0;
	for(i = 0; i < 8; i++)
	{
		WaitForQuarterClock();
		SetSCL(I2C_HIGH);
		while(GetSCL() == I2C_LOW)
		{
			WaitForQuarterClock();
		}
		WaitForHalfClock();
		r |= (GetSDA() == I2C_HIGH) ? (0x80 >> i) : 0;
		SetSCL(I2C_LOW);
		WaitForQuarterClock();
	}
	SetSDA((ack == 0) ? I2C_HIGH : I2C_LOW);
	WaitForQuarterClock();
	SetSCL(I2C_HIGH);
	while(GetSCL() == I2C_LOW)
	{
		WaitForQuarterClock();
	}
	WaitForHalfClock();
	SetSCL(I2C_LOW);
	WaitForQuarterClock();
	return r;
}

int main(int argc, char* argv[])
{
	short s;
	printf("ADT7410  thermometer\n");
	i2c_init();
	i2c_start();
	i2c_write((0x48 << 1) | 0x00);
	i2c_write(0x03);
	i2c_write(0x80);
	i2c_stop();
	while(1)
	{
		usleep(500000);
		i2c_start();
		i2c_write((0x48 << 1) | 0x00);
		i2c_write(0x00);
		i2c_rep_start();
		i2c_write((0x48 << 1) | 0x01);
		s = i2c_read(1) & 0xff;
		s = (s << 8) | (i2c_read(0) & 0xff);
		i2c_stop();
		printf("%.3f C\n", (float)s / 128);
	}
	return 0;
}

