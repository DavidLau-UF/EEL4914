#include <stdint.h>
#include <msp430.h>
void init_uart(void){//initialize uart
    P3SEL  |= BIT5+ BIT4; // P1.1 UCA0RXD input
    //P3SEL2 |= BIT5 + BIT4; // P1.2 UCA0TXD output
    UCA0CTL1 |= UCSSEL_2 + UCSWRST;     // use SMCLK
    UCA0BR0 = 104;            // 1MHz 9600 104 //
    UCA0BR1 = 0x0;              // 1MHz 9600
    UCA0MCTL = UCBRS_1;       //1 to select 9600
    UCA0CTL1 &= ~UCSWRST;     // uart becomes ready to operate
    IE2 |= UCA0RXIE;
}
void uartPutchar(unsigned char c){
    while (!(IFG2&UCA0TXIFG));    // stay here if USCI_A0 TX buffer is not empty
    UCA0TXBUF = c;     // USCI_A0 TX buffer is empty, a new char is loaded in to be sent
}
void uartPutstring(const char *str){
     while(*str) uartPutchar(*str++);
}
int main(void)
{
    init_uart();
    P4DIR = 0xFF;
    while(1)
    {
        while (!(IFG2&UCA0RXIFG));
        unsigned char ch = UCA0RXBUF;
        if(ch == 0x0F)
        {
            P4OUT = 0x01;
        }
        else if (ch == 0xF0)
        {
            P4OUT = 0x02;
        }
        else if (ch == 0x47)
        {
            P4OUT = 0x03;
        }
        else
        {
            P4OUT = 0x00;
        }
    }

}
