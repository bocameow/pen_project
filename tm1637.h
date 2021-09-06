//  Author : Pierre Brial, 2021
//  Based on Arduino TM1637 library by avishorp (https://github.com/avishorp/TM1637)
//  Depends on Wiring Pi library by Gordon Henderson (http://wiringpi.com/)
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include <wiringPi.h>
#include <stdint.h>
#include <stdbool.h>

#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80
#define BITDELAY 100 // pause between writes in microseconds

	//      A
	//     ---
	//  F |   | B
	//     -G-
	//  E |   | C
	//     ---
	//      D
const uint8_t digitToSegment[] = {
		// XGFEDCBA
		0b00111111,    // 0
		0b00000110,    // 1
		0b01011011,    // 2
		0b01001111,    // 3
		0b01100110,    // 4
		0b01101101,    // 5
		0b01111101,    // 6
		0b00000111,    // 7
		0b01111111,    // 8
		0b01101111,    // 9
		0b01110111,    // A
		0b01111100,    // b
		0b00111001,    // C
		0b01011110,    // d
		0b01111001,    // E
		0b01110001     // F
		};

const uint8_t blank[] = {0,0,0,0};

uint8_t
    m_pinClk,
    m_pinDIO,
    m_brightness;
    
void TMstartWrite()
	{
	pinMode(m_pinDIO, OUTPUT);
	delayMicroseconds(BITDELAY);
	}

void TMstopWrite()
	{
	pinMode(m_pinDIO, OUTPUT);
	delayMicroseconds(BITDELAY);
	pinMode(m_pinClk, INPUT);
	delayMicroseconds(BITDELAY);
	pinMode(m_pinDIO, INPUT);
	delayMicroseconds(BITDELAY);
	}

void TMwriteByte(uint8_t b)
	{
	uint8_t data = b;
	
	// 8 Data Bits
	for(uint8_t i = 0; i < 8; i++)
		{
		// CLK low
		pinMode(m_pinClk, OUTPUT);
		delayMicroseconds(BITDELAY);
		
		// Set data bit
		if (data & 0x01)
		  pinMode(m_pinDIO, INPUT);
		else
		  pinMode(m_pinDIO, OUTPUT);
		
		delayMicroseconds(BITDELAY);
		
		// CLK high
		pinMode(m_pinClk, INPUT);
		delayMicroseconds(BITDELAY);
		data = data >> 1;
		}

	// Wait for acknowledge
	// CLK to zero
	pinMode(m_pinClk, OUTPUT);
	pinMode(m_pinDIO, INPUT);
	delayMicroseconds(BITDELAY);
	
	// CLK to high
	pinMode(m_pinClk, INPUT);
	delayMicroseconds(BITDELAY);
	if (digitalRead(m_pinDIO) == 0) pinMode(m_pinDIO, OUTPUT);
	delayMicroseconds(BITDELAY);
	pinMode(m_pinClk, OUTPUT);
	delayMicroseconds(BITDELAY);
	}

void TMsetup(uint8_t pinClk, uint8_t pinDIO)
  //! Initialize a TMsetup object, setting the clock and data pins
  //! (uses wiringpi numbering scheme : https://pinout.xyz/pinout/wiringpi#)
  //! @param pinClk : digital pin connected to the clock pin of the module
  //! @param pinDIO : digital pin connected to the DIO pin of the module
	{
	wiringPiSetup();

	// Copy the pin numbers
	m_pinClk = pinClk;
	m_pinDIO = pinDIO;
	
	// Set the pin direction and default value.
	// Both pins are set as inputs, allowing the pull-up resistors to pull them up
	pinMode(m_pinClk, INPUT);
	pinMode(m_pinDIO,INPUT);
	digitalWrite(m_pinClk, LOW);
	digitalWrite(m_pinDIO, LOW);
	}

void TMsetBrightness(uint8_t brightness)
  //! Sets the brightness of the display.
  //!
  //! Takes effect when a command is given to change the data being displayed.
  //!
  //! @param brightness A number from 0 (lower brightness) to 7 (highest brightness)
  	{
	m_brightness = ((brightness & 0x7) | 0x08) & 0x0f;
	}

