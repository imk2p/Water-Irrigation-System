/*
 * main.c
 *
 *  Created on: 10-Sep-2017
 *      Author: OWNER
 */
/*
    SIM900 GSM Interfacing with ATmega16/32
    http://www.electronicwings.com
 */
#define F_CPU 8000000UL		/* define Clock Frequency */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lcd.h"	/* include 16x2 LCD Header file */
#include "USART_Interrupt.h"	/* include USART Header file */
#include <util/delay.h>

#define SREG   _SFR_IO8(0x3F)

void GSM_Begin();
void GSM_Calling(char *);
void GSM_HangCall();
void GSM_Response();
void GSM_Response_Display();
void GSM_Msg_Read(int);
bool GSM_Wait_for_Msg();
void GSM_Msg_Display();
void GSM_Msg_Delete(unsigned int);
void GSM_Send_Msg(char* , char*);
void GSM_Delete_All_Msg();

char buff[160];			/* buffer to store responses and messages */
char status_flag = 0;		/* for checking any new message */
volatile int buffer_pointer;
char Mobile_no[14];		/* store mobile no. of received message */
char message_received[60];	/* save received message */
int position = 0;		/* save location of current message */

#define LED		SBIT(PORTC,2)
#define LED_DIR	SBIT(DDRC,2)

#define PUMP		SBIT(PORTB,0)
#define PUMP_DIR	SBIT(DDRB,0)

char msg_string[100];
//char msg_str[120];

void initADC()
{
	ADMUX=(1<<REFS0);//|(1<<REFS1);
	ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}
