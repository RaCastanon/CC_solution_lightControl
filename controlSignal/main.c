#include <msp430g2553.h>
#include <stdlib.h>
#include <string.h>

/*
 * Defines section
 */
#define FREQ                      125
#define MINIMUM_RATE              5
#define MAXIMUM_RATE              FREQ-5
#define MAXIMUM_ARRAY_DATA        2
#define ASCII_MIN_NUMBER          47
#define ASCII_MAX_NUMBER          58
/*
 * Struct declarations
 */
typedef struct data{
    char data_input[2];
    unsigned short int data_count;
} data;

/*
 * Global variables
 */
static data input_data;

void main(void) {
    //Watchdog configuration
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    //Clock configuration
    DCOCTL      = CALDCO_1MHZ;
    BCSCTL1     = CALBC1_1MHZ;
    BCSCTL2     = SELM0 + DIVS_0;
    BCSCTL3     = LFXT1S_2;

    //IO configuration
    P1DIR       =  BIT6;
    P1SEL       =  BIT6;
    P1OUT      &= ~BIT6;

    /* Configure Pin Muxing P1.1 RXD and P1.2 TXD */
    P1SEL      |= BIT1 + BIT2 ;
    P1SEL2      = BIT1 + BIT2;
    P1DIR      |= BIT1;
    UCA0CTL0    = 0x00;
    /* Configure */
    UCA0CTL1   |= UCSSEL_2;             // SMCLK
    UCA0BR0     = 104;                  // 1MHz 9600
    UCA0BR1     = 0;                    // 1MHz 9600
    UCA0MCTL    = UCBRS_1;              // Modulation UCBRSx = 1
    /* Take UCA0 out of reset */
    UCA0CTL1 &= ~UCSWRST;

    //Timer A0 configuration
    TA0CTL      = TACLR;
    TA0R        = 0;
    TA0CCR0     = FREQ;
    TA0CCR1     = MAXIMUM_RATE;
    TA0CCTL0    = CCIE;
    TA0CCTL1    = OUTMOD_2;
    TA0CTL      = TASSEL_2 + MC_1 + ID_3;

    //Interrupt configuration
    IE2 |= UCA0RXIE;                    //Enable USCI_A0 RX interrupt
    _BIS_SR(GIE);

    //Global variables initialization
    memset(input_data.data_input, '\0', sizeof(input_data.data_input));
    input_data.data_count = 0;

    //Infinite loop
    for(;;){

    }
}

#pragma vector=TIMER0_A0_VECTOR
   __interrupt void Timer0_A0 (void)
    {
       // Clean interruption flag
       TA0CCTL0 &= ~CCIFG;
    }

#pragma vector=USCIAB0RX_VECTOR
    __interrupt void USCI0RX_ISR(void)
    {
        // Local variable declarations
        static char buff_rx;
        static int pwm_val;

        while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
        buff_rx = UCA0RXBUF;

        // Verify if we have to process data or save data
        // To process data we have to receive '\n'
        if(buff_rx != '\n')
        {
            // Validate incoming data
            if((MAXIMUM_ARRAY_DATA > input_data.data_count) &&
                    (ASCII_MIN_NUMBER < buff_rx) && (ASCII_MAX_NUMBER > buff_rx))
            {
                // Add data to array
                input_data.data_input[input_data.data_count] = buff_rx;
                // Increment count from data received
                input_data.data_count++;
            }
            else
            {
                // Clean variables
                input_data.data_count = 0;
                memset(input_data.data_input, '\0', sizeof(input_data.data_input));
            }
        }
        else
        {
            // Validate count and data
            if((MAXIMUM_ARRAY_DATA >= input_data.data_count) && ('\0' != input_data.data_input[0]))
            {
                // Convert received data (duty cycle) in timer counts
                pwm_val = (int)((atoi(input_data.data_input))*128)/100;
                // Check limits
                if(pwm_val < MINIMUM_RATE)
                {
                    pwm_val = MINIMUM_RATE;
                }
                else if(pwm_val > MAXIMUM_RATE)
                {
                    pwm_val = MAXIMUM_RATE;
                }
                else
                {
                    // Do nothing
                }
                // Set pwm duty cycle
                TA0CCR1 = pwm_val;
                // Clean variables
                input_data.data_count = 0;
                memset(input_data.data_input, '\0', sizeof(input_data.data_input));
            }
        }
    }
