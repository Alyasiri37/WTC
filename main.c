/*
 * Level control software for the tank level monitoring and motor control
 *
 * Created: 1/27/2022 1:55:55 PM
 * Author : Ali
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "i2c.h"
#include "lcd.h"
#include "nrf.h"
#include "spi.h"
#include "timer.h"

//Testing area

//ENDOF testing area

//IRQ signals
#define UP		(~PINB & 0x80)
#define ENTER	(~PINB & 0x40)
#define DOWN    (~PINB & 0x20)
#define DATA	(~PINB & 0x10)

//Pin definitions
//nRF pins
#define	CE_PIN	PC0
#define CSN_PIN PC1
#define	IRQ_PIN PB4

/*Transmission parameters*/
#define ADDR_LEN	5					/*nRF address length in bytes*/
#define CHANNEL		2					/*Transmission channel*/
#define DATA_RATE	0					/*Transmission rate. 0: 1Mbps, 1: 2Mbps, 2: 250kbps*/
#define POWER		3					/*Transmission power. 0: -18 dBm, 1: -12dBm, 2: -6dBm, 11: 0dBm*/
#define PAYLOAD		2					/*Payload length*/
#define RX_PIPE		0					/*RX pipe*/
#define TX_PIPE		0

char rp_buffer[32], wp_buffer[32];		/*Buffers to read and write payload data */

/*Addresses*/

char controller_addr[5]={0x50,0x50,0x50,0x50,0x50};
char sensor_addr[5]={0x51,0x51,0x51,0x51,0x51};
char motor_addr[5]={0x52,0x52,0x52,0x52,0x52};
		
//Settings data store
uint8_t settings[8]={30,80,10,0,100,0,9,1};
enum settings_tags {motor_on_level ,motor_off_level, l_full, l_empty, range, alarm_on , h_alarm, l_alarm};

//Current reading holder
uint8_t reading=25;
uint16_t readings[3]={0,50,200};	
union {
	char data[2];
	uint16_t reading;
}holder;
	
// Menu struct definitions
struct node {
	
	//Add a pointer to settings entries. Settings array or structure have to be initialized first.
	// Use and enum as an easy-to-read indexer, like: H-LEVEL, RANGE, etc..
	char *caption;
	uint8_t type;
	void *set_val; //This is a pointer to settings array entries
};

struct sub_node {
	char *caption;
	struct node *child[5];
	
};

struct main_node {
	char *caption;
	struct sub_node *child[5];
	
};

// Enumerations for value and type definitions
enum input {progress, remote, radio, text};
enum buttons {enter=1, up=2, down=3};

struct node ext_n={.caption="Back"};
	
struct node add_device={.caption="Add device" };
struct node on_level={.caption="Motor-on level",.type=progress,.set_val=&settings[motor_on_level]};
struct node off_level={.caption="Motor-off level",.type=progress,.set_val=&settings[motor_off_level]};

struct node full_lvl={.caption="Full Level",.type=remote ,.set_val=&readings[1]};
struct node empty_lvl={.caption="Empty Level",.type=remote,.set_val=&readings[2]};

struct node man_high_lvl={.caption="High Level",.type=progress,.set_val=&settings[l_full]};
struct node man_low_lvl={.caption="Low Level",.type=progress,.set_val=&settings[l_empty]};

struct node alm_on={.caption="Alarm ON/OFF",.type=radio,.set_val=&settings[alarm_on]};
struct node h_alm={.caption="H Level alarm",.type=progress,.set_val=&settings[h_alarm]};
struct node l_alm={.caption="L level alarm",.type=progress,.set_val=&settings[l_alarm]};

struct sub_node ext={.caption="Exit"};
struct sub_node setup_={.caption="Setup", .child={&ext_n, &add_device, &on_level, &off_level, 0}};
struct sub_node calib={.caption="Calibration",.child={&ext_n, &full_lvl,&empty_lvl, 0, 0}};
struct sub_node man_calib={.caption="Manual Calibration",.child={&ext_n, &man_high_lvl, &man_low_lvl, 0, 0}};
struct sub_node alarms={.caption="Alarms", .child={&ext_n, &alm_on, &h_alm, &l_alm, 0}};

	
struct main_node main_menu={.caption="Main menu", .child={&ext, &alarms, &setup_, &calib, &man_calib }};

