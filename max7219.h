/*
 * max7219.h
 *
 * Created: 24.10.2017 16:38:42
 *  Author: DiGun
 */ 


#ifndef MAX7219_H_
#define MAX7219_H_
#include "spi.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

#define BYTE_SYMBOL 0x08          //Количество символов индикатора

extern void MAX7219_init();
extern void MAX7219_writeData(char data_register, char data);
extern void sendChar(uint8_t pos, uint8_t data, bool dot);
extern void setDisplayDigit(uint8_t pos, uint8_t digit, bool dot);
extern void clearDisplayDigit(uint8_t pos, bool dot);
extern void clearDisplay();
extern void setDisplayToDecNumberAt(unsigned long number, uint8_t dots, uint8_t startingPos,uint8_t len, bool leadingZeros);






#endif /* MAX7219_H_ */