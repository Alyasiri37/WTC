/*
 * spi.h
 *
 * Created: 2/4/2022 7:28:25 PM
 *  Author: Ali
 */ 


#ifndef SPI_H_
#define SPI_H_

void spi_ss_set();
void spi_ss_reset();

void spi_master_init();
char spi_master_trx(char data);

void spi_slave_init();
char spi_slave_trx(char data);



#endif /* SPI_H_ */