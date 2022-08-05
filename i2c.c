/*
 * i2c.c
 *
 * Created: 2/4/2022 1:43:16 PM
 *  Author: Ali
 */ 

#include <avr/io.h>
#include <util/twi.h>
#include "timer.h"

void start_twi(uint8_t addr){
	
	TWBR=0x0a;		//pre-scaler setting
	
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	
	while(!(TWCR & (1<<TWINT)))
	;
	
	if((TWSR & 0xF8) !=TW_START);
	//  Some error checking required
	
	//Write SLA+W (7bit address + R/W flag) to the TWDR and send it
	TWDR = ((addr & 0x7f)<<1);
	
	TWCR = (1<<TWINT) | (1<<TWEN);// To be checked
	while (!(TWCR & (1<<TWINT)))
	;
	if((TWSR & 0xF8) != TW_MT_SLA_ACK);
	//  Some error checking required
	
}

void i2c_send(uint8_t data)
{
	TWDR = data;
	
	TWCR = (1<<TWINT) | (1<<TWEN);
	
	timer_delay_u(1);
	
	while (!(TWCR & (1<<TWINT)))
	;
	if((TWSR & 0xF8) != TW_MT_DATA_ACK);
	
}

void stop_twi(){
	TWCR=(1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}