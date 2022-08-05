/*
 * nrf.c
 * A dedicated library for the nRF24L01+ wireless chip
 * Created: 2/7/2022 2:53:50 PM
 *  Author: Ali
 */ 

#include <avr/io.h>
#include "timer.h"
#include "spi.h"
#include "nrf.h"

// nRF SPI commands
#define R_REGISTER			0x00
#define W_REGISTER			0x20
#define R_RX_PAYLOAD		0x61
#define W_TX_PAYLOAD		0xa0
#define FLUSH_TX			0xe1
#define FLUSH_RX			0xe2
#define REUSE_TX_PL			0xe3
#define R_RX_PL_WID			0x60
#define W_ACK_PAYLOAD		0xa8
#define W_TX_PAYLOAD_NOACK	0xb0
#define NOP					0xff

//Register map
#define CONFIG				0x00
#define EN_AA				0x01
#define EN_RXADDR			0x02
#define SETUP_AW			0x03
#define SETUP_RETR			0x04
#define RF_CH				0x05
#define RF_SETUP			0x06
#define STATUS				0x07
#define OBSERVE_TX			0x08
#define RPD					0x09
#define RX_ADDR_P0			0x0A
#define RX_ADDR_P1			0x0B
#define RX_ADDR_P2			0x0C
#define RX_ADDR_P3			0x0D
#define RX_ADDR_P4			0x0E
#define RX_ADDR_P5			0x0F
#define TX_ADDR				0x10
#define RX_PW_P0			0x11
#define RX_PW_P1			0x12
#define RX_PW_P2			0x13
#define RX_PW_P3			0x14
#define RX_PW_P4			0x15
#define RX_PW_P5			0x16
#define FIFO_STATUS			0x17
#define DYNPD				0x1C
#define FEATURE				0x1D

// Pin definitions
#define	CE_PIN PC0
#define CSN_PIN PC1

#define	IRQ_PIN PB4 /*Change it somewhere on portB, interrupt initialization is done in the main program
						Should be cleaned after testing*/


/*Generic definitions*/
#define ADDR_LENGTH		5			/*Specifies address length for the transceiver*/
//#define PAYLOAD_LENGTH	2			/*Data length*/
//#define CHANNEL			0x02
//#define RF_PWR			0x03		//0 dBm

/*Configuration and setup variables*/
uint8_t setup, config;		/*CONFIG and RF_SETUP register holders*/


					

/* Function definitions */

/*configure the chip*/
void nrf_config(uint8_t rx_int, uint8_t tx_int, uint8_t maxrt_int, uint8_t en_crc, uint8_t crc_bytes)
{
				 
	 config = (rx_int<<MASK_RX_DR)|(tx_int<<MASK_TX_DS)|(maxrt_int<<MASK_MAX_RT)
					|(en_crc<<EN_CRC)|(crc_bytes<<CRCO);
					
	nrf_wr(CONFIG, config);				/*Write the config register*/
	
}

/*Setup wireless parameters*/
void nrf_setup(uint8_t addr_len, uint8_t channel, uint8_t data_rate, uint8_t power)
{


	 				/*RF setup*/
	
	addr_len -= 2;					/*Adjust SETUP_AW bits*/
	nrf_wr(SETUP_AW , addr_len);	/* Set address length to 5 bytes*/
	
	nrf_wr(RF_CH , channel);		/* Set RF channel */
	
	switch(data_rate)
	{
		case 0:						/* 1 Mbps */
		setup = (0<<RF_DR_LOW)|(0<<RF_DR_HIGH);
		break;
		
		case 1:						/*2 Mbps */
		setup = (0<<RF_DR_LOW)|(1<<RF_DR_HIGH);
		break;
		
		case 2:						/*250 kbps */
		setup = (1<<RF_DR_LOW)|(0<<RF_DR_HIGH);
		break;
		
		default:
		setup = (0<<RF_DR_LOW)|(0<<RF_DR_HIGH);
		break;
	}
	
	setup |= (power<<1);			/*Add power bits: 0:-18 dBm, 1:-12dBm, 2:-6dBm, 11:0dBm*/
	nrf_wr(RF_SETUP,setup);			/*Setup RF*/
	
	
}

