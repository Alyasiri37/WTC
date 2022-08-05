/*
 * spi.c
 *SPI is not affected by delay functions, I DONT KNOW WHY
 * Created: 2/4/2022 7:28:34 PM
 *  Author: Ali
 */ 
#include <avr/io.h>
#include "spi.h"

/*Initialize master mode*/
void spi_master_init()
{
	DDRB |= (1<<DDB2) | (1<<DDB1) | (1<<DDB0); //MOSI ,SS and SCK output, 
	DDRB &= ~(1<<DDB3); //MISO input
	PORTB |= (1<<PB0); //Sets SS high, use it when u need it otherwise ground slave SS 
	
	SPCR = (1<<SPE) | (1<<MSTR) |(1<SPR0); //Enable SPI, master, set clock rate to f/16
	SPSR &= ~(1<<SPI2X); //Disable speed doubler
	
}

void spi_ss_set()
{
	PORTB|=(1<<PB0);
}

void spi_ss_reset()
{
	PORTB &= 0xfe;
}

/*master transceive function, no need for separate functions as it does the job*/
char spi_master_trx(char data)
{
	SPDR =data;	
	while(!(SPSR & (1<<SPIF)))
	;
	return SPDR;
	
}



void spi_slave_init()
{
	DDRB &= ~((1<<DDB2) | (1<<DDB1) | (1<<DDB0));
	DDRB |= (1<<DDB3); //MISO output, others input
	SPCR = (1<<SPE); //Enable SPI
}

char spi_slave_trx(char data)
{
	
	SPDR=data;
	while(!(SPSR & (1<<SPIF)))
	;
	return SPDR;
}

