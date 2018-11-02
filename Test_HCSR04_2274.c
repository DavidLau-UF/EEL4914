#include <msp430.h>

int main()
{
    int echo_pulse_duration;      // time in us
    int distance_cm;

    // Disable watchdog timer
    WDTCTL = WDTPW + WDTHOLD;

    // Make P1.0 a digital output for the LED and P1.6
    // a digital output for the sensor trigger pulses
    P4DIR = BIT0;
    P1DIR = BIT0;
    P1IN = BIT1;

    // Set up Timer_A1: SMCLK clock, input divider=1,
    // "continuous" mode. It counts from 0 to 65535,
    // incrementing once per clock cycle (i.e. every 1us).
    TACTL = TASSEL_2 + ID_0 + MC_2;

    // Now just monitor distance sensor
    while(1)
    {

        // Send a 20us trigger pulse
        P1OUT |= BIT0;                // trigger high
        __delay_cycles(20);           // 20us delay
        P1OUT &= ~BIT0;               // trigger low

        // Measure duration of echo pulse
        while ((P1IN & BIT1) == 0);   // Wait for start of echo pulse
        TAR = 0;                     // Reset timer at start of pulse
        while ((P1IN & BIT1) > 0);    // Wait for end of echo pulse
        echo_pulse_duration = TAR;   // Current timer value is pulse length
        distance_cm = echo_pulse_duration / 58; // Convert from us to cm

        if (distance_cm < 50) P4OUT |= BIT0; // LED on
        else P4OUT &= ~BIT0;                 // LED off
    }
}
