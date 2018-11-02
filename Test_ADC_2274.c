#include <msp430.h> 
#include <stdint.h>
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
void output_init()
{
    P4DIR = 0x01;
}
int main(void)
{
    output_init();
    while(1)
    {

        int P1P = ADC_P1_init();
        if (P1P > 900)             //No light
        {
            P4OUT = 0x01;
        }else
        {
            P4OUT = 0x00;
        }

    }
}
