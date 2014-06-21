#ifndef main_h
#define main_h

#include <stdint.h>
#include <stdio.h>
#include <avr/pgmspace.h> // descriptor must be stored in flash memory

#include "kbrd_codes.h"
#include "xbmc_keys.h"
#include "nec_defaults.h"

// compilation settings check
#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
#if F_CPU != 16500000
#error "ATtiny MCU used but not at 16.5 MHz"
#endif
#else
#if F_CPU != 12000000
#error "Clock speed is not 12 MHz"
#endif
#endif

// hardware pin mapping
#define IN_PORTx	PORTB
#define IN_DDRx		DDRB
#define IN_PINx		PINB
#define IN_PINNUM	2
#define IN_PINMASK	_BV(IN_PINNUM)
#define JMP_PORTx	PORTB
#define JMP_DDRx	DDRB
#define JMP_PINx	PINB
#define JMP_PINNUM	1
#define JMP_PINMASK	_BV(JMP_PINNUM)
#define LED_PORTx	PORTB
#define LED_DDRx	DDRB
#define LED_PINx	PINB
#define LED_PINNUM	0
#define LED_PINMASK	_BV(LED_PINNUM)

// timing constants
#if (F_CPU == 12000000)
#define PULSEWIDTH_INITIAL_9MS		100
#define PULSEWIDTH_ON_MIN			3
#define PULSEWIDTH_BIT_THRESHOLD	13 // 1125 us
#define PULSEWIDTH_2MS				23
#define PULSEWIDTH_3MS				35
#define PULSEWIDTH_4MS				47
#define PULSEWIDTH_5MS				59
#define TMR1_TIMEOUT_90MS			67
#define TMR1_TIMEOUT_120MS			90
#define TMR1_TIMEOUT_3S				9
#define TMR1_TIMEOUT_5S				15
#elif (F_CPU == 16500000)
#define PULSEWIDTH_INITIAL_9MS		137
#define PULSEWIDTH_ON_MIN			4
#define PULSEWIDTH_BIT_THRESHOLD	18 // 1125 us
#define PULSEWIDTH_2MS				32
#define PULSEWIDTH_3MS				48
#define PULSEWIDTH_4MS				65
#define PULSEWIDTH_5MS				81
#define TMR1_TIMEOUT_90MS			90
#define TMR1_TIMEOUT_120MS			120
#define TMR1_TIMEOUT_3S				12
#define TMR1_TIMEOUT_5S				20
#endif

// data structure for boot protocol keyboard report
// see HID1_11.pdf appendix B section 1
typedef struct {
	uint8_t report_id;
	uint8_t modifier;
	uint8_t reserved;
	uint8_t keycode[5];
} keyboard_report_t;

// result return codes for ir_cap()
typedef enum
{
	IRCAP_NOTHING,
	IRCAP_BUSY,
	IRCAP_NEWKEY,
	IRCAP_REPEATKEY,
	IRCAP_ERROR
}
ircap_res_t;

typedef struct
{
	const uint32_t	c;
	const char*	s;
}
code_desc_t;

ircap_res_t ir_cap(uint32_t*);
void usbPollWrapper();
uint32_t ir_to_kb(uint32_t);
uint32_t usr_ir_to_kb(uint32_t);
void usr_prog();

#endif