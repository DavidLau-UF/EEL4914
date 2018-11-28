#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#define I2C_PRESCALE        0x35        // sets clock frequency at about 165kHz
#define PCA9685_address 0x40
#define SDA_PIN BIT1
#define SCL_PIN BIT2
#define I2C_TRANSMIT_MODE       UCTR//UCB0TXIE//UCTR
#define I2C_RECEIVE_MODE        0x00//UCB0RXIE//0x00
int ADC_P1_init(void)
{
    int ADC_P1 = 0;
    ADC10CTL0 = SREF_0 | ADC10SHT_2 | ADC10ON;      //Ref from V_ref, 16x CLK, on
    ADC10CTL1 = INCH_0 | SHS_0 | ADC10DIV_0 | ADC10SSEL_0 | CONSEQ_0;
    //Input A0, 10bit sample, 1x divider, ADC clk source, single channel/conversation
    ADC10AE0 = BIT0;                                //Enable A0 input
    ADC10CTL0 |= ENC | ADC10SC;                               //Enable and start conversion
    while (ADC10CTL1 & BUSY);
    ADC_P1 = ADC10MEM;
    return ADC10MEM;
}
void uart_init(void){
    P3SEL |= BIT5;
    UCA0CTL1 |= UCSSEL_2 + UCSWRST;     // use SMCLK
    UCA0BR0 = 104;            // 1MHz 9600 104 //
    UCA0BR1 = 0x0;              // 1MHz 9600
    UCA0MCTL = UCBRS_1;       //1 to select 9600
    UCA0CTL1 &= ~UCSWRST;     // uart becomes ready to operate
    IE2 |= UCA0RXIE;
}
void I2C_masterInit() {
    //I2C_PxSEL |= SDA_PIN + SCL_PIN;             // Assign I2C pins to USCI_B0
    P3SEL |= SDA_PIN + SCL_PIN;                  // Assign I2C pins to USCI_B0
    //P1SEL2|= SDA_PIN + SCL_PIN;                  //p1.7 and p1.6
    //P1REN |= SDA_PIN + SCL_PIN;                 //enable pull up for both pins
    UCB0CTL1 = UCSWRST;                         // Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;       // I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;              // Use SMCLK, keep SW reset
    UCB0BR0 = I2C_PRESCALE;                         // set prescaler, I2C_PRESCALE
    UCB0BR1 = 0x00;
}
void I2C_enable ()
{
    //Reset the UCSWRST bit to enable the USCI Module
    UCB0CTL1 &= ~(UCSWRST);
}
void I2C_disable ()
{
    //Set the UCSWRST bit to disable the USCI Module
    UCB0CTL1 |= UCSWRST;
}
void I2C_setSlaveAddress (unsigned char slaveAddress)
{
    //Set the address of the slave with which the master will communicate.
    UCB0I2CSA = slaveAddress;
}
void I2C_setMode ( unsigned char mode ) {
    UCB0CTL1 &= ~I2C_TRANSMIT_MODE;
    UCB0CTL1 |= mode;
}
unsigned char I2C_isBusBusy ()
{
    //Return the bus busy status.
    return (UCB0STAT & UCBBUSY);
}
void I2C_masterSendSingleByte ( unsigned char txData   )
{
    //Store current TXIE status
    unsigned char txieStatus = IE2 & UCB0TXIE;
    //Disable transmit interrupt enable
    IE2 &= ~(UCB0TXIE);
    //Send start condition.
    UCB0CTL1 |= UCTR + UCTXSTT;
    //Poll for transmit interrupt flag.
    while (!(IFG2 & UCB0TXIFG)) ;
    //while (!(UC0IFG & UCB0TXIFG)) ;
    UCB0TXBUF = txData;
    while (!(IFG2 & UCB0TXIFG)) ;
    UCB0CTL1 |= UCTXSTP;
    IFG2 &= ~(UCB0TXIFG);
    IE2 |= txieStatus;
}
void I2C_masterMultiByteSendStart ( unsigned char txData )
{
    unsigned char txieStatus = IE2 & UCB0TXIE;
    IE2 &= ~(UCB0TXIE);
    UCB0CTL1 |= UCTR + UCTXSTT;//UCTR=1, write,
    while (!(IFG2 & UCB0TXIFG)) ;
    UCB0TXBUF = txData;
    IE2 |= txieStatus;
}

