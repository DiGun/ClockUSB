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

// TIMER0 with prescaler clkI/O/256
#define TIMER0_PRESCALER (1 << CS02)

void Init()
{
	// init Timer0
	
	TIMSK |= (1 << TOIE0);        // interrupt enable - here overflow
	TCCR0 |= TIMER0_PRESCALER; // use defined prescaler value
	//разрешаем внешнее прерывание на int0
	//   GICR |= (1<<INT0)|(1<<INT1);
	//настраиваем условие прерывания
	//   MCUCR |= (1<<ISC01)|(1<<ISC00)|(1<<ISC11)|(1<<ISC10);

	uart_init();
	//	init_USART();
	_delay_ms(250);
	sei();
	_delay_ms(250);

}

volatile uint32_t time;
volatile uint8_t refresh;
uint32_t number;
uint8_t	mode;

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
	
	utc += rtc->mday - 1;

//	utc *= 86400;
//	return utc;

	utc *= 86400;
	utc += (uint32_t)rtc->hour * 3600 + (uint32_t)rtc->min * 60 + rtc->sec;

	utc -= (uint32_t)TIME_ZONE;
	return utc;
}

/*------------------------------------------*/
/* Get time in calendar form                */
/*------------------------------------------*/

void Unix2Time(uint32_t utc, RTCTIME *rtc)
{
	uint32_t n,i,d;
	utc += (uint32_t)TIME_ZONE;
	/* Compute  hours */

	rtc->sec = (uint8_t)(utc % 60); utc /= 60;
	rtc->min = (uint8_t)(utc % 60); utc /= 60;
	rtc->hour = (uint8_t)(utc % 24); utc /= 24;
	rtc->wday = (uint8_t)((utc + 4) % 7);
	rtc->year = (uint16_t)(1970 + utc / 1461 * 4); utc %= 1461;
	n = ((utc >= 1096) ? utc - 1 : utc) / 365;
	rtc->year += n;
	utc -= n * 365 + (n > 2 ? 1 : 0);
	for (i = 0; i < 12; i++) 
	{
		d = numofdays[i];
		if (i == 1 && n == 2) d++;
		if (utc < d) break;
		utc -= d;
	}
	rtc->month = (uint8_t)(1 + i);
	rtc->mday = (uint8_t)(1 + utc);
}

//uint8_t cmd_cur;
uint32_t cmd_get_num;//полученный номер
uint8_t cmd_mode; //текущий режим
uint8_t cmd_type; //тип команды
uint8_t cmd_status;//статус текущей команды


inline void func_hello(void)
{
	uart_putc_w('A');
}

void func_error(void)
{
	uart_putc_w('E');
	cmd_mode=0;
	cmd_status =0;
	cmd_type=0;
}

inline void func_ok(void)
{
	cmd_mode=1;
	cmd_status =0;
	cmd_type=0;
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

void func_get(char c)
{
	switch (c)
	{
		case 'T':	//timestamp
			uart_putc_w('A');
			uart_putc_w('T');
			NumbToUART(time);
			uart_putc_w('D');
			NumbToUART(time);
			uart_putln();
			func_ok();
		break;

		case 'M':	//mode
			uart_putc_w('A');
			uart_putc_w('M');
			NumbToUART(mode);
			uart_putc_w('D');
			NumbToUART(mode);
			uart_putln();
			func_ok();
		break;
		case 'A':	//get
		func_error();
		//		cmd_type=c;
		break;
		default:
		func_error();
	}
//	cmd_type=0;
}

int8_t get_str_num(char c)
{
	static char buf[11];//текстовы числовой параметр
	static uint8_t len; //длина
	static uint8_t dup; //дибликат
	if (c>'9' || c<'0')
	{
		if (len!=0)
		{
			buf[len]=0;
			len=0;

			uint32_t t_num=strtoul(buf,NULL,10);;
			if (dup)
			{
				if (t_num!=cmd_get_num)
				{
					dup=0;
					return -1;
					
				}
			}
			cmd_get_num=t_num;
		}
		switch (c)
		{
		case 13:
		case 10:
			dup=0;
			return 1;
		case 'D':
			dup=1;
			break;
		default:
			len=0;
			dup=0;
			return -1;
		}
	}
	else
	{
		if (len!=10)
		{
			buf[len]=c;
			len++;
		}
		else
		{
//			PORTC|=1<<PC5;			
			len=0;
			dup=0;
			func_error();
		}
	}
	return 0;	
}

void func_set(char c)
{
	if (cmd_status)
	{
		switch (get_str_num(c))
		{
		case 0:
		break;
		case 1:
			switch (cmd_status)
			{
			case 'T':
				time=cmd_get_num;
				break;
			case 'M':
				mode=cmd_get_num;
				break;
			case 'I':
				MAX7219_writeData(0x0A, cmd_get_num);//Яркость
				break;
			case 'L':
				if(cmd_get_num)
				{
					PORTC|=(1<<PC5);
				}
				else
				{
					PORTC&=~(1<<PC5);
				}
				break;
			case 'N':
				number=cmd_get_num;
			break;
			}

			uart_putln();
			uart_putc_w('R');
			NumbToUART(cmd_get_num);
			uart_putln();
			func_ok();
		break;
		case -1:
			func_error();
		break;
		}
	}
	else
	{
		switch (c)
		{
			case 'T':	//timestamp 
			case 'M':	//mode
			case 'I':	//intensivity
			case 'L':	//led
			case 'N':	//number
			cmd_status = c;
			break;

			case 'A':	//get
			func_error();
			//		cmd_type=c;
			break;
			default:
			func_error();
		}
	}
}

// cmd 
//Q - start
//[S,G]	- Set,Get

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
			func_get(c);
			break;
		case 'S':
			func_set(c);
			refresh=1;
			break;
		}
	break;
		
	}
	
}

#define STEP2SECUNDA	225
ISR(TIMER0_OVF_vect)
{
	static uint8_t cnt_secunda=STEP2SECUNDA;
	if (!(--cnt_secunda))
	{
		time++;
		refresh=1;
		cnt_secunda=STEP2SECUNDA;
//		PORTC^=1<<PC5;
	}
}	



void print_time()
{
	Unix2Time(time,&tm);
	setDisplayToDecNumberAt(tm.sec,0b00000000,1,2,1);
	sendChar(3, 1, 0);
	setDisplayToDecNumberAt(tm.min,0b00000000,4,2,1);
	sendChar(6, 0b00000001, 0);
	setDisplayToDecNumberAt(tm.hour,0b00000000,7,2,1);
}

void print_date()
{
	Unix2Time(time,&tm);
	setDisplayToDecNumberAt(tm.mday,0b00010100,1,2,1);
	setDisplayToDecNumberAt(tm.month,0b00010100,3,2,1);
	setDisplayToDecNumberAt(tm.year,0b00010100,5,4,1);
}

inline void print_numb()
{
	setDisplayToDecNumberAt(number,0b00000000,1,8,0);
}


char c;
int main(void)
{
		
//		cmd_cur=0;
		mode=1;
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
//		time=1514764825;
		time=1510841438;
		number =0;
//		time+=TIME_ZONE;

    while(1)
    {
				if ((uart_getc(&c))==0)
				{
					main_func(c);
//					setDisplayDigit(8, c>>4, 0);
//					setDisplayDigit(7, c, 0);
				}
				if (refresh)
				{
					switch (mode)
					{
					case 1:
						print_time();
						break;
					case 2:
						print_date();
					break;
					case 3:
						clearDisplay();
					break;
					case 4:
						print_numb();
					break;
					}
					refresh=0;
				}
    }
}