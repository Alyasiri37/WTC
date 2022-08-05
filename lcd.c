/*
 * lcd.c
 *
 * Created: 1/27/2022 1:57:12 PM
 *  Author: Ali
 */ 
#include <avr/io.h>
#include "timer.h"
#include "lcd.h"
#include "i2c.h"

void send_4bit(uint8_t data,uint8_t, uint8_t);	// 4bit mode, amends Rs, Rw, En and back light
void send_8bit(char data,uint8_t,uint8_t);		// 8bit mode, amends Rs, Rw, En and back light

// Some bit definitions
#define Bl 3  // Back light
#define En 2  // Enable bit
#define Rw 1  // Read/Write bit
#define Rs 0  // Register select bit

//Cursor navigation
#define ROW1 DDADDR|0
#define ROW2 DDADDR|64
#define ROW3 DDADDR|20
#define ROW4 DDADDR|84

//LCD command definitions
#define CLEAR 0x01		//Clear the display
#define HOME 0x02		//Return cursor to home position
#define ENTRY 0x04		//Entry mode: cursor moving direction and display shift
#define DISPLAY 0x08	//Display on/off control including cursor and blinking
#define CURSOR 0x10		//Cursor moving direction and display shift
#define MODE 0x20		// 4/8 bit mode, number of lines and font type
#define CGADDR 0x40		// CGram address
#define DDADDR 0x80		// DD ram address, sets cursor position

uint8_t lcd_rows[4]={ROW1, ROW2, ROW3, ROW4};	// Used to navigate screen
	
 void lcd_init()
 {
	 timer_delay_m(50); //Wait for voltage to settle
	 
	 send_4bit(0x03,0,0);//Initialization procedure. 3 tries to set 8bit mode
	 timer_delay_m(5);
	 send_4bit(0x03,0,0);
	 timer_delay_u(150);
	 send_4bit(0x03,0,0);
	 
	 send_4bit(0x02,0,0);// 4bit mode
	 
	 send_4bit(0x02,0,0);// Number of lines and character font
	 send_4bit(0x08,0,0);
	 
	 
	 send_4bit(0x00,0,0);// Display control, first 3 bits for screen, cursor and blinking
	 send_4bit(0x0c,0,0);
	 
	 
	 send_4bit(0x00,0,0);// clear the screen
	 send_4bit(0x01,0,0);
	 
	 
	 send_4bit(0x00,0,0);// entry mode
	 send_4bit(0x04,0,0);
	 
	 
 }
 
 void send_4bit(uint8_t data,uint8_t rs, uint8_t rw )
 {
	 i2c_send((data<<4)|(1<<En)|(rs<<Rs)|(rw<<Rw)|(1<<Bl));
	 
	 timer_delay_u(1);
	 
	 i2c_send((data<<4)|(0<<En)|(rs<<Rs)|(rw<<Rw)|(1<<Bl));
	 
	 timer_delay_u(50);
	 
	 
 }
 
 void send_8bit(char data,uint8_t rs, uint8_t rw){
	 
	 uint8_t UB = (data & 0xf0)>>4; // Upper bits
	 uint8_t LB = data & 0x0f; // Lower bits
	 
	 send_4bit(UB,rs,rw);
	 send_4bit(LB,rs,rw);
	 
 }

 void lcd_cmd(uint8_t cmd)
 {
	 send_8bit(cmd,0,0);
 }

 void lcd_char(char data)
 {
	 send_8bit(data,1,0);
	 
 }

 void lcd_print(char *text)
 {
	 for (; *text !='\0'; text++)
	 {
		 lcd_char(*text);
	 }
	 
 }
 
 void lcd_cell(int row, int cell){
	 
	 lcd_cmd(lcd_rows[row-1] + cell);
 }
 
 void lcd_clear()
 {
	 lcd_cmd(CLEAR);
 }