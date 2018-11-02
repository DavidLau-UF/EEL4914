#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define FOSC 1000000
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
void USART_Init(unsigned int ubrr)
{
/*Set baud rate */
UBRR0H = (unsigned char)(ubrr>>8);
UBRR0L = (unsigned char)ubrr;
/* Enable receiver and transmitter */
UCSR0B = (1<<RXEN0);
/* Set frame format: 8data, 2stop bit */
UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}
unsigned char USART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) )
	;
	/* Get and return received data from buffer */
	return UDR0;
}
void lcd_init()
{
	DDRB |= 0xCF; //Change DDRA --> DDRB and PORTA --> PORTB if LCD attached to port B and so forth
	lcd_command(0x33); //Initialize LCD Driver
	lcd_command(0x32); //Four bit mode
	lcd_command(0x2C); //2 Lin e Mode
	lcd_command(0x0C); //Display On, Cursor Off, Blink Off Change to 0x0F if cursor is desired
	lcd_command(0x01); //Clear Screen, Cursor Home
}
void lcd_command(char cmd)
{
	char temp = cmd;
	PORTB=0;
	_delay_ms(5);
	cmd = ( (cmd & 0xF0) >> 4) | 0x80; //Write Upper Nibble (RS=0) E --> 1
	PORTB = cmd;
	_delay_ms(5);
	cmd ^= 0x80; //E --> 0
	PORTB = cmd;
	_delay_ms(5);
	cmd = temp;
	cmd = ( (cmd & 0x0F) ) | 0x80; //Write Lower Nibble (RS=0) E --> 1
	PORTB = cmd;
	_delay_ms(5);
	cmd ^= 0x80; //E --> 0
	PORTB = cmd;
	_delay_ms(7);
}
void lcd_char(char data)
{
	char temp = (data);
	PORTB = 0x40;
	_delay_us(100);
	data = ( (data & 0xF0) >> 4) | 0xC0; //Write Upper Nibble (RS=1) E --> 1
	PORTB = data;
	_delay_us(100);
	data ^= 0x80; // E --> 0
	PORTB = data;
	_delay_us(100);
	data = temp;
	data = ( (data & 0x0F) ) | 0xC0; //Write Lower Nibble (RS=1) E --> 1
	PORTB = data;
	_delay_us(100);
	data ^= 0x80; //E --> 0
	PORTB = data;
	_delay_us(100);
}
void lcd_string(char *string_of_characters)
{
	while(*string_of_characters)
	{
		lcd_char(*string_of_characters++);
	}
}
void main(void)
{
	lcd_init();
	USART_Init(MYUBRR);
	unsigned char data;
	DDRA=0xFF;
	while(1)
	{
		data=USART_Receive();
		_delay_ms(200);
		lcd_string(data);
		
		if (data == 0x41)
		{
			PORTA = 0xFF;
		} 
		else
		{
			PORTA = 0x00;
		}
		UDR0 = 0;
	}
}
