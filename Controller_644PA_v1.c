#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define FOSC 1000000
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
void ADC_JSX_init()														//Joystick X-axis
{
	ADMUX = (1<<REFS0)|(1<<MUX0)|(1<<MUX2);								//Set reference voltage to 3.3V
	ADCSRA |=(1<<ADEN)|(1<<ADATE)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
	_delay_ms(100);
	ADCSRA |=(1<<ADSC);													//ADC run continuously
	_delay_ms(200);
}
void ADC_JSY_init()														//Joystick Y-axis
{
	ADMUX = (1<<REFS0)|(1<<MUX2);										//Set reference voltage to 3.3V
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
void output_init()														//UART TX/RX and switches
{
	DDRA |= (0<<PORTA0)|(0<<PORTA1)|(0<<PORTA6);					//Input pin for switches and joystick
	DDRD |= (0<<PORTD0)|(1<<PORTD1);								//Output pin for UART
}
void USART_Init(unsigned int ubrr)										//Reference from manual - UART initialize
{
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 1stop bit */
	UCSR0C = (0<<USBS0)|(3<<UCSZ00);
}
unsigned char USART_Receive( void )										//Reference from manual - UART RX
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) )
	;
	/* Get and return received data from buffer */
	return UDR0;
}
void USART_Transmit( unsigned char data )								//Reference from manual - UART TX
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}
void main(void)
{
	char cord_x[5], cord_y[5];											//System initialize
	float cord_xx, cord_yy;
	int i, j;
	USART_Init(MYUBRR);
	lcd_init();
	output_init();
	lcd_command(0x01);
	lcd_string("WELCOME USER");											//System ready for user
	lcd_command(0xC0);
	lcd_string("SYSTEM ONLINE");
	_delay_ms(2500);
	while(1)
	{
		_delay_ms(100);
		USART_Transmit(0x00);											//Return to original position
		_delay_ms(2500);
		if ((PINA & 0x03) == 0x03)										//Autonomous mode
		{
			USART_Transmit(0x01);										//Trigger send, ready to engage
			lcd_command(0x01);
			lcd_string("Autonomous mode");
			lcd_command(0xC0);
			lcd_string("UGV ONLINE");
			_delay_ms(2500);
			i = 0;
			while (1)
			{
				if ((PINA & 0x43) != 0x43)								//Press joystick key to between sentry mode or seeker mode
				{
					_delay_ms(100);
					USART_Transmit(0x02);
					i++;
					_delay_ms(100);
				}
				if (i % 2 != 1)											//Display Mode
				{
					lcd_command(0x01);
					lcd_string("SENTRY MODE");
				}
				else
				{
					lcd_command(0x01);
					lcd_string("SEEKER MODE");
				}
				if ((PINA & 0x03) != 0x03)								//Check switch for mode
				{
					break;												//Back to main loop
				}
			}
		}else if ((PINA & 0x03) == 0x02)								//Testing mode
		{
			USART_Transmit(0x03);										//Trigger send, ready for testing
			lcd_command(0x01);
			lcd_string("Testing");
			_delay_ms(250);
			while (1)
			{
				ADC_JSX_init();
				cord_xx = (int)ADC;
				dtostrf(cord_xx, 5, 1, cord_x);
				ADC_JSY_init();
				cord_yy = (int)ADC;
				dtostrf(cord_yy, 5, 1, cord_y);
				if (cord_xx < 400)										//Lean left
				{
					_delay_ms(100);
					USART_Transmit(0x04);
					lcd_command(0x01);
					lcd_string("LEAN LEFT");
					_delay_ms(1500);
				}else if (cord_xx >= 600)								//Lean right
				{
					_delay_ms(100);
					USART_Transmit(05);
					lcd_command(0x01);
					lcd_string("LEAN RIGHT");
					_delay_ms(1500);
				}else if (cord_yy < 400)								//Lean back
				{
					_delay_ms(100);
					USART_Transmit(0x06);
					lcd_command(0x01);
					lcd_string("LEAN BACK");
					_delay_ms(1500);
				}else if (cord_yy >= 600)								//Lean front
				{
					_delay_ms(100);
					USART_Transmit(0x07);
					lcd_command(0x01);
					lcd_string("LEAN FRONT");
					_delay_ms(1500);
				}else
				{
					lcd_command(0x01);
					lcd_string("TESTING");
				}
				if ((PINA & 0x03) != 0x02)								//Back to main loop
				{
					break;
				}
			}
		}else
		{
			_delay_ms(100);
			USART_Transmit(0x08);
			lcd_command(0x01);
			lcd_string("Manual Mode");
			_delay_ms(250);
			i = 0;
			while(1)
			{
				ADC_JSX_init();											//Read joystick value
				cord_xx = (int)ADC;
				dtostrf(cord_xx, 5, 1, cord_x);
				ADC_JSY_init();
				cord_yy = (int)ADC;
				dtostrf(cord_xx, 5, 1, cord_y);
				if (cord_xx < 400)										//Turn has higher priority
				{
					_delay_ms(100);
					USART_Transmit(0x09);
					lcd_command(0x01);
					lcd_string("TURN LEFT");
					_delay_ms(100);
				}else if (cord_xx >= 600)								//Turn right
				{
					_delay_ms(100);
					USART_Transmit(0x10);
					lcd_command(0x01);
					lcd_string("TURN RIGHT");
					_delay_ms(100);
				}else if (cord_yy < 400)								//Move backward
				{
					_delay_ms(100);
					USART_Transmit(0x11);
					lcd_command(0x01);
					lcd_string("MOVE BACKWARD");
					_delay_ms(100);
				}else if (cord_yy >= 600)								//Move foreword
				{
					_delay_ms(100);
					USART_Transmit(0x12);
					lcd_command(0x01);
					lcd_string("MOVE FORWARD");
					_delay_ms(100);
				}
				if ((PINA & 0x40) != 0x40)								//Press joystick key to between sentry mode or seeker mode
				{
					_delay_ms(100);
					USART_Transmit(0x13);
					i++;
					_delay_ms(100);
				}
				if (i % 2 != 1)											//Display Mode
				{
					lcd_command(0x01);
					lcd_string("NORMAL MODE");
				}
				else
				{
					lcd_command(0x01);
					lcd_string("CLIMBING MODE");
				}
				if ((PINA & 0x02) != 0x00)								//Back to main loop
				{
					break;
				}
			}
		}
	}
}
