#ifndef COMMON_H
#define COMMON_H

#include <msp430.h>

void Init_GPIO();

void Init_Clock();

#define KEYNUM 16 // button number of keypad
#define COL		4
#define ROW		4

void Init_KeypadIO();
unsigned char Scan_Key();
const char* Key_To_String(unsigned char key);

void Init_UART(unsigned long baudrate);
void Print_UART(const char* str);

#endif // COMMON_H
