#include <msp430.h>
#include <stdio.h>

// === Pin definitions ===
#define LED       BIT0        // P1.0 onboard LED
#define RAIN_DO   BIT0        // P2.0 - Digital Rain signal
#define THRESHOLD 2200        // Adjust this value to make detection more sensitive

// === Function prototypes ===
void uart_init(void);
void uart_print(char *str);
unsigned int read_rain_analog(void);
unsigned char read_rain_digital(void);
void send_sensor_data(unsigned int rainValue, unsigned char rainDetected);

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog
    // === Clock setup: 1 MHz SMCLK ===
    UCSCTL3 = SELREF__REFOCLK;          // FLL reference = REFO
    UCSCTL4 |= SELA__REFOCLK;           // ACLK = REFO
    __bis_SR_register(SCG0);            // Disable FLL control loop
    UCSCTL0 = 0x0000;                   // Reset DCO and modulation
    UCSCTL1 = DCORSEL_2;                // DCO range ~1MHz
    UCSCTL2 = FLLD_1 | 30;              // (N + 1) * FLLRef = 32768 * 31 / 1 = ~1.013 MHz
    __bic_SR_register(SCG0);            // Enable FLL control loop
    __delay_cycles(250000);             // Wait for DCO to settle


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
        // Read analog and digital values
        unsigned int rainAnalog = read_rain_analog();
        unsigned char rainDigital = read_rain_digital();

        // === Combined detection logic ===
        // - rainAnalog < THRESHOLD  analog says "rain"
        // - rainDigital == 1  digital pin says "rain"
        //
        // You can require either one OR both depending on reliability preference.
        // Here we use OR (detect rain if either says so)
        unsigned char rainDetected = (rainAnalog < THRESHOLD) || (rainDigital == 1);

        // LED reflects detection
        if (rainDetected)
            P1OUT |= LED;
        else
            P1OUT &= ~LED;

        // Send both analog and digital data over UART
        send_sensor_data(rainAnalog, rainDetected);

        __delay_cycles(1000000); // ~1 sec delay (1 MHz clock)
    }
}

// === Read analog output (AO) ===
unsigned int read_rain_analog(void)
{
    ADC12CTL0 |= ADC12ENC | ADC12SC;   // Start conversion
    while (ADC12CTL1 & ADC12BUSY);     // Wait for finish
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
    sprintf(msg,
            "RainADC=%d, Threshold=%d, Rain=%s\r\n",
            rainValue,
            THRESHOLD,
            rainDetected ? "DETECTED" : "CLEAR");

    uart_print(msg);
}

void uart_init(void)
{
    P4SEL |= BIT4 + BIT5;           // P4.4 TX, P4.5 RX
    UCA1CTL1 |= UCSWRST;            // Put USCI in reset
    UCA1CTL1 |= UCSSEL_2;           // Use SMCLK (1 MHz)
    UCA1BR0 = 104;                  // 1MHz / 9600
    UCA1BR1 = 0;
    UCA1MCTL = UCBRS_1;             // Modulation
    UCA1CTL1 &= ~UCSWRST;           // Initialize USCI
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