void nrf_retransmit_setup(uint8_t ard, uint8_t arc)
{
	nrf_wr(SETUP_RETR, ((ard<<4)|arc));
}

/*Power up the chip to enter standby mode*/

void nrf_up()
{
	config |= (1<<PWR_UP);
	nrf_wr(CONFIG, config);
	timer_delay_m(5);				/*Wait for the chip to stabilize after power up*/
}


/*Power down the chip to enter standby mode*/

void nrf_down()
{
	config &= ~(1<<PWR_UP);
	nrf_wr(CONFIG, config);
}

void nrf_start_tx(uint8_t *tx_addr_p, uint8_t *rx_addr_p, uint8_t pipe)
{
	
	config &= ~(1<<PRIM_RX);				/*PTX mode*/
	nrf_wr(CONFIG, config);
	
	/*	Enable auto ack on data pipe
		Set the ack node address	*/
	
	switch(pipe)
	{
		case 0:
		nrf_wr(EN_AA,(1<<ENAA_P0));
		nrf_wr_addr(rx_addr_p,RX_ADDR_P0);
		break;
		
		case 1:
		nrf_wr(EN_AA,(1<<ENAA_P1));
		nrf_wr_addr(rx_addr_p,RX_ADDR_P1);
		break;
		
		case 2:
		nrf_wr(EN_AA,(1<<ENAA_P2));
		nrf_wr(RX_ADDR_P2,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		case 3:
		nrf_wr(EN_AA,(1<<ENAA_P3));
		nrf_wr(RX_ADDR_P3,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
		case 4:
		nrf_wr(EN_AA,(1<<ENAA_P4));
		nrf_wr(RX_ADDR_P4,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
		case 5:
		nrf_wr(EN_AA,(1<<ENAA_P5));
		nrf_wr(RX_ADDR_P5,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
	}
	
	nrf_wr_addr(tx_addr_p ,TX_ADDR);		/*Set the receiving node address*/
	
	//
	PORTC |= (1<<CE_PIN);
	timer_delay_u(15);
	PORTC &= ~(1<<CE_PIN);
	
}

void nrf_start_rx(char *rx_addr_p, uint8_t pipe, uint8_t payload_length)
{
	config |= (1<<PRIM_RX);					/* PRX mode */
	nrf_wr(CONFIG, config);	
	
	switch(pipe)
	{
		case 0:
		nrf_wr(EN_RXADDR,(1<<pipe));			/*Enable RX data pipe */
		nrf_wr(EN_AA,(1<<ENAA_P0));				/*Enable auto ack on data pipe 0*/
		nrf_wr(RX_PW_P0, payload_length);		/*Set payload length*/
		nrf_wr_addr(rx_addr_p,RX_ADDR_P0);
		break;
		
		case 1:
		nrf_wr(EN_RXADDR,(1<<pipe));			
		nrf_wr(EN_AA,(1<<ENAA_P1));	
		nrf_wr(RX_PW_P1, payload_length);			
		nrf_wr_addr(rx_addr_p,RX_ADDR_P1);
		break;
		
		case 2:
		nrf_wr(EN_RXADDR,(1<<pipe));
		nrf_wr(EN_AA,(1<<ENAA_P2));
		nrf_wr(RX_PW_P2, payload_length);
		nrf_wr(RX_ADDR_P2,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
		case 3:
		nrf_wr(EN_RXADDR,(1<<pipe));
		nrf_wr(EN_AA,(1<<ENAA_P3));
		nrf_wr(RX_PW_P3, payload_length);
		nrf_wr(RX_ADDR_P3,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
		case 4:
		nrf_wr(EN_RXADDR,(1<<pipe));
		nrf_wr(EN_AA,(1<<ENAA_P4));
		nrf_wr(RX_PW_P4, payload_length);
		nrf_wr(RX_ADDR_P4,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
		case 5:
		nrf_wr(EN_RXADDR,(1<<pipe));
		nrf_wr(EN_AA,(1<<ENAA_P5));
		nrf_wr(RX_PW_P5, payload_length);
		nrf_wr(RX_ADDR_P5,*(rx_addr_p + (ADDR_LENGTH-1)));
		break;
		
	}				
	
	
	PORTC |= (1<<CE_PIN);					/*Start listening */
	timer_delay_u(200);
	
}

int nrf_transmit()						/*Activate data transmission*/
{
	PORTC |= (1<<CE_PIN);				/*Initiate transmission*/
	//takes around 15 us
	timer_delay_u(15);
	
	// Implement interrupt to save CPU time. Entering idle state to be moved to ISR
	while (nrf_rr(STATUS)&(1<<TX_DS)==0)
	;
	// Error checking functionality?
	
	nrf_wr(STATUS,(0x70));
	PORTC &= ~(1<<CE_PIN);				/*Transfer back to idle state 1. to be moved to ISR*/
}

void nrf_receive(char *arr,uint8_t payload)
{
	if (nrf_rr(STATUS)&(1<<RX_DR))
	{
		nrf_r_payload(arr, payload);
		
		//nrf_wr(STATUS, (1<<RX_DR));			/*Clear the interrupt flag*/
	}
}

/*Read register command*/
char nrf_rr(char addr)
{
	char buffer;
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	char cmd = R_REGISTER | (addr & 0x1F);
	spi_master_trx(cmd);
	buffer = spi_master_trx(NOP);
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
	return buffer;
}

/*Write register command*/
char nrf_wr(char addr, char value)
{
	char buffer;
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	char cmd = W_REGISTER | (addr & 0x1F);
	buffer = spi_master_trx(cmd);
	spi_master_trx(value);
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
	return buffer;
}

/*Read address bytes, have to be modified to use pointers*/

void nrf_r_addr(char *arr, char addr)
{
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	char cmd = W_REGISTER | (addr & 0x1F);
	spi_master_trx(cmd);
	
	for ( int i=0 ; i< ADDR_LENGTH ; i++  )
	{
		*(arr+i) = spi_master_trx(NOP);
		
	}
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/

}

/*Write address bytes, have to be modified to use pointers*/
void nrf_wr_addr(char *arr ,char addr)
{
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	char cmd = W_REGISTER | (addr & 0x1F);
	spi_master_trx(cmd);
	
	for ( int i=0 ; i<ADDR_LENGTH;i++  )
	{
		spi_master_trx(*(arr+i));
	}
	
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
}
/*Read payload*/

void nrf_r_payload(char *arr, uint8_t payload)
{
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	spi_master_trx(R_RX_PAYLOAD);
	
	for ( int i=0 ; i<payload ; i++  )
	{
		*(arr+i) = spi_master_trx(NOP);
	}
	
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
	

}

/*Write payload*/

void nrf_wr_payload(char *arr, uint8_t payload)
{
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	spi_master_trx(W_TX_PAYLOAD);
	
	for ( int i=0 ; i<payload;i++  )
	{
		spi_master_trx(*(arr+i));
		
	}
	
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
}

/*Flush TX FIFO in TX mode*/
void nrf_flush_tx()
{
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	spi_master_trx(FLUSH_TX);
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
}

/*Flush RX FIFO in RX mode*/
void nrf_flush_rx()
{
	PORTC &= ~(1<<CSN_PIN);					/*Pull CSN down*/
	spi_master_trx(FLUSH_RX);
	PORTC |= (1<<CSN_PIN);					/*Pull CSN up*/
}

/*Set the function to transmit a continuous carrier for testing*/

void nrf_const_carrier(char set_reset)
{ 
	char setup = nrf_rr(RF_SETUP);
	
	
	switch(set_reset)
	{
	case 0:												/*Reset testing parameters*/
	setup &= ~((1<<CONT_WAVE)|(1<<PLL_LOCK));
	nrf_wr(RF_SETUP,setup);
	PORTC &= ~(1<<CE_PIN);								/*Transfer back to idle state 1*/
	break;
	
	case 1:												/*Set testing parameters*/
	setup |= ((1<<CONT_WAVE)|(1<<PLL_LOCK));
	nrf_wr(RF_SETUP,setup);
	PORTC |= (1<<CE_PIN);								/*Initiate transmission*/
	break;
	}
}

/*Not needed really*/
int nrf_carrier_detect()
{
	return nrf_rr(RPD);
}

