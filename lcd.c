/****************************************************************************
**									   **
**		    	Hitachi Character-Type LCD Display		   **
**									   **
****************************************************************************/
//
//		NOTE: 	This is set up for 4 MHz oscillator
//

#include <avr/io.h>
#include<util/delay.h>
#include "lcd.h"
#include "always.h"
#include "delay.h"

//#define	LCD_C

const byte LCD_ROW_ADDRESS[] =					// Row/Column information for lcd_gotoxy()
	{
#if LCD_MODE_1x8
	0x00
#endif
#if LCD_MODE_1x16_A
	0x00
#endif
#if LCD_MODE_1x16_B
	0x00,
	0x40
#endif
#if LCD_MODE_1x40
	0x00,
#endif
#if LCD_MODE_2x8
	0x00,
	0x40
#endif
#if LCD_MODE_2x12
	0x00,
	0x40
#endif
#if LCD_MODE_2x16
	0x00,
	0x40
#endif
#if LCD_MODE_2x20
	0x00,
	0x40
#endif
#if LCD_MODE_2x24
	0x00,
	0x40
#endif
#if LCD_MODE_2x40
	0x00,
	0x40
#endif
#if LCD_MODE_4x16
	0x00,
	0x40,
	0x10,
	0x50
#endif
#if LCD_MODE_4x20
	0x00,
	0x40,
	0x14,
	0x54
#endif
#if LCD_MODE_4x24
	0x00,
	0x40,
	0x80,
	0xC0
#endif
	};

const byte LCD_INIT_STRING [] =					// LCD Init String on powerup
	{
     0x01, //0b00000001,							//	Clear display
	 0x02,//0b00000010,							//	Home cursor
    0x04 //0b00000100							//	Entry Mode
#if LCD_CURSOR_INCREMENT
	| 0x02 //0b00000010							//		Increment cursor
#endif
#if LCD_CURSOR_SHIFT
	| 0x01//0b00000001							//		Shift on cursor
#endif
	,						//		end
	0x08 //0b00001000							//	Display Control
#if LCD_DISPLAY_ON
	| 0x04 //0b00000100							//		Display on
#endif
#if LCD_CURSOR_ON
	| 0x02 //0b00000010							//		Cursor on
#endif
#if LCD_CURSOR_BLINK
	| 0x01//0b00000001							//		Blink on
#endif
		,							            //		end
	0x20 //0b00100000							//	Function Set
#if LCD_8_BIT_MODE
	| 0x10 //0b00010000							//		8-bit data bus
#endif
#if LCD_MULTI_LINE
	|0x08//0b00001000							//		2-line refreshing
#endif
#if LCD_DISPLAY_5x10
	| 0x04//0b00000100							//		5x10 matrix
#endif
							//	Clear display

	}
   ;
#if LCD_4_BIT_MODE
void lcd_putnybble (byte c)						// Write nybble to port in current RS mode
{
	c = c << LCD_D4_BIT;						// Shift over to correct bit column
	c &= LCD_TRIS_DATAMASK;						// Remove any extraneous bits

	LCD_DATA_PORT = (LCD_DATA_PORT & ~LCD_TRIS_DATAMASK) | c;	// Write data bits to port
	DelayUs(CONST_US_DELAY);
	LCD_E = 1;
	DelayUs(CONST_US_DELAY);
	LCD_E = 0;
	/*
	   DelayUs(CONST_US_DELAY);
	   LCD_E = 1;							// Start to write it
	   DelayUs(CONST_US_DELAY*2);
	   LCD_E = 0;	
	 */						// Finish write cycle
}
#endif

