#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define FOSC 1000000
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
void ADC_JSY_init()
{
	ADMUX = (1<<REFS0)|(1<<MUX2);										//Set reference voltage to 3.3V
	ADCSRA |=(1<<ADEN)|(1<<ADATE)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
	_delay_ms(100);
	ADCSRA |=(1<<ADSC);													//ADC run continuously
	_delay_ms(200);
}
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
void ADC_JSX_init()
{
	ADMUX = (1<<REFS0)|(1<<MUX2)|(1<<MUX0);								//Set reference voltage to 3.3V
	ADCSRA |=(1<<ADEN)|(1<<ADATE)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
	_delay_ms(100);
	ADCSRA |=(1<<ADSC);													//ADC run continuously
	_delay_ms(200);
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
void output_init()
{
	DDRD |= (0<<PORTD3)|(0<<PORTD4);					//Input pin
	DDRA |= (0<<PORTA0)|(0<<PORTA1)|(0<<PORTA6);
}

int main (void)
{
	char cord_x[5], cord_y[5];
	float cord;
	lcd_init();
	output_init();
	lcd_command(0x01);								
	lcd_string("WELCOME");							//System initialize
	lcd_command(0xC0);
	lcd_string("SYSTEM ONLINE");
	_delay_ms(500);
	while(1)
	{
		if ((PINA & 0x03) == 0x03)					//Both switches are closed, fully autonomous mode, user cannot control
		{
			lcd_command(0x01);
			lcd_string("UGV IS ONLINE");
			lcd_command(0xC0);
			lcd_string("AUTONOMOUS MODE");
		}
		else if ((PINA & 0x02) == 0x02)				//First switch is closed, 
		{
			lcd_command(0x01);
			lcd_string("Stage 1");
			
		}
		else if ((PINA & 0x01) == 0x01)				//Second switch is closed
		{
			lcd_command(0x01);
			lcd_string("UGV IS ONLINE");
			
		}
		else
		{	if ((PINA & 0x40) == 0x40)				//Both switches are open, user has full control, robot movement
			{
				ADC_JSX_init();						//ADC module on, read joystick data
				lcd_command(0x01);					//clear LCD
				cord = (int)ADC;
				dtostrf(cord, 5, 1, cord_x);
				lcd_string("X: ");
				lcd_string(cord_x);
				ADC_JSY_init();
				lcd_command(0xC0);
				cord = (int)ADC;
				dtostrf(cord, 5, 1, cord_y);
				lcd_string("Y: ");
				lcd_string(cord_y);
			}
			else
			{
				lcd_command(0x01);					//Joystick switch pushed, adjust robot height
				lcd_string("FIRING");
			}
		}
	}
}
