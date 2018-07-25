#include "common.h"

void Send_Data(const unsigned char* data, unsigned int length);

int main()
{
	WDTCTL = WDTPW + WDTHOLD;

    Init_GPIO();
    Init_Clock();
	Init_KeypadIO();
	Init_UART(115200);
	
	// configure P1.2 to send IR data
	P1SEL0 &= ~BIT2;
	P1SEL1 &= ~BIT2;
	P1DIR |= BIT2;
	P1OUT &= ~BIT2;

	__delay_cycles(100000);

	Print_UART("Emitter ready!\r\n");

	while(1)
	{
		const unsigned char key = Scan_Key();

		if(key)
		{
			const unsigned char data[4] = { 0x55, 0xaa, key, ~key };
			Send_Data(data, 4);
		}

		__bis_SR_register(LPM3_bits | GIE); // enter low power mode
	}

    return 0;
}

static inline void Send_High(unsigned int count)
{
	// 1 unit = 0.562ms = 21.356 carriers at 38kHz
	// after many tests it works far better when rounding
	// to 22 instead of 21
	const unsigned int carrierCount = count * 22;

	for(unsigned int i = 0; i < carrierCount; i++)
	{
		// 1 carrier = 26.316µs at 38kHz
		// 38kHz 1/4 duty-cycle carrier
		// 6.579µs high = 52.632 cycles high at 8MHz (1 cycle = 0.125µs)
		// 19.737µs low = 157.896 cycles low at 8MHz (1 cycle = 0.125µs)
		// round floor to handle bits changes overhead

		P1OUT |= BIT2;
		__delay_cycles(52);

		P1OUT &= ~BIT2;
		__delay_cycles(157 - 2); // minus 2 for loop overhead
	}
}

static inline void Send_Low(unsigned int count)
{
	// 1 unit = 562µs = 4496 cycles at 8MHz (1 cycle = 0.125µs)
	for(unsigned int i = 0; i < count; i++)
	{
		__delay_cycles(4496 - 2); // minus 2 for loop overhead
	}
}

void Send_Data(const unsigned char* data, unsigned int length)
{
	// do not disturb when sending data
	_disable_interrupts();

	const unsigned char* const end = data + length;

	Send_High(16); // 9ms high
	Send_Low(8); // 4.5ms low

	while(data < end)
	{
		for(unsigned char bit = 0; bit < 8; bit++)
		{
			if((*data & (1 << bit)) == 0)
			{
				Send_High(1); // 0.562ms high
				Send_Low(1); // 0.562ms low
			}
			else
			{
				Send_High(1); // 0.562ms high
				Send_Low(3);  // 1.686ms low
			}
		}

		data++;
	}

	Send_High(1); // 0.562ms high

	_enable_interrupts();
}