//Generic variables
uint8_t depth=0, cursor=0,menu_ll=0, btn_pressed=0, btn_flag=0, data_flag=0;
uint8_t menu_size = sizeof(main_menu.child)/sizeof(main_menu.child[0]);
struct sub_node *menu_ptr = 0;
struct node     *node_ptr = 0;

// Motor command
uint8_t motor_control=0;

/*Function definitions*/

//HMI controls
void progress_bar(float value, float scale);
void radio_btn(uint8_t);

//Button actions
void btn_down();
void btn_up();
void btn_enter();

//Screens
void main_scr();
void opt_scr();
void setting_scr();
void edit_scr(struct node *node_ptr);
void update_screen(uint8_t screen);
void int16print(uint16_t number);
void byte2char(uint16_t bite);
//Control and data acquisition

uint8_t read_data();
void control(uint8_t current);


//Menu definitions
//char *menu_items[]={"H LEVEL","L LEVEL","ARLARM","MANUAL MODE","EXIT"};
//uint8_t menu_size = (sizeof(menu_items)/sizeof(menu_items[0]));
//int menu_ll=0; //menu lower text limit

// Other HMI components to be added, might be a good idea to move them to a separate file
void progress_bar(float value, float scale)// 10 divisions
{
	uint8_t temp = (value/scale)*10;
	lcd_cell(2,4);
	lcd_char('[');
	for (int i=0;i<temp;i++)
	{
		lcd_char(0xff);
	}
	for (int j=0; j<(10-temp);j++)
	{
		lcd_char(' ');
	}
	
	lcd_char(']');
	
	lcd_cell(1,1);
}

void radio_btn(uint8_t value)
{
	lcd_cell(2,4);
	lcd_print("Setting: [");
	
	switch (value)
	{
		
		case 0:
		lcd_print("OFF");
		break;
		
		case 1:
		lcd_print("ON");
		break;
		
	}
	
	lcd_char(']');
}

//interrupt routines

ISR(PCINT0_vect)	//For port B, used to navigate screen and set values
{
	
	if(ENTER)
	{
		btn_pressed=enter;
		btn_flag=1;
	}
	
	else if (UP)
	{
		btn_pressed=up;
		btn_flag=1;
	}
	
	else if (DOWN)
	{
		btn_pressed=down;
		btn_flag=1;
	}
	
	else if(DATA)	//Data ready
	data_flag=1;
	
	//reti();
}

void interrupt_init(){
		//Enable pin change interrupt on port B and set mask for upper 4 pins
		cli();
		PCICR  = 0b00000001;
		PCMSK0 = 0b11110000;
		
		//Set pins as input and activate pull-up resistors
		DDRB=0x00;
		PORTB=0b11110000;
		timer_delay_m(5); // needed for interrupt signals to settle
		sei();
		
}

/*Menu structure definitions*/

int main(void)
{    
	DDRC = 0xff;							/*set first four bits as output*/
	PORTC = 0x00;
	
	interrupt_init();
	start_twi(0x27);	//Start I2C with selected device address(LCD)
	timer_delay_m(1);
	lcd_init();
	spi_master_init();	
	
	/*To reflect an interrupt on the IRQ pin, set its value to 0 in this function.
	CRC byte setting: 0: 1byte, 1:2bytes*/
	nrf_config(0,1,1,1,0);
	
	/*These settings are defined at the beginning of this file*/
	nrf_setup(ADDR_LEN,CHANNEL,DATA_RATE,POWER);
	
	nrf_up();	/*power up the chip in standby I mode*/
	nrf_start_rx(controller_addr,RX_PIPE,PAYLOAD);	/*Transmission setup*/
	//
// 	nrf_start_tx(motor_addr,motor_addr,RX_PIPE);	/*Switch to transmit mode to signal the motor*/
// 	
// 	wp_buffer[0]=1;
// 	nrf_wr_payload(wp_buffer,1);
// 	nrf_transmit();
// 	timer_delay_m(1000);
// 	nrf_start_rx(controller_addr,RX_PIPE,PAYLOAD);		/*Switch back to receive mode*/
// 	timer_delay_m(10);
// 	//
	timer_delay_m(20);
	nrf_flush_rx(rp_buffer);
	
	main_scr();
    while (1) 
    {
		
	timer_delay_m(10); //Needed so the ISR can have a chance XD
	
		if (btn_flag)
		{
			switch(btn_pressed)
			{
							
				case enter:
				btn_enter();
				break;
							
				case up:
				btn_up();
				break;
							
				case down:
				btn_down();
				break;
			}
			
			btn_flag = 0;
		}
	
		
        if (data_flag)
        {
			reading=read_data();
			if (depth==0)
			main_scr();
			lcd_cell(3,5);
			//int16print(reading);
			control(reading);
			
			//
			//if (readings[1]!=0 && readings[2]!=0);
			
			data_flag=0;
        }
        //sleep mode until signal update
  
    }
	
	stop_twi();
	
}

