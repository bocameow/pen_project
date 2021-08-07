/*
 * 1.13.1.c
 * 
 * Copyright 2021  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>

void main() 
{
	// Create I2C bus
	int file;
	char *bus = "/dev/i2c-1";
	if ((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}
	// Get I2C device, ADXL345 I2C address is 0x53(83)
	ioctl(file, I2C_SLAVE, 0x53);

	// Select Bandwidth rate register(0x2C)
	// Normal mode, Output data rate = 100 Hz(0x0A)
	char config[2]={0};
	config[0] = 0x2C;
	config[1] = 0x0F;
	write(file, config, 2);
	// Select Power control register(0x2D)
	// Auto-sleep disable(0x08)
	config[0] = 0x2D;
	config[1] = 0x08;
	write(file, config, 2);
	// Select Data format register(0x31)
	// Self test disabled, 4-wire interface, Full resolution, range = +/-2g(0x08)
	config[0] = 0x31;
	config[1] = 0x0B;
	write(file, config, 2);
	sleep(1);

	// Read 6 bytes of data from register(0x32)
	// xAccl lsb, xAccl msb, yAccl lsb, yAccl msb, zAccl lsb, zAccl msb
	
	//for(int a=0;a<10;a++)
	//time
	struct timespec ts1, ts2;
	char reg[1] = {0x32};
	char data[6] ={0};
	int xAccl;
	int yAccl;
	int zAccl;
	unsigned long long ms1, ms2, diff;
	double d;
	
	//while(1)
	for(int a=0;a<10;a++)
	{
		timespec_get(&ts1, TIME_UTC);
		write(file, reg, 1);
		if(read(file, data, 6) != 6)
		{
			printf("Erorr : Input/output Erorr \n");
			exit(1);
		}
		else
		{
			// Convert the data to 10-bits
			xAccl = ((data[1] & 0x03) * 256 + (data[0] & 0xFF));
			yAccl = ((data[3] & 0x03) * 256 + (data[2] & 0xFF));
			zAccl = ((data[5] & 0x03) * 256 + (data[4] & 0xFF));
			if(xAccl > 511)
			{
				xAccl -= 1024;
			}
			
			if(yAccl > 511)
			{
				yAccl -= 1024;
			}
			if(zAccl > 511)
			{
				zAccl -= 1024;
			}

			// Output data to screen
			printf("data : %d \n", data);
			printf("data1 : %d \n", data[1]);
			printf("data0 : %d \n", data[0]);
			printf("data3 : %d \n", data[3]);
			printf("data2 : %d \n", data[2]);
			printf("data5 : %d \n", data[5]);
			printf("data4 : %d \n", data[4]);
			printf("Acceleration in X-Axis : %d \n", xAccl);
			printf("Acceleration in Y-Axis : %d \n", yAccl);
			printf("Acceleration in Z-Axis : %d \n", zAccl);
			timespec_get(&ts2, TIME_UTC);
			ms1 = ts1.tv_nsec;
			ms2 = ts2.tv_nsec;
			diff = ms2-ms1;
			d= diff/1000000000.0;
			//printf("First: %llu\n",ms1);
			//printf("Last:  %llu\n",ms2);
			//printf("%llu\n",diff);
			//printf("%f\n",d);
			printf("\n");
		
			//int o;
			//o+=diff;
			//printf(" o:%d\n", o);
			//printf("\n");
		}
	}
}