byte lcd_getbyte (void)							// Read byte at cursor (RS=1) or ready status (RS=0)
{
	byte	retval;
#if LCD_4_BIT_MODE
	byte	highbits;

#if OUTPUT == 1
	LCD_TRIS_PORT |= LCD_TRIS_DATAMASK;				// Set port to read mode for data pins
#else
	LCD_TRIS_PORT |= ~LCD_TRIS_DATAMASK;				// Set port to read mode for data pins
#endif 

	//	LCD_RW = 1;							// Tell LCD we want to read
	DelayUs (CONST_US_DELAY);
	LCD_E = 1;
	highbits = (((LCD_DATA_PORT & LCD_TRIS_DATAMASK) >> LCD_D4_BIT) << 4);// Grab high bits and shift to right place
	LCD_E = 0;
	DelayUs (CONST_US_DELAY);
	LCD_E = 1;
	DelayUs (CONST_US_DELAY);
	retval = ((LCD_DATA_PORT & LCD_TRIS_DATAMASK) >> LCD_D4_BIT);	// Grab low bits
	LCD_E = 0;
	retval |= highbits;
#if OUTPUT == 1
	LCD_TRIS_PORT &= ~LCD_TRIS_DATAMASK;				// Set port back to output mode
#else
	LCD_TRIS_PORT &= LCD_TRIS_DATAMASK;				// Set port back to output mode
#endif

#else
	LCD_TRIS_PORT = 0xFF;						// Set port to all input
	//	LCD_RW = 1;							// Tell LCD we want to read
	DelayUs (CONST_US_DELAY);
	LCD_E = 1;							// Do the read
	DelayUs (CONST_US_DELAY);
	retval = LCD_DATA_PORT;
	LCD_E = 0;
	LCD_TRIS_PORT = 0x00;						// Set port back to outputs
#endif

	//return (retval);						// Give answer to caller
	return (0);			 //WR pin always GND

}

void lcd_putbyte (byte c)						// Write byte to port in current RS mode
{
	byte	RS_Status;

	/*
	   RS_Status = LCD_RS;						// Get old pin state
	   LCD_RS = 0;							// Force into command mode to read state
	   while (lcd_getbyte () & 0x80);					// Wait for read state
	   if (RS_Status)
	   LCD_RS = 1;						// Restore RS to old state
	 */	
	//	LCD_RW = 0;							// Set to write mode
	DelayUs (CONST_US_DELAY);
#if LCD_4_BIT_MODE
	DelayUs (CONST_US_DELAY);
	lcd_putnybble (c >> 4);						// Send the character out
	DelayUs (CONST_US_DELAY);
	lcd_putnybble (c);
	DelayUs (CONST_US_DELAY);
	LCD_E = 0;
#else
	LCD_DATA_PORT = c;						// Send the character out
	LCD_E = 1;
	DelayUs (CONST_US_DELAY);
	LCD_E = 0;
#endif
}

void lcd_command (byte c)						// Send command to LCD port
{
	LCD_RS = 0;
	lcd_putbyte (c);
	_delay_ms(3);
}

#if LCD_ALLOW_USER_CHARS
void lcd_define_char (byte c, const byte *bitmap)			// Define user-defined chars
{
	byte	i;

	lcd_command ((0x40 )/*0b01000000)*/ | (c << 3));					// Select char to define

	LCD_RS = 1;
	for (i = 0; i < 8; i++)
		lcd_putbyte (*bitmap++);				//	Put in each byte of memory
}
#endif

byte lcd_lineof (byte CursorAddress)					// Calculate cursor row from it's address
{
	CursorAddress &= 0x50;						//	Strips out uniquely the address bits
	switch (CursorAddress)
	{
		case 0x00:						// Note - this handles all cases except for some
			CursorAddress = 1;				//	of those unsupported displays listed in
		case 0x40:						//	lcd.h file.
#if LCD_MODE_1x16_B
			CursorAddress = 1;				//	Only 1 row this type of display
#else
			CursorAddress = 2;
#endif
		case 0x10:
			CursorAddress = 3;
		case 0x50:
			CursorAddress = 4;
		default:
			CursorAddress = 1;
	}
	return (CursorAddress);
}

byte lcd_cursorpos (void)						// Return address of cursor position
	{
	LCD_RS = 0;
	return (lcd_getbyte ());					//	Get cursor position
	}

