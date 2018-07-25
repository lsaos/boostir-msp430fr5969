#include "common.h"

unsigned char buttonDebounce = 1;

void Init_GPIO()
{
    P1OUT = 0;
    P2OUT = BIT2;
    P3OUT = BIT0;
    P4OUT = 0;
    PJOUT = 0;

    P1DIR = 0xff;
    P2DIR = 0xff & (~BIT2);
    P3DIR = 0xff & (~BIT0);
    P4DIR = 0xff;
    PJDIR = 0xff;

	PM5CTL0 &= ~LOCKLPM5;
}

void Init_Clock()
{
    PJSEL0 |= BIT4 | BIT5;
    
    // Setup XT1
    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
    CSCTL1 = DCOFSEL_6;                       // Set DCO to 8MHz
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK; // set ACLK = XT1; MCLK = DCO
    CSCTL3 = DIVA__1 | DIVS__2 | DIVM__1;     // Set all dividers
    CSCTL4 &= ~LFXTOFF;
    do
    {
        CSCTL5 &= ~LFXTOFFG;                    // Clear XT1 fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
    CSCTL0_H = 0;                             // Lock CS registers
}

void Init_KeypadIO()
{
    //Set P1.3/1.4/1.5/4.3 as keypad input
	P1DIR &= ~(BIT3 + BIT4 + BIT5);
	P4DIR &= ~BIT3;
    
    //Pull up P1.3/1.4/1.5/4.3
	P1REN |= (BIT3 + BIT4 + BIT5);
	P4REN |= BIT3;

    //P1.3/1.4/1.5/4.3 output high
	P1OUT |= (BIT3 + BIT4 + BIT5);
	P4OUT |= BIT3;

    //Set P1.3/1.4/1.5/4.3 interrupt edge(high to low)
	P1IES |= (BIT3 + BIT4 + BIT5);
	P4IES |= BIT3;

    //Enable P1.3/1.4/1.5/4.3 interrupt
	P1IE  |= (BIT3 + BIT4 + BIT5);
	P4IE  |= BIT3;

    //Set P2.4/4.2/2.6/3.4 as keypad output
	P2DIR |= (BIT4 + BIT6);
	P4DIR |= BIT2;
	P3DIR |= BIT4;

    //P2.4/4.2/2.6/3.4 output low
	P2OUT &= ~(BIT4 + BIT6);
	P4OUT &= ~BIT2;
	P3OUT &= ~BIT4;

	P1IFG &= ~(BIT3 + BIT4 + BIT5); //Clear Port1 IFG
	P2IFG &= ~(BIT4 + BIT6); //Clear Port2 IFG
    P3IFG &= ~(BIT4); //Clear Port3 IFG
    P4IFG &= ~(BIT2); //Clear Port4 IFG

    _enable_interrupts();
}

unsigned char Scan_Key()
{
	unsigned char i = 0;
    unsigned char keyNum = 0;

	for (i = 0; i < COL; i++)
	{
        P2DIR &= ~(BIT4 + BIT6);
        P4DIR &= ~BIT2;
        P3DIR &= ~BIT4;

		switch(i)
		{
		case 0:
			P4OUT &= ~BIT2;
			P4DIR |= BIT2;
            break;
		case 1:
			P2OUT &= ~BIT6;
			P2DIR |= BIT6;
            break;
		case 2:
			P2OUT &= ~BIT4;
			P2DIR |= BIT4;
            break;
		case 3:
			P3OUT &= ~BIT4;
			P3DIR |= BIT4;
            break;
		}

		__delay_cycles(100);

        // find the pressed button row
		if((P1IN & BIT5) == 0) {
			keyNum = (0 + i * ROW  + 1);
            break;
        }

		if((P1IN & BIT4) == 0) {
			keyNum = (1 + i * ROW  + 1);
            break;
        }

		if((P1IN & BIT3) == 0) {
			keyNum = (2 + i * ROW  + 1);
            break;
        }

		if((P4IN & BIT3) == 0) {
			keyNum = (3 + i * ROW  + 1);
            break;
        }
	}

    //Set P2.4/4.2/2.6/3.4 as keypad output
	P2DIR |= (BIT4 + BIT6);
	P4DIR |= BIT2;
	P3DIR |= BIT4;

    //P2.4/4.2/2.6/3.4 output low
	P2OUT &= ~(BIT4 + BIT6);
	P4OUT &= ~BIT2;
	P3OUT &= ~BIT4;

	return keyNum;
}

const char* Key_To_String(unsigned char key)
{
    switch(key)
    {
        case 1: return "OK";
        case 2: return "Copy";
        case 3: return "Temp-";
        case 4: return "Temp+";
        case 5: return "Cool";
        case 6: return "9";
        case 7: return "6";
        case 8: return "3";
        case 9: return "0";
        case 10: return "8";
        case 11: return "5";
        case 12: return "2";
        case 13: return "Power";
        case 14: return "7";
        case 15: return "4";
        case 16: return "1";
        default: return "Unknown";
    }
}

void Buttons_startWDT()
{
	// WDT as 250ms interval counter
	SFRIFG1 &= ~WDTIFG;
	WDTCTL = WDTPW + WDTSSEL_1 + WDTTMSEL + WDTCNTCL + WDTIS_5;
	SFRIE1 |= WDTIE;
}

__attribute__((interrupt(WDT_VECTOR)))
void WDT_ISR(void)
{
	if (buttonDebounce == 2)
	{
		buttonDebounce = 1;

		SFRIFG1 &= ~WDTIFG;
		SFRIE1 &= ~WDTIE;
		WDTCTL = WDTPW + WDTHOLD;
	}
}

__attribute__((interrupt(PORT1_VECTOR)))
void PORT1_ISR(void)
{
	P1IFG = 0; // clear IFG

	if (buttonDebounce == 1)
	{
		buttonDebounce = 2;
		Buttons_startWDT();
		LPM3_EXIT; // exit low power mode
	}
}

__attribute__((interrupt(PORT4_VECTOR)))
void PORT4_ISR(void)
{
	P4IFG = 0; // clear IFG

	if (buttonDebounce == 1)
	{
		buttonDebounce = 2;
		Buttons_startWDT();
		LPM3_EXIT; // exit low power mode
	}
}

void Init_UART(unsigned long baudrate)
{
    P2SEL1 |= BIT0 | BIT1; // USCI_A0 UART operation
    P2SEL0 &= ~(BIT0 | BIT1);

    const unsigned long clockSpeed = 4000000;

    const unsigned long divInt = clockSpeed / baudrate;
    const unsigned long divFracNumerator = clockSpeed - (divInt * baudrate);
    const unsigned long divFracNumX8 = divFracNumerator * 8;
    const unsigned long divFracX8 = divFracNumX8 / baudrate;

    unsigned int brsVal;
    
    if(((divFracNumX8 - (divFracX8 * baudrate)) * 10) / baudrate < 5) {
        brsVal = divFracX8 << 1;
    } else {
        brsVal = (divFracX8 + 1) << 1;
    }

    UCA0CTLW0 = UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

    UCA0BR0 = divInt & 0x00ff;
    UCA0BR1 = (divInt >> 8) & 0xff;
    UCA0MCTLW |= brsVal;
    
    UCA0CTLW0 &= ~UCSWRST;
    UCA0IE |= UCRXIE;
}

void Print_UART(const char* str)
{
    while(*str)
    {
        while(!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = *str;
        str++;
    }
}
