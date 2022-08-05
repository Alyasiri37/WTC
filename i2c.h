/*
 * i2c.h
 *
 * Created: 1/27/2022 1:56:37 PM
 *  Author: Ali
 */ 


#ifndef I2C_H_
#define I2C_H_

//Function definitions for I2C

void start_twi(uint8_t addr);	//setup the bus and send send start condition
void i2c_send(uint8_t data);	//Send a byte of data
void stop_twi();				//Stop transmission







#endif /* I2C_H_ */