void lcd_putc (char c)							// Write character to LCD
{
#if !LCD_ALLOW_USER_CHARS
		byte	CursAddr;
#endif
#if LCD_ALLOW_USER_CHARS						// Allow user-defined characters - no terminal mode
		LCD_RS = 1;
		lcd_putbyte (c);
#else
		switch (c)
		{
				case (0x08):						  //	Backspace?
						lcd_command (LCD_COMMAND_BACKSPACE);		//		back cursor up
#if LCD_DESTRUCTIVE_BS
						LCD_RS = 1;					     //		set display mode
						lcd_putbyte (' ');				//		erase previous character
						lcd_command (LCD_COMMAND_BACKSPACE);		//		move cursor back again
#endif
						break;
				case (0x0d):						      //	Newline?
						LCD_RS = 0;
						CursAddr = lcd_getbyte ();			//		Get cursor position
						CursAddr = lcd_lineof (CursAddr);
#if LCD_ENABLE_SCROLL
						if (CursAddr >= LCD_MAXROWS)			//		Bottom line?
								lcd_scroll ();				//			Yes, force scroll
						else						//			No, just go to start of next line
								lcd_gotoxy (CursAddr+1,1);
#else
						lcd_gotoxy (CursAddr+1, 1);			//		Position cursor to start of line
#endif
						break;
				case (0x1a)://AAK: control-z						//	Form Feed (clear screen)?
						lcd_command (LCD_COMMAND_CLEAR);		//		Erase screen
						lcd_gotoxy (1,1);				//		Position cursor to top of screen
						break;
				default:						//	Printable?
						LCD_RS = 1;					//		Set to display mode
						lcd_putbyte (c);				//		Send character out
		}
#endif
}

#if LCD_ENABLE_GETC
byte lcd_getc (void)							// Read character at cursor
	{
	byte	retval;

	LCD_RS = 1;
	retval = lcd_getbyte ();
	LCD_RS = 0;
	return (retval);
	}
#endif

#if LCD_ENABLE_GOTOXY
void lcd_gotoxy (byte row, byte col)					// Position cursor
	{
#if LCD_MODE_1x16_B
	if (col > 7)							// 1x16 is treated the same as 2x8 for addressing
		{
		row++;
		col -= 8;
		}
	if (col > 8)
		col = 8;
	if (row > 2)
		row = 2;
#else
	if (row > LCD_MAXROWS)						// Range limit
		row = LCD_MAXROWS;
	if (col > LCD_MAXCOLS)
		col = LCD_MAXCOLS;
#endif

	row = LCD_ROW_ADDRESS[row-1];					// Get address of first byte on desired row
	row += col - 1;

	lcd_command (0x80 | row);					// Write new cursor address
	}

void lcd_getxy (byte *row, byte *col)					// Return row and column of cursor position
	{
	byte	rr,
		cc;

	cc = lcd_cursorpos ();
	rr = lcd_lineof (cc);						//	Get row of the cursor
	cc = (cc & 0x7f) - LCD_ROW_ADDRESS[rr-1];			//	Find the column
	*row = rr;							       //	Convert to lcd_gotoxy() units
	*col = cc;
	}

#endif

#if LCD_ENABLE_PRINTF
void lcd_printf (const char* message)					// Write message to LCD (C string type)
	{
	while (*message)						//	Look for end of string
		lcd_putc (*message++);					//	Show and bump
	}
#endif

#if LCD_ENABLE_SCROLL
void lcd_scroll (void)							// Scroll up one line
	{
	byte	CursorPos,						// Hold position of cursor
		Character,						// Hold character being moved
		SrcAddr,						// Source Address
		DestAddr,						// Destination Address
		EndAddr;						// Ending copy address (last address of Source)

	LCD_RS = 0;
	CursorPos = lcd_getbyte () | 0x80;				// Get cursor position

	lcd_gotoxy (2,1);
	LCD_RS = 0;
	SrcAddr = lcd_getbyte () | 0x80;				// Find address of copy start
	lcd_gotoxy (1,1);
	LCD_RS = 0;
	DestAddr = lcd_getbyte () | 0x80;				// Find address of copy destination
	lcd_gotoxy (LCD_MAXROWS, LCD_MAXCOLS);
	LCD_RS = 0;
	EndAddr = lcd_getbyte () | 0x80;				// Find address of last byte to copy over

	do
		{
		LCD_RS = 0;						//	Position to source of copy char
		lcd_putbyte (SrcAddr);
		LCD_RS = 1;
		Character = lcd_getbyte ();				//	Read the character there
		LCD_RS = 0;
		lcd_putbyte (DestAddr);					//	Move to the destination
		LCD_RS = 1;
		lcd_putbyte (Character);				//	Write it the char there
		SrcAddr++;
		DestAddr++;
		}
	while (SrcAddr <= EndAddr);					// Loop through all memory

	for (Character = 1; Character <= LCD_MAXCOLS; Character++)
		{
		lcd_gotoxy (LCD_MAXROWS, Character);			//	Position on last line
		lcd_putc (' ');						//	Blank out the char
		}
	lcd_gotoxy (lcd_lineof (CursorPos) + 1,1);			// Home cursor next line
	}
