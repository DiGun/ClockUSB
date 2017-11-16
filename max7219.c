/*
 * CFile1.c
 *
 * Created: 24.10.2017 16:38:58
 *  Author: DiGun
 */ 
#include "max7219.h"

#include <avr/io.h>

#define cs_lo PORTB |= (1<<PB2) // 1 на SS данные могут записываться в регистр защёлку (16-розрядн.)
#define cs_hi PORTB &= ~(1<<PB2)// 0 на SS активный выбор кристала

//------------------------------------------------------
//Адреси регістрів настройки МАХ7219
//------------------------------------------------------
#define MAX7219_MODE_DECODE 0x09    // Режим декодирования
#define MAX7219_MODE_INTENSITY 0x0A // Яркость
#define MAX7219_MODE_SCAN_LIMIT 0x0B// Розряднось дисплея
#define MAX7219_MODE_POWER 0x0C     // Выключение питания
#define MAX7219_MODE_TEST 0x0F      // Тестовий режим
#define MAX7219_MODE_NOOP 0x00      // Пустая операция
//Адреси регістрів сегментів

#define MAX7219_CHAR_BLANK 0xF     // пустота
#define MAX7219_CHAR_NEGATIVE 0xA  // минус "-" 


unsigned char active_digit = 8;//к-ть активних розрядів дисплея 

PROGMEM const uint8_t NUMBER_FONT[] = {
	0x7E,	//0
	0x30,	//1
	0x6D,	//2
	0x79,	//3
	0x33,	//4
	0x5B,	//5
	0x5F,	//6
	0x70,	//7
	0x7F,	//8
	0x7B,	//9
	0x77,	//A
	0x1F,	//B
	0x4E,	//C
	0x3D,	//D
	0x4F,	//E
	0x47	//F
};


void MAX7219_writeData(char data_register, char data)
{
	cs_hi; //Slave_SPI активний може приймати дані
	spi_fast_shift(data_register);//старші біти передаємо першими
	spi_fast_shift(data);// їх потіснять на місця біти байта даних
	cs_lo; // Закриваємо Slave_SPI
}

void MAX7219_init()
{
    MAX7219_writeData(MAX7219_MODE_DECODE,0x00); // режим декодирования выключен
    MAX7219_writeData(MAX7219_MODE_SCAN_LIMIT,active_digit-1);// Всі сегменти скануються 0-7
    MAX7219_writeData(MAX7219_MODE_INTENSITY, 8); // яскравість на половину 17/34
    MAX7219_writeData(MAX7219_MODE_POWER, 1);// вмикаємо всі табла	
}	


void sendChar(uint8_t pos, uint8_t data, bool dot)
{
	MAX7219_writeData(pos, data | (dot ? 0b10000000 : 0));
}

void setDisplayDigit(uint8_t pos, uint8_t digit, bool dot)
{
	sendChar(pos, pgm_read_byte(&NUMBER_FONT[digit & 0xF]), dot);
}

void clearDisplayDigit(uint8_t pos, bool dot)
{
	sendChar(pos, 0, dot);
}

void clearDisplay()
{
		for (uint8_t i = 1; i <= BYTE_SYMBOL ; i++)
		{
					clearDisplayDigit(i, 0);
		}	
}

void setDisplayToDecNumberAt(unsigned long number, uint8_t dots, uint8_t startingPos,uint8_t len, bool leadingZeros)
{
	if (number > 99999999L)
	{
		//		setDisplayToError();
//		sendError();
		//;
		;
	}
	else
	{
		for (uint8_t i = startingPos; (i <= BYTE_SYMBOL)&&(i<startingPos+len) ; i++) 
		{
			if (number != 0) {
				setDisplayDigit(i, number % 10, (dots & (1 << (i-1))) != 0);
				number /= 10;
			}
			else
			{
				if (leadingZeros||(i == startingPos)) 
				{
					setDisplayDigit(i, 0, (dots & (1 << (i-1))) != 0);
				}
				else
				{
					clearDisplayDigit(i, (dots & (1 << (i-1))) != 0);
				}
			}
		}
	}
}
