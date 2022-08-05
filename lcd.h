/*
 * lcd.h
 * This header file is meant for use along with I2C protocol. 
 * Future versions will make separate functionalities for for parallel setup.
 * Created: 1/29/2022 3:14:10 PM
 *  Author: Ali
 */ 


#ifndef LCD_H_
#define LCD_H_


//LCD command parameters



//LCD functions


void lcd_init();								// Soft initialize LCD
void lcd_char(char data);				
void lcd_print(char *text);
void lcd_cell(int row, int cell);
void lcd_clear();
#endif /* LCD_H_ */