void I2C_masterMultiByteSendNext ( unsigned char txData )
{
    //If interrupts are not used, poll for flags
    if (!(IE2 & UCB0TXIE)){
        //Poll for transmit interrupt flag.
        while (!(IFG2 & UCB0TXIFG)) ;
    }
    //Send single byte data.
    UCB0TXBUF = txData;
}
void I2C_masterMultiByteSendFinish ( unsigned char txData )
{
    //If interrupts are not used, poll for flags
    if (!(IE2 & UCB0TXIE)){
        //Poll for transmit interrupt flag.
        while (!(IFG2 & UCB0TXIFG)) ;
    }
    //Send single byte data.
    UCB0TXBUF = txData;
    //Poll for transmit interrupt flag.
    while (!(IFG2 & UCB0TXIFG)) ;
    //Send stop condition.
    UCB0CTL1 |= UCTXSTP;
}
void I2C_masterMultiByteSendStop ()
{
    //If interrupts are not used, poll for flags
    if (!(IE2 & UCB0TXIE)){
        //Poll for transmit interrupt flag.
        while (!(IFG2 & UCB0TXIFG)) ;
    }
    //Send stop condition.
    UCB0CTL1 |= UCTXSTP;
}
void I2C_masterPCA9685_write (unsigned char regAddr, unsigned char txData2){
    I2C_setSlaveAddress(PCA9685_address);
    I2C_setMode(I2C_TRANSMIT_MODE);
    I2C_enable();
    I2C_masterMultiByteSendStart (regAddr);
    I2C_masterMultiByteSendFinish (txData2);
    while (I2C_isBusBusy()) ;
    I2C_disable();
}
void I2C_masterPCA9685_PWM_write(volatile uint8_t num,volatile uint16_t on, volatile uint16_t off){
//num= which pwm output, on, where in4096 to turn on, off, where in 4096 to be off
    I2C_setSlaveAddress(PCA9685_address);
    I2C_setMode(I2C_TRANSMIT_MODE);
    I2C_enable();
    I2C_masterMultiByteSendStart (0x06+4*num);
    I2C_masterMultiByteSendNext (on);
    I2C_masterMultiByteSendNext (on>>8);
    I2C_masterMultiByteSendNext (off);
    I2C_masterMultiByteSendFinish (off>>8);
    while (I2C_isBusBusy()) ;
    I2C_disable();
}
void reset_PCA9685(){
    I2C_setSlaveAddress(0x00);//reserved address
    I2C_setMode(I2C_TRANSMIT_MODE);
    I2C_enable();
    I2C_masterSendSingleByte (0x06);
    while (I2C_isBusBusy());
    I2C_disable();
}
void init_PCA9685(){
    I2C_masterInit();
    reset_PCA9685();
    delay();
    I2C_masterPCA9685_write (0x00, 0x11 );//sleep
    I2C_masterPCA9685_write (0xFE,0x7F);//set the freq prescaler in scale register
    delay();
    I2C_masterPCA9685_write(0x00, 0x21);//auto increment on, wake up
    delay();
}
void delay(){
    __delay_cycles(20000);
}
void long_delay(){
    __delay_cycles(50000);
}
int main(void)
{
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL  = CALDCO_1MHZ;
    TACTL = TASSEL_2 + ID_0 + MC_2;
    WDTCTL  = WDTPW + WDTHOLD;
    uart_init();
    init_PCA9685();
    P1DIR = BIT0;
    P2DIR |= BIT3;
    P2OUT &= ~BIT3;
    int echo_pulse_duration;      // time in us
    int distance_cm;

    while(1)
    {
        while (!(IFG2&UCA0RXIFG));
        unsigned char ch = UCA0RXBUF;
        P2OUT &= ~BIT3;
        if(ch == 0x81){
            while(1){
                P1OUT |= BIT0;                // trigger high
                __delay_cycles(20);           // 20us delay
                P1OUT &= ~BIT0;               // trigger low
                // Measure duration of echo pulse
                while ((P1IN & BIT1) == 0);   // Wait for start of echo pulse
                TAR = 0;                     // Reset timer at start of pulse
                while ((P1IN & BIT1) > 0);    // Wait for end of echo pulse
                echo_pulse_duration = TAR;   // Current timer value is pulse length
                distance_cm = echo_pulse_duration / 58; // Convert from us to cm
                unsigned char ch = UCA0RXBUF;
                if(distance_cm < 20){
                    P2OUT |= BIT3;
                }else{
                    P2OUT &= ~BIT3;
                    int P1 = ADC_P1_init();
                    if(P1 < 100){
                        I2C_masterPCA9685_PWM_write(13, 0, 350);
                    }else{
                        I2C_masterPCA9685_PWM_write(13, 0, 240);
                    }
                }
                if(ch == 0x80){
                    break;
                }
            }
        }else if(ch == 0x88){
            while(1){
                P1OUT |= BIT0;                // trigger high
                __delay_cycles(20);           // 20us delay
                P1OUT &= ~BIT0;               // trigger low
                while ((P1IN & BIT1) == 0);   // Wait for start of echo pulse
                TAR = 0;                     // Reset timer at start of pulse
                while ((P1IN & BIT1) > 0);    // Wait for end of echo pulse
                echo_pulse_duration = TAR;   // Current timer value is pulse length
                distance_cm = echo_pulse_duration / 58; // Convert from us to cm
                unsigned char ch = UCA0RXBUF;
                if(distance_cm < 15){
                    P2OUT |= BIT3;
                }else{
                    P2OUT &= ~BIT3;
                    if(ch == 0x89){
                        I2C_masterPCA9685_PWM_write(0, 0, 200);
                        delay();
                        I2C_masterPCA9685_PWM_write(5, 0, 430);
                        delay();
                        I2C_masterPCA9685_PWM_write(13, 0, 350);
                        delay();
                        I2C_masterPCA9685_PWM_write(8, 0, 390);
                        delay();
                    }else if(ch == 0x90){
                        I2C_masterPCA9685_PWM_write(0, 0, 200);
                        delay();
                        I2C_masterPCA9685_PWM_write(5, 0, 430);
                        delay();
                        I2C_masterPCA9685_PWM_write(13, 0, 240);
                        delay();
                        I2C_masterPCA9685_PWM_write(8, 0, 275);
                        delay();
                    }else if(ch == 0x92){
                        I2C_masterPCA9685_PWM_write(0, 0, 200);
                        delay();
                        I2C_masterPCA9685_PWM_write(5, 0, 310);
                        delay();
                        I2C_masterPCA9685_PWM_write(13, 0, 240);
                        delay();
                        I2C_masterPCA9685_PWM_write(8, 0, 390);
                        delay();
                    }else if(ch == 0x91){
                        I2C_masterPCA9685_PWM_write(0, 0, 310);
                        delay();
                        I2C_masterPCA9685_PWM_write(5, 0, 430);
                        delay();
                        I2C_masterPCA9685_PWM_write(13, 0, 240);
                        delay();
                        I2C_masterPCA9685_PWM_write(8, 0, 390);
                        delay();
                    }
                }
                if(ch == 0x80){
                    break;
                }
            }
        }
    }
}
void stand(){
    I2C_masterPCA9685_PWM_write(2, 0, 330);
    delay();
    I2C_masterPCA9685_PWM_write(3, 0, 305);
    delay();
    I2C_masterPCA9685_PWM_write(10, 0, 290);
    delay();
    I2C_masterPCA9685_PWM_write(11, 0, 340);
    delay();
    I2C_masterPCA9685_PWM_write(0, 0, 200);
    delay();
    I2C_masterPCA9685_PWM_write(5, 0, 430);
    delay();
    I2C_masterPCA9685_PWM_write(13, 0, 240);
    delay();
    I2C_masterPCA9685_PWM_write(8, 0, 390);
    delay();
    I2C_masterPCA9685_PWM_write(1, 0, 225);
    delay();
    I2C_masterPCA9685_PWM_write(4, 0, 400);
    delay();
    I2C_masterPCA9685_PWM_write(12, 0, 230);
    delay();
    I2C_masterPCA9685_PWM_write(9, 0, 400);
    delay();
}
