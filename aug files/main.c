#include <msp430.h>
#include <stdio.h>

// === Pin definitions ===
#define LED       BIT0        // P1.0 onboard LED
#define RAIN_DO   BIT0        // P2.0 - Digital Rain signal
#define THRESHOLD 2000        // Example ADC threshold

// === Function prototypes ===
void uart_init(void);
void uart_print(char *str);
unsigned int read_rain_analog(void);
unsigned char read_rain_digital(void);
void send_sensor_data(unsigned int rainValue, unsigned char rainDetected);

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog

    // --- I/O setup ---
    P1DIR |= LED;               // LED output
    P2DIR &= ~RAIN_DO;          // P2.0 input
    P2REN |= RAIN_DO;           // Enable pull resistor
    P2OUT |= RAIN_DO;           // Pull-up resistor

    // --- ADC setup for analog rain sensor (A0 = P6.0) ---
    P6SEL |= BIT0;
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12MCTL0 = ADC12INCH_0;   // A0

    // --- UART setup ---
    uart_init();
    uart_print("Starting Environmental Sensor Node...\r\n");

    while (1)
    {
        unsigned int rainAnalog = read_rain_analog();
        unsigned char rainDigital = read_rain_digital();

        // LED reflects digital signal (rain detected)
        if (rainDigital == 1)
            P1OUT |= LED;
        else
            P1OUT &= ~LED;

        // Send combined data (can later add temperature, humidity, etc.)
        send_sensor_data(rainAnalog, rainDigital);

        __delay_cycles(1000000); // 1 second delay (at 1 MHz)
    }
}

// === Read analog output (AO) ===
unsigned int read_rain_analog(void)
{
    ADC12CTL0 |= ADC12ENC | ADC12SC;
    while (ADC12CTL1 & ADC12BUSY);
    return ADC12MEM0;
}

// === Read digital output (DO) ===
unsigned char read_rain_digital(void)
{
    // DO = LOW when wet, HIGH when dry
    return ((P2IN & RAIN_DO) == 0) ? 1 : 0;
}

// === UART send data ===
void send_sensor_data(unsigned int rainValue, unsigned char rainDetected)
{
    char msg[64];
    sprintf(msg, "RainADC=%d, Rain=%s\r\n", rainValue,
            rainDetected ? "DETECTED" : "CLEAR");
    uart_print(msg);
}

// === UART setup ===
void uart_init(void)
{
    P4SEL |= BIT4 + BIT5;               // P4.4 TX, P4.5 RX
    UCA1CTL1 |= UCSWRST;
    UCA1CTL1 |= UCSSEL_2;               // SMCLK
    UCA1BR0 = 104;                      // 1MHz/9600
    UCA1BR1 = 0;
    UCA1MCTL = UCBRS_1;
    UCA1CTL1 &= ~UCSWRST;
}

// === UART print helper ===
void uart_print(char *str)
{
    while (*str)
    {
        while (!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = *str++;
    }
}