//these functions to be triggered by IRQ

void btn_up()
{
	switch (depth)
	{

		case 1:
		if ( cursor>0)
		cursor--;
		else if(cursor==0 && menu_ll>0)
		menu_ll--;
		break;

		case 2:
		if ( cursor>0)
		cursor--;
		else if(cursor==0 && menu_ll>0)
		menu_ll--;
		break;

		case 3:
		
		//Adjustment functionality
		if(node_ptr->type==progress && *(uint8_t*)node_ptr->set_val<settings[range])
		{
			 (*(uint8_t*)node_ptr->set_val)+=10;
		}
		else if(node_ptr->type==text && *(uint8_t*)node_ptr->set_val<0x10) //Max hex number input
		{
			(*(uint8_t*)node_ptr->set_val)++;
		}
		else if (node_ptr->type==2)
		(*(uint8_t*)node_ptr->set_val)=1;
		break;

	}
		update_screen(depth);
}

void btn_down()
{
	switch (depth)
	{

		case 1:
		if ( cursor<3)
		cursor++;
		//This portion have to be optimized as sizeof is calculated many times
		else if(cursor==3 && menu_ll<menu_size-4)
		menu_ll++;
		break;

		case 2:
		if ( cursor<3)
		cursor++;

		else if(cursor==3 && menu_ll<(sizeof(menu_ptr->child)/sizeof(menu_ptr->child[0])-5))
		menu_ll++;
		break;

		case 3:
		//Adjustment functionality
		if(node_ptr->type==progress && *(uint8_t*)node_ptr->set_val>0)
		(*(uint8_t*)node_ptr->set_val)-=10;
		
		else if(node_ptr->type==text && *(uint8_t*)node_ptr->set_val>0) //Max hex number input
		(*(uint8_t*)node_ptr->set_val)--;
		
		else if (node_ptr->type==radio)
		(*(uint8_t*)node_ptr->set_val)=0;

		break;

	}
	update_screen(depth);
}

