/*
 * timer.c
 *
 * Created: 2/12/2022 1:40:57 AM
 *  Author: Ali
 */ 
#include <avr/io.h>


void timer_delay_m(unsigned int millisec)
{
	TCCR0A = (1<<WGM01);			/* CTC */
	TCCR0B = (1<<CS01)|(1<<CS00);	/* clock/1024 */
	OCR0A  = 249;					/*Count for 1 millisecond*/					
									
	for (int i = 0; i < millisec ; i++)
	{
			
			while((TIFR0 & (1<<OCF0A))==0);
			
			TIFR0 = (1<<OCF0A);
		
	}
	TCCR0A = 0;
	TCCR0B = 0;
}

void timer_delay_u(unsigned int microsec)
{
	
	TCCR0A = (1<<WGM01);			/* CTC */
	TCCR0B = (1<<CS00);				/* clock/1 */
	OCR0A  = 15;					/*Count for 1 microsecond*/
		
	for (int i = 0; i < microsec ; i++)
	{
			
		while((TIFR0 & (1<<OCF0A))==0);
			
		TIFR0 = (1<<OCF0A);
			
	}
	TCCR0A = 0;
	TCCR0B = 0;
	
}