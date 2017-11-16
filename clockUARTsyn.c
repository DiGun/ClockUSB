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

#define TIME_ZONE -2 * 60*60

struct tm {
	int8_t tm_sec;
	int8_t tm_min;
	int8_t tm_hour;
	int8_t tm_mday;
	int8_t tm_mon;
	int16_t tm_year;
	int8_t tm_wday;
	int16_t tm_yday;
	int8_t tm_isdst;
};

struct tm tmtmp;


#define YEAR0                   1900
#define EPOCH_YR                1970
#define SECS_DAY                (24L * 60L * 60L)
#define LEAPYEAR(year)          (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)          (LEAPYEAR(year) ? 366 : 365)

#define TIME_MAX                2147483647L
#define	_timezone	TIME_ZONE
#define	_daylight	0
#define	_dstbias	0

// long _timezone = 0;                 // Difference in seconds between GMT and local time
// int _daylight = 0;                  // Non-zero if daylight savings time is used
// long _dstbias = 0;                  // Offset for Daylight Saving Time

const int _ytab[2][12] = {
{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

struct tm gmtime(uint32_t timer)
{
	uint32_t time = timer;
	unsigned long dayclock, dayno;
	struct tm tmbuf;
	
	int year = EPOCH_YR;

	dayclock = (unsigned long) time % SECS_DAY;
	dayno = (unsigned long) time / SECS_DAY;

	tmbuf.tm_sec = dayclock % 60;
	tmbuf.tm_min = (dayclock % 3600) / 60;
	tmbuf.tm_hour = dayclock / 3600;
	tmbuf.tm_wday = (dayno + 4) % 7; // Day 0 was a thursday
	while (dayno >= (unsigned long) YEARSIZE(year)) 
	{
		dayno -= YEARSIZE(year);
		year++;
	}
	tmbuf.tm_year = year ; //- YEAR0;
	tmbuf.tm_yday = dayno;
	tmbuf.tm_mon = 1;
	while (dayno >= (unsigned long) _ytab[LEAPYEAR(year)][tmbuf.tm_mon]) 
	{
		dayno -= _ytab[LEAPYEAR(year)][tmbuf.tm_mon];
		tmbuf.tm_mon++;
	}
	tmbuf.tm_mday = dayno;
	setDisplayToDecNumberAt(tmbuf.tm_mday,0b01010100,3,2,1);
	setDisplayToDecNumberAt(tmbuf.tm_mon,0b01010100,5,2,1);

	
	
	
	tmbuf.tm_isdst = 0;

	return tmbuf;
}

uint32_t mktime(struct tm *tmbuf) {
	long day, year;
	int tm_year;
	int yday, month;
	/*unsigned*/ long seconds;
	int overflow;
	long dst;

	tmbuf->tm_min += tmbuf->tm_sec / 60;
	tmbuf->tm_sec %= 60;
	if (tmbuf->tm_sec < 0) {
		tmbuf->tm_sec += 60;
		tmbuf->tm_min--;
	}
	tmbuf->tm_hour += tmbuf->tm_min / 60;
	tmbuf->tm_min = tmbuf->tm_min % 60;
	if (tmbuf->tm_min < 0) {
		tmbuf->tm_min += 60;
		tmbuf->tm_hour--;
	}
	day = tmbuf->tm_hour / 24;
	tmbuf->tm_hour= tmbuf->tm_hour % 24;
	if (tmbuf->tm_hour < 0) {
		tmbuf->tm_hour += 24;
		day--;
	}
	tmbuf->tm_year += tmbuf->tm_mon / 12;
	tmbuf->tm_mon %= 12;
	if (tmbuf->tm_mon < 0) {
		tmbuf->tm_mon += 12;
		tmbuf->tm_year--;
	}
	day += (tmbuf->tm_mday - 1);
	while (day < 0) {
		if(--tmbuf->tm_mon < 0) {
			tmbuf->tm_year--;
			tmbuf->tm_mon = 11;
		}
		day += _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
	}
	while (day >= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon]) {
		day -= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
		if (++(tmbuf->tm_mon) == 12) {
			tmbuf->tm_mon = 0;
			tmbuf->tm_year++;
		}
	}
	tmbuf->tm_mday = day + 1;
	year = EPOCH_YR;
	if (tmbuf->tm_year < year - YEAR0) return (uint32_t) -1;
	seconds = 0;
	day = 0;                      // Means days since day 0 now
	overflow = 0;

	// Assume that when day becomes negative, there will certainly
	// be overflow on seconds.
	// The check for overflow needs not to be done for leapyears
	// divisible by 400.
	// The code only works when year (1970) is not a leapyear.
	tm_year = tmbuf->tm_year + YEAR0;

	if (TIME_MAX / 365 < tm_year - year) overflow++;
	day = (tm_year - year) * 365;
	if (TIME_MAX - day < (tm_year - year) / 4 + 1) overflow++;
	day += (tm_year - year) / 4 + ((tm_year % 4) && tm_year % 4 < year % 4);
	day -= (tm_year - year) / 100 + ((tm_year % 100) && tm_year % 100 < year % 100);
	day += (tm_year - year) / 400 + ((tm_year % 400) && tm_year % 400 < year % 400);

	yday = month = 0;
	while (month < tmbuf->tm_mon) {
		yday += _ytab[LEAPYEAR(tm_year)][month];
		month++;
	}
	yday += (tmbuf->tm_mday - 1);
	if (day + yday < 0) overflow++;
	day += yday;

	tmbuf->tm_yday = yday;
	tmbuf->tm_wday = (day + 4) % 7;               // Day 0 was thursday (4)

	seconds = ((tmbuf->tm_hour * 60L) + tmbuf->tm_min) * 60L + tmbuf->tm_sec;

	if ((TIME_MAX - seconds) / SECS_DAY < day) overflow++;
	seconds += day * SECS_DAY;

	// Now adjust according to timezone and daylight saving time
	if (((_timezone > 0) && (TIME_MAX - _timezone < seconds)) ||
	((_timezone < 0) && (seconds < -_timezone))) {
		overflow++;
	}
	seconds += _timezone;

	if (tmbuf->tm_isdst) {
		dst = _dstbias;
	} else {
		dst = 0;
	}

	if (dst > seconds) overflow++;        // dst is always non-negative
	seconds -= dst;

	if (overflow) return (uint32_t) -1;

	if ((uint32_t) seconds != seconds) return (uint32_t) -1;
	return (uint32_t) seconds;
}

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
		setDisplayDigit(1, 3, 0);
//		uart_puts_P(PSTR("MCU"));
//		uart_putln();
//		time=1510824697;
//		time=1514764825;
//		time=1483228799;
		time=1483228801;
//		NumbToUART(time);
//		uart_putln();
//		_delay_ms(200);

			tmtmp =gmtime(time);
//			NumbToUART(tmtmp.tm_year);
//			time= mktime(&tmtmp);
//			NumbToUART(time);
//			uart_putln();
//		_delay_ms(200);
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