int readADC(int ch)
{
	ADMUX&=0xf0;
	ch&=0x07;
	ADMUX|=ch;
	ADCSRA|=(1<<ADSC);
	while(!(ADCSRA&(1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	return(ADC);
}



int main(void)
{
	buffer_pointer = 0;
	int is_msg_arrived;
	memset(message_received, 0, 60);
	lcd_init();		//initialize the lcd module
	USART_Init(9600);	/* initialize USART */
	sei();		//Master interrupt enable
	lcd_gotoxy(1,0);
	lcd_printf("GSM Initializing");
	initADC(); 		//initialize the ADC module
	LED_DIR = 1;
	PUMP_DIR = 1;
	LED = 1;
	PUMP = 0;
	_delay_ms(3000);		//Wait for GSM network
	_delay_ms(3000);
	_delay_ms(3000);
	lcd_clear();		//clear the lcd module
	lcd_gotoxy(1,0);
	lcd_printf("AT");
	GSM_Begin();		/* Start GSM with AT*/
	lcd_clear();
	int rain,soil,ldr;
	int pump_state = 0,rain_state = 0, soil_state = 0,light_state=0;
	static int is_pump_on=0;
	char arr[4];
	char rain_str[20];
	char soil_str[20];
	char pump_str[20];
	char light_str[20];
	while (1){
		rain = readADC(3);
		soil = readADC(1);
		ldr = readADC(0);
		lcd_gotoxy(1,0);
		if(rain > 200)
		{
			lcd_printf("RAIN=Y");
			rain_state = 1;

		}
		else if(rain < 100)
		{
			lcd_printf("RAIN=N");
			rain_state = 0;

		}
		lcd_gotoxy(1,8);
		if(soil < 400)
		{
			lcd_printf("SOIL=Y");
			soil_state = 1;

		}
		else if(soil > 700)
		{
			lcd_printf("SOIL=N");
			soil_state = 0;

		}

		lcd_gotoxy(2,0);
		if(ldr < 200)
		{
			lcd_printf("LED=Y");
			LED = 0;
			light_state = 1;

		}
		else if(ldr > 220)
		{
			lcd_printf("LED=N");
			LED = 1;
			light_state = 0;

		}

		lcd_gotoxy(2,7);
		if((!rain_state)&&(!soil_state))
		{
			PUMP = 1;
			pump_state = 1;
			lcd_printf("PUMP=ON ");
			if(is_pump_on==0)
			{
				_delay_ms(1000);
				lcd_init();
				lcd_printf("sending msg");
				GSM_Send_Msg("+917300472780","PUMP IS ON");
				_delay_ms(3000);
				GSM_Send_Msg("+919680859206","PUMP IS ON");
				lcd_clear();
				is_pump_on = 1;
			}
		}
		else
		{
			PUMP = 0;
			pump_state = 0;
			lcd_printf("PUMP=OFF");

			if(is_pump_on)
			{
				_delay_ms(1000);
				lcd_init();
				lcd_printf("sending msg");
				GSM_Send_Msg("+917300472780","PUMP IS OFF");
				_delay_ms(3000);
				GSM_Send_Msg("+919680859206","PUMP IS OFF");
				lcd_clear();
				is_pump_on = 0;
			}

		}



		/*check if any new message received */
		if(status_flag==1){
			is_msg_arrived = GSM_Wait_for_Msg(); //check for message arrival
			if(is_msg_arrived== true)
			{

				if(rain_state)
					memcpy(&rain_str[0],"RAIN PRESENT. ",sizeof("RAIN PRESENT. "));
				else
					memcpy(&rain_str[0],"RAIN NOT PRESENT. ",sizeof("RAIN NOT PRESENT. "));
				if(soil_state)
					memcpy(&soil_str[0],"SOIL PRESENT. ",sizeof("SOIL PRESENT. "));
				else
					memcpy(&soil_str[0],"SOIL NOT PRESENT. ",sizeof("SOIL NOT PRESENT. "));
				if(light_state)
					memcpy(&light_str[0],"LIGHT IS ON. ",sizeof("LIGHT IS ON. "));
				else
					memcpy(&light_str[0],"LIGHT IS OFF. ",sizeof("LIGHT IS OFF. "));
				if(pump_state)
					memcpy(&pump_str[0],"PUMP IS ON. ",sizeof("PUMP IS ON. "));
				else
					memcpy(&pump_str[0],"PUMP IS OFF. ",sizeof("PUMP IS OFF. "));
				sprintf(msg_string,"%s %s %s %s",rain_str,soil_str,light_str,pump_str);
				/*for(int i=0;msg_string[i]!='\0';i++)
				{
					lcd_gotoxy(1,1);
					lcd_putc(msg_string[i]);
					_delay_ms(800);
				}*/
				/*// lcd_clear();
			   lcd_gotoxy(1,0);
			   lcd_printf("new message");  //new message arrived
			   _*/_delay_ms(1000);
			   //lcd_clear();
			   GSM_Msg_Read(position);  //read arrived message
			   _delay_ms(3000);

			//check if received message is "call me"
				if(strstr( message_received,"Give data")){


					//msg_string = concat(&rain_str[0],&msg_string[0]);
					//do things with s

					//sprintf(msg_str,"%s%s",&msg_string[0],&rain_str[0]);
					GSM_Send_Msg("+917300472780",msg_string);
					_delay_ms(3000);
					GSM_Send_Msg("+919680859206",msg_string);

					_delay_ms(2000);
				}

			//lcd_clear();
			GSM_Msg_Delete(position);  //to save SIM memory delete current message
			/*lcd_gotoxy(1,0);
			lcd_printf("Clear msg");*/
			GSM_Response();
			_delay_ms(500);
			lcd_init();

			}


			is_msg_arrived=0;
			status_flag=0;
			//lcd_clear();
			}
		/*lcd_gotoxy(1,0);
		lcd_printf("waiting for msg");*/
		memset(Mobile_no, 0, 14);
		memset(message_received, 0, 60);
		memset(msg_string,0,100);
		memset(rain_str,0,20);
		memset(soil_str,0,20);
		memset(pump_str,0,20);
		memset(light_str,0,20);



	}
}



void GSM_Begin()
{
	int cnt = 0;
	while(1)
	{
		cnt++;
		lcd_command(0xc0);
		USART_SendString("ATE0\r"); /* send ATE0 to check module is ready or not */
		_delay_ms(500);
		if(strstr(buff,"OK"))
		{
			GSM_Response();
			memset(buff,0,160);
			break;
		}
		else
		{
			lcd_printf("Error");
		}
		if(cnt>=4)
		{
			break;
		}
	}
	_delay_ms(1000);

	lcd_clear();
	lcd_gotoxy(1,0);
	lcd_printf("Text Mode");
	lcd_command(0xc0);
	USART_SendString("AT+CMGF=1\r"); /* select message format as text */
	GSM_Response();
	_delay_ms(1000);
}

void GSM_Msg_Delete(unsigned int position)
{
	buffer_pointer=0;
	char delete_cmd[20];

      /* delete message at specified position */
	//sprintf(delete_cmd,"AT+CMGDA=\"DEL ALL\"\r",position);
	USART_SendString("AT+CMGDA=\"DEL ALL\"\r");
}

void GSM_Delete_All_Msg()
{
   USART_SendString("AT+CMGDA=\"DEL ALL\"\r"); /* delete all messages of SIM */
}

bool GSM_Wait_for_Msg()
{
	char msg_location[4];
	int i;
	_delay_ms(500);
	buffer_pointer=0;

	while(1)
	{
           /*eliminate "\r \n" which is start of string */

	   if(buff[buffer_pointer]=='\r' || buff[buffer_pointer]== '\n'){
			buffer_pointer++;
		}
		else
			break;
	}

	/* "CMTI:" to check if any new message received */

	if(strstr(buff,"CMTI:")){
		while(buff[buffer_pointer]!= ',')
		{
			buffer_pointer++;
		}
		buffer_pointer++;

		i=0;
		while(buff[buffer_pointer]!= '\r')
		{
			msg_location[i]=buff[buffer_pointer];				      /* copy location of received message where it is stored */
			buffer_pointer++;
			i++;
		}

		/* convert string of position to integer value */
		position = atoi(msg_location);

		memset(buff,0,strlen(buff));
		buffer_pointer=0;

		return true;
	}
	else
	{
		return false;
	}
}

/* ISR routine to save responses/new message */
ISR(USART_RXC_vect)
{
   buff[buffer_pointer] = UDR;	/* copy UDR (received value) to buffer */
   buffer_pointer++;
   status_flag = 1;		/* flag for new message arrival */
}


void GSM_Send_Msg(char *num,char *sms)
{
	char sms_buffer[35];
	buffer_pointer=0;
	sprintf(sms_buffer,"AT+CMGS=\"%s\"\r",num);
	USART_SendString(sms_buffer); /*send command AT+CMGS="Mobile No."\r */
	_delay_ms(200);
	while(1)
	{
		if(buff[buffer_pointer]==0x3e) /* wait for '>' character*/
		{
		   buffer_pointer = 0;
		   memset(buff,0,strlen(buff));
		   USART_SendString(sms); /* send msg to given no. */
		   USART_TxChar(0x1a); /* send Ctrl+Z */
		   break;
		}
		buffer_pointer++;
	}
	_delay_ms(300);
	buffer_pointer = 0;
	memset(buff,0,strlen(buff));
	memset(sms_buffer,0,strlen(sms_buffer));
}

void GSM_Calling(char *Mob_no)
{
   char call[20];
   sprintf(call,"ATD%s;\r",Mob_no);
   USART_SendString(call);	/* send command ATD<Mobile_No>; for calling*/
}

void GSM_HangCall()
{
   lcd_clear();
   USART_SendString("ATH\r");	/*send command ATH\r to hang call*/
}

void GSM_Response()
{
	unsigned int timeout=0;
	int CRLF_Found=0;
	char CRLF_buff[2];
	int Response_Length=0;
	while(1)
	{
		if(timeout>=6000)								/*if timeout occur then return */
		return;
		Response_Length = strlen(buff);
		if(Response_Length)
		{
			_delay_ms(2);
			timeout++;
			if(Response_Length==strlen(buff))
			{
				for(int i=0;i<Response_Length;i++)
				{
					memmove(CRLF_buff,CRLF_buff+1,1);
					CRLF_buff[1]=buff[i];
					if(strncmp(CRLF_buff,"\r\n",2))
					{
					    if(CRLF_Found++==2)				                                    /* search for \r\n in string */
					     {
						GSM_Response_Display();
						return;
					     }
					}

				}
				CRLF_Found = 0;

			}

		}
		_delay_ms(1);
		timeout++;
	}
	status_flag=0;
}

void GSM_Response_Display()
{
	buffer_pointer = 0;
	int lcd_pointer = 0;
	while(1)
	{
          /* search for \r\n in string */
	  if(buff[buffer_pointer]== '\r' || buff[buffer_pointer]== '\n')
		{
			buffer_pointer++;
		}
		else
			break;
	}


	lcd_command(0xc0);
	while(buff[buffer_pointer]!='\r')								   /* display response till "\r" */
	{
		lcd_putc(buff[buffer_pointer]);
		buffer_pointer++;
		lcd_pointer++;
		if(lcd_pointer==15)	/* check for end of LCD line */
		  lcd_command(0x80);
	}
	buffer_pointer = 0;
	memset(buff,0,strlen(buff));
}

void GSM_Msg_Read(int position)
{
	char read_cmd[10];
	sprintf(read_cmd,"AT+CMGR=%d\r",position);
	USART_SendString(read_cmd);	/* read message at specified location/position */
	GSM_Msg_Display();	/* display message */
}

void GSM_Msg_Display()
{
	_delay_ms(500);
	if(!(strstr(buff,"+CMGR")))	/*check for +CMGR response */
	{
		//lcd_gotoxy(1,0);
		//lcd_printf("No message");
	}
	else
	{
		buffer_pointer = 0;

		while(1)
		{
                        /*wait till \r\n not over*/

			if(buff[buffer_pointer]=='\r' || buff[buffer_pointer]== '\n') 			                {
				buffer_pointer++;
			}
			else
			        break;
		}

		/* search for 1st ',' to get mobile no.*/
		while(buff[buffer_pointer]!=',')
		{
			buffer_pointer++;
		}
		buffer_pointer = buffer_pointer+2;

		/* extract mobile no. of message sender */
		for(int i=0;i<=12;i++)
		{
			Mobile_no[i] = buff[buffer_pointer];
			buffer_pointer++;
		}

		do
		{
			buffer_pointer++;
		}while(buff[buffer_pointer-1]!= '\n');

		//lcd_command(0xC0);
		int i=0;

		/* display and save message */
		while(buff[buffer_pointer]!= '\r' && i<31)
		{
				//lcd_putc(buff[buffer_pointer]);
				message_received[i]=buff[buffer_pointer];

				buffer_pointer++;
				i++;
				if(i==16)
				{}//lcd_command(0x80); /* display on 1st line */
		}

		buffer_pointer = 0;
		memset(buff,0,strlen(buff));
	}
	status_flag = 0;
}