void TMsetSegments(const uint8_t segments[], uint8_t length, uint8_t pos)
  //! Display arbitrary data on the module
  //!
  //! This function receives raw segment values as input and displays them. The segment data
  //! is given as a byte array, each byte corresponding to a single digit. Within each byte,
  //! bit 0 is segment A, bit 1 is segment B etc.
  //! The function may either set the entire display or any desirable part on its own. The first
  //! digit is given by the @ref pos argument with 0 being the leftmost digit. The @ref length
  //! argument is the number of digits to be set. Other digits are not affected.
  //!
  //! @param segments An array of size @ref length containing the raw segment values
  //! @param length The number of digits to be modified
  //! @param pos The position from which to start the modification (0 - leftmost, 3 - rightmost)
	{
	// Write COMM1
	TMstartWrite();
	TMwriteByte(TM1637_I2C_COMM1);
	TMstopWrite();
	
	// Write COMM2 + first digit address
	TMstartWrite();
	TMwriteByte(TM1637_I2C_COMM2 + (pos & 0x03));
	
	// Write the data bytes
	for (uint8_t k=0; k < length; k++)
	  TMwriteByte(segments[k]);
	
	TMstopWrite();
	
	// Write COMM3 + brightness
	TMstartWrite();
	TMwriteByte(TM1637_I2C_COMM3 + m_brightness);
	TMstopWrite();
	}

void TMclear()
	{
	TMsetSegments(blank,4,0);
	}

void TMshowNumber(int num, uint8_t dots, bool leading_zero, uint8_t length, uint8_t pos)
  //! Displays a decimal number, with dot control
  //!
  //! Displays the given argument as a decimal number. The dots between the digits (or colon)
  //! can be individually controlled
  //!
  //! @param num The number to be shown
  //! @param dots Dot/Colon enable. The argument is a bitmask, with each bit corresponding to a dot
  //!        between the digits (or colon mark, as implemented by each module). i.e.
  //!        For displays with dots between each digit:
  //!        * 0000 (0)
  //!        * 0.000 (0b10000000)
  //!        * 00.00 (0b01000000)
  //!        * 000.0 (0b00100000)
  //!        * 0.0.0.0 (0b11100000)
  //!        For displays with just a colon:
  //!        * 00:00 (0b01000000)
  //!        For displays with dots and colons colon:
  //!        * 0.0:0.0 (0b11100000)
  //! @param leading_zero When true, leading zeros are displayed. Otherwise unnecessary digits are
  //!        blank
  //! @param length The number of digits to set
  //! @param pos The position least significant digit (0 - leftmost, 3 - rightmost)
	{
	uint8_t digits[4];
	const static int divisors[] = { 1, 10, 100, 1000 };
	bool leading = true;
	
	for(int8_t k = 0; k < 4; k++)
		{
		int divisor = divisors[4 - 1 - k];
		int d = num / divisor;
		uint8_t digit = 0;
		
		if (d == 0)
			{
			if (leading_zero || !leading || (k == 3)) digit = digitToSegment[d];
			else digit = 0;
			}
		else
			{
			digit = digitToSegment[d];
			num -= d * divisor;
			leading = false;
			}
	
		// Add the decimal point/colon to the digit
		digit |= (dots & 0x80); 
		dots <<= 1;
		digits[k] = digit;
		}
	
	TMsetSegments(digits + (4 - length), length, pos);
	}
	
void TMshowDouble(double x)
	//! Displays a double as 00.00
	{
	const uint8_t
    	minus[] =     {64,64,64,64},
    	zeropoint[] = {0B10111111};
	int x100;
	if (x>99) x=99.99;
	x100=x*(x<0 ? -1000 : 1000);// round and abs
	x100=x100/10+(x100%10 > 4);	//
	if (x100<100)
		{
		TMsetSegments(zeropoint,1,1);
		TMshowNumber(x100,0b1000000,true,2,2);
		TMsetSegments(x<0 ? minus : blank , 1, 0);
		}
	else if (x<0)
		{
		TMsetSegments(minus, 1, 0);
		TMshowNumber(x100,0b1000000,false,3,1);
		}
	else TMshowNumber(x100,0b1000000,false,4,0);
	}