#endif

#if LCD_ENABLE_UNSCROLL
void lcd_unscroll (void)				// Roll scroll backwards one line
	{
	byte	CursorPos,					// Hold position of cursor
		Character,						// Hold character being moved
		SrcAddr,						// Source Address
		DestAddr;						// Destination Address

	LCD_RS = 0;
	CursorPos = lcd_getbyte () | 0x80;				// Get cursor position

	lcd_gotoxy (LCD_MAXROWS-1,LCD_MAXCOLS);
	LCD_RS = 0;
	SrcAddr = lcd_getbyte () | 0x80;				// Find address of copy start

	lcd_gotoxy (LCD_MAXROWS,LCD_MAXCOLS);
	LCD_RS = 0;
	DestAddr = lcd_getbyte () | 0x80;				// Find address of copy destination

	do
		{
		LCD_RS = 0;						//	Position to source of copy char
		lcd_putbyte (SrcAddr);
		LCD_RS = 1;
		Character = lcd_getbyte ();				//	Read the character there
		LCD_RS = 0;
		lcd_putbyte (DestAddr);					//	Move to the destination
		LCD_RS = 1;
		lcd_putbyte (Character);				//	Write it the char there
		SrcAddr--;
		DestAddr--;
		}
	while (SrcAddr != 0x80);					// Loop through all memory

	for (Character = 0; Character < LCD_MAXCOLS; Character++)
		{
		lcd_gotoxy (1, Character);				//	Position on top row
		lcd_putc (' ');						//	Blank out the char
		}
	lcd_gotoxy (lcd_lineof (CursorPos),1);				// Home cursor same line as before
	}
#endif

#if LCD_ENABLE_CLEAR
void lcd_clear (void)							// Clear LCD screen
	{
	lcd_command (LCD_COMMAND_CLEAR);
	}
#endif

void lcd_init (void)							// Reset display from software
{
	char	i;
	LCD_TRIS_E = OUTPUT;
	LCD_TRIS_RS = OUTPUT;

	LCD_E = 0;							// Set up control pin I/O
	//LCD_RW = 0;							// Write mode
		
	LCD_RS = 0;					            // Command mode
#if OUTPUT == 1	
	LCD_TRIS_PORT |= LCD_TRIS_DATAMASK;				// Set data bus to output
#else 
	LCD_TRIS_PORT |= ~LCD_TRIS_DATAMASK;				// Set data bus to output
#endif

	DelayMs (CONST_US_DELAY * 20);							// Wait a little while

#if LCD_4_BIT_MODE							// Set LCD into 4-bit mode
	lcd_putnybble (0x03);//(0b0011);						// Select 8-bit mode
	DelayMs (10);							// Spec calls for 4.1 mS
	lcd_putnybble (0x03);//(0b0011);						// Do it again
	DelayUs (130);
	lcd_putnybble (0x03);//(0b0011);
	DelayMs (10);
	lcd_putnybble (0x02);//(0b0010);						// Off and running...
#else
	lcd_putbyte (0x3F);//(0b00110000);					// Select 8-bit mode
	DelayMs (10);							// Spec calls for 4.1 mS
	lcd_putbyte (0x3F);// (0b00110000);					// Do it again
	DelayUs (130);
	lcd_putbyte (0x3F);//(0b00110000);
	DelayMs (10);
	lcd_putbyte (0x38);//(0b00110000);					// Off and running...
#endif
   

	for (i = 0; i < sizeof(LCD_INIT_STRING); i++)			// Send other LCD initialization stuff
	{
		lcd_command (LCD_INIT_STRING[i]);
		DelayMs(3);
	}
	 
	 
	  /*  
	DelayMs(10);
	lcd_putnybble(0x28);                   //	lcd_putbyte(0x18);
	DelayMs(10);
	lcd_putnybble(0x08);                  //0C);
	DelayMs(10);
	lcd_putnybble(0x01);
	DelayMs(10);
	lcd_putnybble(0x06);
	DelayMs(10);
	
	 */  
}
