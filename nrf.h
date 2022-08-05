/*
 * nrf.h
 * A dedicated library for the nRF24L01+ wireless chip
 * Created: 2/7/2022 2:54:03 PM
 *  Author: Ali
 */ 


#ifndef NRF_H_
#define NRF_H_

//Bit definitions

/* Configuration register bits */
#define MASK_RX_DR			6
#define MASK_TX_DS			5
#define MASK_MAX_RT			4
#define EN_CRC				3
#define CRCO				2
#define PWR_UP				1
#define PRIM_RX				0

/* Enhanced ShockBurst auto acknowledge register bits */
#define ENAA_P5				5
#define ENAA_P4				4
#define ENAA_P3				3
#define ENAA_P2				2
#define ENAA_P1				1
#define ENAA_P0				0

/* RX pipe enable bits */
#define ERX_P5				5
#define ERX_P4				4
#define ERX_P3				3
#define ERX_P2				2
#define ERX_P1				1
#define ERX_P0				0

/* RF setup register register bits */
#define CONT_WAVE			7
#define RF_DR_LOW			5
#define PLL_LOCK			4
#define RF_DR_HIGH			3

/* Status register bits */
#define RX_DR				6
#define TX_DS				5
#define MAX_RT				4
#define TX_FULL				0

/* Received power detector */


/* FIFO status register bits */
#define TX_REUSE			6
#define TX_FULL				5
#define TX_EMPTY			4
#define RX_FULL				1
#define RX_EMPTY			0

/* Dynamic payload register bits */
#define DPL_P5				5
#define DPL_P4				4
#define DPL_P3				3
#define DPL_P2				2
#define DPL_P1				1
#define DPL_P0				0

/* Feature register bits */
#define EN_DPL				2
#define EN_ACK_PAY			1
#define EN_DYN_ACK			0

//Functions


void nrf_init(char mode, char *tx_addr_p, char *rx_addr_p);		/* Function definitions */

void nrf_config(uint8_t rx_int, uint8_t tx_int, uint8_t maxrt_int,
				uint8_t en_crc, uint8_t crc_bytes);				/*Edits the CONFIG register*/
				
void nrf_setup(uint8_t addr_len, uint8_t channel, uint8_t data_rate,
				uint8_t power);									/*RF setup*/
void nrf_retransmit_setup(uint8_t ard, uint8_t arc);
void nrf_up();
void nrf_down();
void nrf_start_tx(uint8_t *tx_addr_p, uint8_t *rx_addr_p, uint8_t pipe);
void nrf_start_rx(char *rx_addr_p, uint8_t pipe, uint8_t payload_length);
int nrf_transmit();												/*Activate data transmission*/
void nrf_receive(char *arr, uint8_t payload);
char nrf_rr(char addr);											/*Read register command*/
char nrf_wr(char addr, char value);								/*Write register command*/
void nrf_r_addr(char *arr, char addr);							/*Read address bytes, have to be modified to use pointers*/
void nrf_wr_addr(char *arr ,char addr);							/*Write address bytes, have to be modified to use pointers*/
void nrf_r_payload(char *arr, uint8_t payload);									/*Read payload*/
void nrf_wr_payload(char *arr, uint8_t payload);									/*Write payload*/
void nrf_flush_tx();											/*Flush TX FIFO in TX mode*/
void nrf_flush_rx();											/*Flush RX FIFO in RX mode*/

void nrf_const_carrier(char set_reset);
int nrf_carrier_detect();


#endif /* NRF_H_ */