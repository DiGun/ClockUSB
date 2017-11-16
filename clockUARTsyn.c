/*
 * clockUARTsyn.c
 *
 * Created: 14.11.2017 9:28:21
 *  Author: DiGun
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#define BAUD 9600
#include <util/setbaud.h>
#include "max7219.h"
#include "uart.h"

void Init()
{
	// init Timer0
/*
	TIMSK |= (1 << TOIE0);        // interrupt enable - here overflow
	TCCR0 |= TIMER0_PRESCALER; // use defined prescaler value
	//разрешаем внешнее прерывание на int0
	//   GICR |= (1<<INT0)|(1<<INT1);
	//настраиваем условие прерывания
	//   MCUCR |= (1<<ISC01)|(1<<ISC00)|(1<<ISC11)|(1<<ISC10);
	encoded=0;
*/	
	uart_init();
	//	init_USART();
	_delay_ms(250);
	sei();
	_delay_ms(250);

}

uint32_t time;

#define TIME_ZONE 2 * 60*60
static const uint8_t numofdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	
typedef struct {
	uint16_t   year;   /* 1970..2106 */
	uint8_t      month;   /* 1..12 */
	uint8_t      mday;   /* 1..31 */
	uint8_t      hour;   /* 0..23 */
	uint8_t      min;   /* 0..59 */
	uint8_t      sec;   /* 0..59 */
	uint8_t      wday;   /* 0..6 (Sun..Sat) */
} RTCTIME;

RTCTIME tm;


void NumbToUART(uint32_t number)
{
	char s[10];
	char* c;
	c=ultoa(number,s,10);
	while(c[0])
	{
		uart_putc_w(c[0]);
		c++;
	}
}

/*------------------------------------------*/
/* Convert time structure to timeticks      */
/*------------------------------------------*/

uint32_t Time2Unix(const RTCTIME *rtc)
{
	uint32_t utc, i, y;


	y = rtc->year - 1970;
	if (y > 2106 || !rtc->month || !rtc->mday) return 0;

	utc = y / 4 * 1461; y %= 4;
	utc += y * 365 + (y > 2 ? 1 : 0);
	
	for (i = 0; i < 12 && i + 1 < rtc->month; i++) 
	{
		utc += numofdays[i];
		if (i == 1 && y == 2) utc++;
	}
	
	utc *= 86400;
	return utc;
	
	utc += rtc->mday - 1;
	utc *= 86400;
	utc += rtc->hour * 3600 + rtc->min * 60 + rtc->sec;

//	utc -= (long)(_RTC_TDIF * 3600);
	return utc;
}

/*------------------------------------------*/
/* Get time in calendar form                */
/*------------------------------------------*/

void Unix2Time(uint32_t utc, RTCTIME *rtc)
{
	uint32_t n,i,d;
//	utc = RTC_GetCounter();
	/* Compute  hours */
//	if(timetype==time_current)
	{
		rtc->sec = (uint8_t)(utc % 60); utc /= 60;
		rtc->min = (uint8_t)(utc % 60); utc /= 60;
		rtc->hour = (uint8_t)(utc % 24); utc /= 24;
	}
/*	
	if(timetype==time_midnight)
	{
		rtc->sec = 0;
		rtc->min = 0;
		rtc->hour = 0;
		utc/=86400;
	}
*/	
	rtc->wday = (uint8_t)((utc + 4) % 7);
	rtc->year = (uint16_t)(1970 + utc / 1461 * 4); utc %= 1461;
	n = ((utc >= 1096) ? utc - 1 : utc) / 365;
	rtc->year += n;
	utc -= n * 365 + (n > 2 ? 1 : 0);
	for (i = 0; i < 12; i++) {
		d = numofdays[i];
		if (i == 1 && n == 2) d++;
		if (utc < d) break;
		utc -= d;
	}
	rtc->month = (uint8_t)(1 + i);
	rtc->mday = (uint8_t)(1 + utc);
}



uint8_t cmd_cur;
uint8_t cmd_mode; //текущий режим
uint8_t cmd_type; //тип команды
uint8_t cmd_status;


inline void func_hello(void)
{
	uart_putc_w('A');
}

void func_error(void)
{
	uart_putc_w('E');
	cmd_mode=0;
}


inline uint8_t func_type(char c)
{
	switch (c)
	{
	case 'S':	//set
	case 'G':	//get
		cmd_type=c;
		break;
	default:
		cmd_type=0;
		func_error();
	}
	return cmd_type;
}


void main_func(char c)
{
	switch (cmd_mode)
	{
	case 0:
		if (c=='Q')
		{
			func_hello();
			cmd_mode++;
		}
		else
		{
			func_error();
		}
		break;
	case 1:
			if (func_type(c))
			{
				cmd_mode++;
			} 
		break;
	case 2:
		switch (cmd_type)
		{
		case 'G':
			break;
		case 'S':
			break;
		}
	break;
		
	}
	
}


int main(void)
{
		char c;
		cmd_cur=0;
		cmd_mode=0;
		cmd_status=0;
		DDRC|=1<<PC5;
		spi_init_master();
		
		MAX7219_init();
		PORTC&=~(1<<PC5);
		Init();
		clearDisplay();
//		setDisplayDigit(1, 3, 0);
		uart_puts_P(PSTR("MCU"));
		uart_putln();
		time=1510841438;
//		time+=TIME_ZONE;
		NumbToUART(time);
		uart_putln();
//		time=1514764825;
//		time=1483228799;
//		time=1483228801;
//		_delay_ms(200);

			Unix2Time(time,&tm);
							setDisplayToDecNumberAt(tm.sec,0b01010100,1,2,1);
		NumbToUART(tm.sec);
		uart_putln();
							setDisplayToDecNumberAt(tm.min,0b01010100,3,2,1);
		NumbToUART(tm.min);
		uart_putln();
							setDisplayToDecNumberAt(tm.hour,0b01010100,5,2,1);
		NumbToUART(tm.hour);
		uart_putln();
							setDisplayToDecNumberAt(tm.mday,0b01010100,7,2,1);
		NumbToUART(tm.mday);
		uart_putln();
			
		NumbToUART(tm.month);
		uart_putln();
			
		NumbToUART(tm.year);
		uart_putln();

		NumbToUART(tm.wday);
		uart_putln();
		uart_putln();
	
			
		time=Time2Unix(&tm);
//		time-=TIME_ZONE;			
		NumbToUART(time);
//			NumbToUART(tmtmp.tm_year);
//			time= mktime(&tmtmp);
//			NumbToUART(time);
			uart_putln();
//		_delay_ms(200);


			Unix2Time(time,&tm);
			setDisplayToDecNumberAt(tm.sec,0b01010100,1,2,1);
			NumbToUART(tm.sec);
			uart_putln();
			setDisplayToDecNumberAt(tm.min,0b01010100,3,2,1);
			NumbToUART(tm.min);
			uart_putln();
			setDisplayToDecNumberAt(tm.hour,0b01010100,5,2,1);
			NumbToUART(tm.hour);
			uart_putln();
			setDisplayToDecNumberAt(tm.mday,0b01010100,7,2,1);
			NumbToUART(tm.mday);
			uart_putln();
			
			NumbToUART(tm.month);
			uart_putln();
			
			NumbToUART(tm.year);
			uart_putln();

			NumbToUART(tm.wday);
			uart_putln();
			uart_putln();





    while(1)
    {
				if ((uart_getc(&c))==0)
				{
					main_func(c);
					setDisplayDigit(8, c>>4, 0);
					setDisplayDigit(7, c, 0);
				}		
    }
}