void btn_enter()
{
	switch (depth)
	{
		case 0:
		depth++;
		cursor=0;
		break;

		case 1:
		if (cursor == 0 && menu_ll==0) // this is the exit value
		{
			depth--;
		}
		else
		{						
			menu_ptr=main_menu.child[cursor+menu_ll];
			depth++;
		}
		
		cursor=menu_ll=0;
		break;

		case 2:

		if (cursor == 0 && menu_ll==0)
		{
			depth --;
			
		}
		else
		{		
			node_ptr=menu_ptr->child[cursor+menu_ll];
			if (node_ptr->type==remote)
			{
			reading =read_data();	
			}
			depth++;
		}
		
		cursor=menu_ll=0;
		break;

		case 3:
		if (node_ptr->type==remote)
		{
			reading =read_data();
			*(uint16_t*)node_ptr->set_val=readings[0];
		}
		//Set value have to be adjusted with correct functionality
		//*(node_ptr->set_val)=0;
		depth--;
		break;

	}
	
	update_screen(depth);
} 

   //Screen view functions
   //Show home screen and status

   void main_scr()
   {
	   lcd_clear();
	   timer_delay_m(2);
	   lcd_cell(1,5);
	   lcd_print("TANK LEVEL");
	   progress_bar(reading,settings[range]);
	   lcd_cell(4,7);
	   lcd_print("MENU");
   }

   //Show options
   void opt_scr()
    {		
	  		// Used inside the loops to set print limits
	  		lcd_clear();
	  		timer_delay_m(5);
	  		
	  		for (int i=0; i <=3; i++)
	  		{
		  		lcd_cell(i+1,1);
		  		lcd_print(main_menu.child[menu_ll+i]->caption);
		  		timer_delay_u(200);
	  		}
	  		lcd_cell(cursor+1,0);
	  		lcd_char('>');
   }

   //Show option settings
   void setting_scr()
   {
	   	// Used inside the loops to set print limits
	   	lcd_clear();
	   	timer_delay_m(2);
	   	
	   	for (int i=0; i <= 3 ; i++)
	   	{
		  if (menu_ptr->child[menu_ll+i]!=0)
	   	  {
	   	   	lcd_cell(i+1,1);
		   	lcd_print(menu_ptr->child[menu_ll+i]->caption);
		   	
	      }
	   }
	   
	   	lcd_cell(cursor+1,0);
	   	lcd_char('>');
   }

   //show edit screen. needs to be updated to cover other HMI elements
   void edit_scr(struct node *node_ptr)
   {
	   lcd_clear();
	   timer_delay_m(2);
	   lcd_cell(1,0);
	   lcd_print(node_ptr->caption);
	   
	   switch (node_ptr->type)
	   { 
		   
		   case remote:
		   lcd_cell(2,0);
		   lcd_print("Sensor reading: ");
		   lcd_cell(3,0);
		   int16print(readings[0]);
		   //byte2char(readings[0]);
		   break;
		   
		   case progress:
		   progress_bar(*(uint8_t*)node_ptr->set_val,settings[range]);
	   	   break;
		   
		   case radio:
		   radio_btn(*(uint8_t*)node_ptr->set_val);
		   break;
		   
		   case text:
		   break;
	   	   
	   }
	   
	   lcd_cell(4,9);
	   lcd_print("OK");
   }
   
   void update_screen(uint8_t screen)
   {
	   switch  (screen)
	   {
		   case 0:
		   main_scr();
		   break;
		   
		   case 1:
		   opt_scr();
		   break;
		   
		   case 2:
		   setting_scr();
		   break;
		   
		   case 3:
		   edit_scr(node_ptr);
		   break;
		   
	   }
	   
   }
   
   
   //Data acquisition and control
   uint8_t read_data()
   {
	   //Data read is upside down because low level is farther than high level compared to the sensor
	   if (data_flag)
	   {
		   nrf_receive(rp_buffer,PAYLOAD);
		   holder.data[0]=rp_buffer[0];
		   holder.data[1]=rp_buffer[1];											/*Read data from the chip*/
		   readings[0] = holder.reading;
		   if (readings[1]!=0 && readings[2]!=0)										/*Transfer it to the variable matrix*/
		   return (1-((float)readings[0]-(float)readings[1])/((float)readings[2]-(float)readings[1]))*100;	/* then scale the reading to 100% */
	   }
// 	   else
// 	   {
// 		   //readings[0]=0;
// 		   return 0;
// 	   }
	   
   
   }

  void control(uint8_t current)
  {
	
	uint8_t temp=motor_control;
	if (current < 40/*settings[motor_on_level]*/)		/*Check for high level in the tank*/
	motor_control=1;
	
	else if (current >= 70/*settings[motor_off_level]*/)		/*Low level check*/
	motor_control=0;
	
	//
	
	//
	
	//if (temp != motor_control)
	//{
	
		//PORTC &= ~(1<<CE_PIN);	
		nrf_start_tx(motor_addr,motor_addr,RX_PIPE);	/*Switch to transmit mode to signal the motor*/
		wp_buffer[0]=motor_control;
		nrf_wr_payload(wp_buffer,1);
		nrf_transmit();
		timer_delay_m(100);
		nrf_start_rx(controller_addr,RX_PIPE,PAYLOAD);		/*Switch back to receive mode*/
	
	
	//}
	
	  
  } 
  
void int16print(uint16_t number)
{
	uint16_t temp = number;
	char digits[6];
	
	digits[6]='\0';
	
	for (int i = 5; i>=0; i--)
	{
		digits[i] = (temp % 10)+'0';
		temp/=10;
		
	}
	
	lcd_print(digits);
}  
void byte2char(uint16_t bite)
{
	for (int i=0;i<=15;i++)
	{
		if((bite<<i)&0x8000)
		{
			lcd_char('1');
			
		}
		else
		{
			lcd_char('0');
			
		}
		
	}
}

