#include "common.h"
#include <stdio.h>

enum IR_STATE
{
	IR_IDLE, // idle state
	IR_DATA, // data
	IR_END   // the end of IR code
};

unsigned int newCount = 0; // new timer counter
unsigned int oldCount = 0; // old timer counter
unsigned char rxData[4]; // data buffer for IR receiver
unsigned char dataCount = 0; // byte counter
unsigned char edgeCount = 0; // edge counter
unsigned char bitCount = 0;	// bit counter
unsigned char headerCount = 0; // header edge counter
unsigned char irState; // IR decoder state machine

void BlinkLED();

int main()
{
	WDTCTL = WDTPW + WDTHOLD;

    Init_GPIO();
    Init_Clock();
	Init_UART(115200);

	P3SEL0 &= ~BIT0;
	P3SEL1 &= ~BIT0;
	P3OUT &= ~BIT0;
	P3DIR &= ~BIT0; // Mode input
	P3REN &= ~BIT0; 
  	P3IES |= BIT0;
  	P3IFG &= ~BIT0;
	P3IE |= BIT0;

	P1DIR |=  BIT0; // Set P1.0 as output
	P1OUT &= ~BIT0; // output low
	
	irState = IR_IDLE; // initialize decoder state machine

	__delay_cycles(100000);
	
	_enable_interrupts();

	TA0CTL = TASSEL_2 + MC_2 + TACLR;	// SMCLK, Continuous mode

	Print_UART("Receiver ready!\r\n");

	while(1)
	{
		if(rxData[2] == (unsigned char)(~rxData[3]) && rxData[0]==0x55 && rxData[1]==0xaa)
		{
			const unsigned char buttonNum = rxData[2];
			
			BlinkLED();

			Print_UART("Button ");
			Print_UART(Key_To_String(buttonNum));
			Print_UART(" pressed\r\n");
		}

		__bis_SR_register(LPM3_bits | GIE); // enter low power mode
	}

    return 0;
}

void BlinkLED ()
{
	unsigned char i;
	for(i = 0; i < 8; i++) {
		P1OUT ^= BIT0;
		__delay_cycles(300000);
	}
	P1OUT &= ~BIT0;
}

__attribute__((interrupt(PORT3_VECTOR)))
void PORT3_ISR(void)
{
	if(P3IFG & BIT0)
	{
		// We want to detect both rising and falling edges
		// so we invert the edge selection on each interrupt
		P3IES ^= BIT0;

		oldCount = newCount; // Update the counter value
		newCount = TA0R;
		const unsigned int timeCount = newCount - oldCount; // Time interval
		
		switch(irState) // IR decoder state machine
		{
			case IR_IDLE: // Header of IR code
			{
				unsigned char i;
				for(i = 0; i < 4; i++) {
					rxData[i] = 0x00; // Clear the data buffer
				}
				if(headerCount == 0) {
					headerCount = 1;
				}
				else if(headerCount == 1 && timeCount > 32000 && timeCount < 40000) {
					// 9ms leading pulse burst
					headerCount = 2;
				}
				else if(headerCount == 2 && timeCount > 14000 && timeCount < 22000) {
					// 4.5ms space
					headerCount = 0;
					irState = IR_DATA;
					edgeCount = 1;
				}
				else {
					headerCount = 0;
					P3IES |= BIT0;
				}

				break;
			}
			case IR_DATA: // Data of IR code(address + command)
			{
				if(timeCount > 32000 && timeCount < 40000)
				{
					edgeCount = 0; // Clear the counter
					P3IES |= BIT0;
					dataCount = 0;
					bitCount	 = 0;
					headerCount = 2;
					irState = IR_IDLE; // Enter idle state and wait for new code
					break;
				}
				else
				{
					edgeCount++; // Both rising edge and falling edge are captured

					if(edgeCount % 2 == 1)
					{
						dataCount = (edgeCount - 2) / 16;

						if(timeCount > 4500) {			// Space is 1.68ms
							rxData[dataCount] |= 0x80;	// Logical '1'
						} else {						// Space is 0.56ms
							rxData[dataCount] &= 0x7f;	// Logical '0'
						}
						
						if(bitCount != 7) {
							// Shifting based on Byte
							rxData[dataCount] = rxData[dataCount] >> 1;
							bitCount++;
						}
						else {
							bitCount = 0; // Start a new byte
						}
					}

					if(edgeCount > 64) {
						irState = IR_END; // The end of address and command
					}
					break;
				}
			}
			case IR_END: // The end of IR code
			{
				edgeCount = 0; // Clear the counter
				P3IES |= BIT0;
				dataCount = 0;
				bitCount	 = 0;
				if(timeCount > 2000)	{ // A final 0.56ms pulse burst
					irState = IR_IDLE; // Enter idle state and wait for new code
				}
				break;
			}
			default:
			{
				irState = IR_IDLE;	// Set idle state as default state
				headerCount = 0;	// Clear all the counter
				edgeCount = 0;
				P3IES |= BIT0;
				dataCount = 0;
				bitCount	 = 0;
				break;
			}
		}

		P3IFG &= ~BIT0;
		LPM3_EXIT; // Exit low power mode
	}
}
