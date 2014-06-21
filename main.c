// project: IR-Keyboard, for Adafruit Industries, by Frank Zhao
// Takes IR remote control input (NEC extended protocol) and translates them into multimedia keyboard keystrokes

// required avr-libc modules, see http://www.nongnu.org/avr-libc/user-manual/modules.html
#include <avr/io.h> // allows access to AVR hardware registers
#include <avr/interrupt.h> // allows enabling/disabling and declaring interrupts
#include <util/delay.h> // includes delay functions
#include <avr/wdt.h> // allows enabling/disabling watchdog timer
#include <avr/pgmspace.h> // descriptor must be stored in flash memory
#include <avr/eeprom.h> // text file and calibration data is stored in EEPROM
#include <stdio.h> // allows streaming strings
#include <stdlib.h> // random numbers

// configure settings for V-USB then include the V-USB driver so V-USB uses your settings
#include "usbconfig.h"
#include "usbdrv/usbdrv.h"

#include "main.h"

// USB HID report descriptor for boot protocol keyboard
// see HID1_11.pdf appendix B section 1
// USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH is defined in usbconfig
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
	0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,       // USAGE (Keyboard)
	0xA1, 0x01,       // COLLECTION (Application)
	0x85, 0x01,       // REPORT_ID (1)
	0x75, 0x01,       //   REPORT_SIZE (1)
	0x95, 0x08,       //   REPORT_COUNT (8)
	0x05, 0x07,       //   USAGE_PAGE (Keyboard)(Key Codes)
	0x19, 0xE0,       //   USAGE_MINIMUM (Keyboard LeftControl)(224)
	0x29, 0xE7,       //   USAGE_MAXIMUM (Keyboard Right GUI)(231)
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
	0x81, 0x02,       //   INPUT (Data,Var,Abs) ; Modifier byte
	0x95, 0x01,       //   REPORT_COUNT (1)
	0x75, 0x08,       //   REPORT_SIZE (8)
	0x81, 0x03,       //   INPUT (Cnst,Var,Abs) ; Reserved byte
	0x95, 0x05,       //   REPORT_COUNT (5)
	0x75, 0x01,       //   REPORT_SIZE (1)
	0x05, 0x08,       //   USAGE_PAGE (LEDs)
	0x19, 0x01,       //   USAGE_MINIMUM (Num Lock)
	0x29, 0x05,       //   USAGE_MAXIMUM (Kana)
	0x91, 0x02,       //   OUTPUT (Data,Var,Abs) ; LED report
	0x95, 0x01,       //   REPORT_COUNT (1)
	0x75, 0x03,       //   REPORT_SIZE (3)
	0x91, 0x03,       //   OUTPUT (Cnst,Var,Abs) ; LED report padding
	0x95, 0x05,       //   REPORT_COUNT (5)
	0x75, 0x08,       //   REPORT_SIZE (8)
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x26, 0xA4, 0x00, //   LOGICAL_MAXIMUM (164)
	0x05, 0x07,       //   USAGE_PAGE (Keyboard)(Key Codes)
	0x19, 0x00,       //   USAGE_MINIMUM (Reserved (no event indicated))(0)
	0x2A, 0xA4, 0x00, //   USAGE_MAXIMUM (Keyboard Application)(164)
	0x81, 0x00,       //   INPUT (Data,Ary,Abs)
	0xC0,             // END_COLLECTION

#ifdef ENABLE_CONSUMER
	// this second consumer report is what handles the multimedia keys
	0x05, 0x0C,       // USAGE_PAGE (Consumer Devices)
	0x09, 0x01,       // USAGE (Consumer Control)
	0xA1, 0x01,       // COLLECTION (Application)
	0x85, 0x02,       //   REPORT_ID (2)
	0x19, 0x00,       //   USAGE_MINIMUM (Unassigned)
	0x2A, 0x3C, 0x02, //   USAGE_MAXIMUM
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x26, 0x3C, 0x02, //   LOGICAL_MAXIMUM
	0x95, 0x01,       //   REPORT_COUNT (1)
	0x75, 0x10,       //   REPORT_SIZE (16)
	0x81, 0x00,       //   INPUT (Data,Ary,Abs)
	0xC0,             // END_COLLECTION
#endif

#ifdef ENABLE_SYS_CONTROL
	// system controls, like power, needs a 3rd different report and report descriptor
	0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
	0x09, 0x80,       // USAGE (System Control)
	0xA1, 0x01,       // COLLECTION (Application)
	0x85, 0x03,       //   REPORT_ID (3)
	0x95, 0x01,       //   REPORT_COUNT (1)
	0x75, 0x02,       //   REPORT_SIZE (2)
	0x15, 0x01,       //   LOGICAL_MINIMUM (1)
	0x25, 0x03,       //   LOGICAL_MAXIMUM (3)
	0x09, 0x82,       //   USAGE (System Sleep)
	0x09, 0x81,       //   USAGE (System Power)
	0x09, 0x83,       //   USAGE (System Wakeup)
	0x81, 0x60,       //   INPUT
	0x75, 0x06,       //   REPORT_SIZE (6)
	0x81, 0x03,       //   INPUT (Cnst,Var,Abs)
	0xC0,             // END_COLLECTION
#endif

#ifdef ENABLE_MOUSE
	0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
	0x09, 0x02,       // USAGE (Mouse)
	0xa1, 0x01,       // COLLECTION (Application)
	0x09, 0x01,       //   USAGE (Pointer)
	0xA1, 0x00,       //   COLLECTION (Physical)
	0x85, 0x04,       //     REPORT_ID (4)
	0x05, 0x09,       //     USAGE_PAGE (Button)
	0x19, 0x01,       //     USAGE_MINIMUM
	0x29, 0x03,       //     USAGE_MAXIMUM
	0x15, 0x00,       //     LOGICAL_MINIMUM (0)
	0x25, 0x01,       //     LOGICAL_MAXIMUM (1)
	0x95, 0x03,       //     REPORT_COUNT (3)
	0x75, 0x01,       //     REPORT_SIZE (1)
	0x81, 0x02,       //     INPUT (Data,Var,Abs)
	0x95, 0x01,       //     REPORT_COUNT (1)
	0x75, 0x05,       //     REPORT_SIZE (5)
	0x81, 0x03,       //     INPUT (Const,Var,Abs)
	0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
	0x09, 0x30,       //     USAGE (X)
	0x09, 0x31,       //     USAGE (Y)
	0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
	0x25, 0x7F,       //     LOGICAL_MAXIMUM (127)
	0x75, 0x08,       //     REPORT_SIZE (8)
	0x95, 0x02,       //     REPORT_COUNT (2)
	0x81, 0x06,       //     INPUT (Data,Var,Rel)
	0xC0,             //   END_COLLECTION
	0xC0,             // END COLLECTION
#endif
};

#define OSCCAL_EEADDR         (const uint8_t *)(E2END - 2)
#define MMKEY_TRANSLATE_EEADDR (const uint8_t *)(E2END - 4)

// private local function prototypes
uint32_t ir_to_kb(uint32_t);
void send_report_once();
void ASCII_to_keycode(uint8_t);
void type_out_char(uint8_t, FILE*);
static FILE mystdout = FDEV_SETUP_STREAM(type_out_char, NULL, _FDEV_SETUP_WRITE); // setup writing stream

// global variables

static keyboard_report_t keyboard_report;
#define keyboard_report_reset() do { *((uint32_t*)(&keyboard_report)) &= 0x000000FF; } while (0)
static uint8_t idle_rate = 500 / 4; // see HID1_11.pdf sect 7.2.4
static uint8_t protocol_version = 0; // see HID1_11.pdf sect 7.2.6
static uint8_t LED_state = 0; // see HID1_11.pdf appendix B section 1
static char report_pending = 0; // if we need to send out a report
static int8_t bit_idx = 0; // bit index of current reception
#define INDICATE_ERROR -10 // used for bit_idx to indicate error
static volatile char has_commed = 0; // if the host made any usb requests
static uint32_t ir_code = 0; // current IR code being received
static uint32_t last_keycode = 0; // the last keycode, used for key holding
#ifdef ENABLE_TIMEBUFF_DEBUG
static uint8_t time_buff[32*3];
static uint8_t time_buff_idx = 0;
#endif


#define buttonPressed()  bit_is_clear(JMP_PINx, JMP_PINNUM)
uint8_t tryProgram = 1;

#if defined(ENABLE_DEFAULT_CODES) || defined(ENABLE_APPLE_DEFAULTS)
#include <nec_defaults.h>
#if defined(ENABLE_DEFAULT_CODES)
const PROGMEM uint32_t ir_but_tbl[] = IR_BUT_PAIRS;
#endif
#endif

// delays a certain number of ms, but also servicing USB requests at the same time
static void usb_polling_delay_ms(uint8_t x)
{
	uint8_t y;
	for (y = 0; y < (x); y++) {
		usbPoll();
		_delay_ms(1);
	}
}

// see http://vusb.wikidot.com/driver-api
// constants are found in usbdrv.h
usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
	has_commed = 1;

	// see HID1_11.pdf sect 7.2 and http://vusb.wikidot.com/driver-api
	usbRequest_t *rq = (void *)data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS)
		return 0; // ignore request if it's not a class specific request

	// see HID1_11.pdf sect 7.2
	switch (rq->bRequest)
	{
		case USBRQ_HID_GET_IDLE:
			usbMsgPtr = &idle_rate; // send data starting from this byte
			return 1; // send 1 byte
		case USBRQ_HID_SET_IDLE:
			idle_rate = rq->wValue.bytes[1]; // read in idle rate
			return 0; // send nothing
		case USBRQ_HID_GET_PROTOCOL:
			usbMsgPtr = &protocol_version; // send data starting from this byte
			return 1; // send 1 byte
		case USBRQ_HID_SET_PROTOCOL:
			protocol_version = rq->wValue.bytes[1];
			return 0; // send nothing
		case USBRQ_HID_GET_REPORT:
			usbMsgPtr = (uint8_t*)&keyboard_report; // send the report data
			report_pending = 0;
			keyboard_report_reset();
			// determine the return data length based on which report ID was requested
			if (rq->wValue.bytes[0] == 1) return 8;
			if (rq->wValue.bytes[0] == 2) return 3;
			if (rq->wValue.bytes[0] == 3) return 2;
			if (rq->wValue.bytes[0] == 4) return 4;
			return 8; // default
		case USBRQ_HID_SET_REPORT:
			if (rq->wLength.word == 1) // check data is available
			{
				// 1 byte, we don't check report type (it can only be output or feature)
				// we never implemented "feature" reports so it can't be feature
				// so assume "output" reports
				// this means set LED status
				// since it's the only one in the descriptor
				return USB_NO_MSG; // send nothing but call usbFunctionWrite
			}
			else // no data or do not understand data, ignore
			{
				return 0; // send nothing
			}
		default: // do not understand data, ignore
			return 0; // send nothing
	}
}

// see http://vusb.wikidot.com/driver-api
usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len)
{
	LED_state = data[0];
	return 1; // 1 byte read
}


int main()
{
	wdt_disable(); // disable watchdog, good habit if you don't use it
	
	#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
	uint8_t calibrationValue = eeprom_read_byte(OSCCAL_EEADDR); /* calibration value from last time */
	if (calibrationValue != 0xFF)
	{
		OSCCAL = calibrationValue;
	}
	#endif
	
	stdout = &mystdout; // set default stream
	
	TCCR0B = 0x05; // start timer0, used for measuring pulse widths
	TCCR1 = 0x0F; // start timer1, used for key release timeout

	// input with pull up
	JMP_DDRx  &= ~JMP_PINMASK;
	JMP_PORTx |=  JMP_PINMASK;

	// output, default off
	LED_DDRx  |=  LED_PINMASK;
	LED_PORTx &= ~LED_PINMASK;

	if (! buttonPressed())  tryProgram = 0;  // dont bother trying to check the programming button

	// initialize report (I never assume it's initialized to 0 automatically)
	keyboard_report_reset();
	
	// enforce USB re-enumeration by pretending to disconnect and reconnect
	usbDeviceDisconnect();
	LED_PORTx |=  LED_PINMASK; // LED on
	_delay_ms(250);

	// check button again
	if (! buttonPressed())  tryProgram = 0;  // dont bother trying to check the programming button

	LED_PORTx &= ~LED_PINMASK; // LED off
	usbDeviceConnect();
	// initialize various modules
	usbInit();

	#ifdef ENABLE_MMKEY_TRANSLATE
	if (eeprom_read_byte(MMKEY_TRANSLATE_EEADDR) != 0xFF) {
		// flash LED to indicate we are in MMKEY translate mode
		LED_PORTx &= ~LED_PINMASK; // LED off
		usb_polling_delay_ms(200);
		if (! buttonPressed())  tryProgram = 0;  // dont bother trying to check the programming button
		LED_PORTx |=  LED_PINMASK; // LED on
		usb_polling_delay_ms(200);
		LED_PORTx &= ~LED_PINMASK; // LED off
	}
	#endif

	sei(); // enable interrupts

	// if we started off with the jumper in place for 2 seconds, enter the user programming routine
	// wait for OS to get ready
	uint16_t toProg = 0;
	for (int i = 0; i < 2100 && tryProgram; )
	{
	  if (has_commed == 0) {
	    i = 0;
	  }
	  usbPoll();
	  if (buttonPressed())
	    {
	      if (toProg < 2000) {
		toProg++;
	      }
	      else {
		LED_PORTx |=  LED_PINMASK; // LED on
	      }
	    }
	  else
	    {
	      i++;
	      
	      // this is a debounce mechanism
	      if (toProg < 2000) {
		toProg = 0;
		break;
	      }
	    }
	  _delay_ms(1);
	}
	if (toProg >= 2000) {
	  while (buttonPressed()) 
	    usbPoll(); // wait for release
	  usr_prog();
	}
	LED_PORTx &= ~LED_PINMASK; // LED off

	while (1) // main loop, do forever
	{
		// perform usb related background tasks
		usbPollWrapper(); // this needs to be called at least once every 10 ms
		// this is also called in send_report_once

		ircap_res_t r = ir_cap(&ir_code);

		if (r == IRCAP_NEWKEY)
		{
			last_keycode = ir_to_kb(ir_code);
			#ifdef ENABLE_FULL_DEBUG
			printf_P(PSTR(" C: 0x%04X%04X K: 0x%04X%04X "),
			(unsigned int)((ir_code & 0xFFFF0000) >> 16), (unsigned int)(ir_code & 0xFFFF),
			(unsigned int)(last_keycode >> 16), (unsigned int)(last_keycode & 0xFFFF)); // split into 16 bit chunks due to suspected stdio bug
			#endif
			if (last_keycode != 0)
			{
				*((uint32_t*)&keyboard_report) = last_keycode;
				report_pending = 1;
				LED_PORTx |= LED_PINMASK; // LED on
			}
			else
			{
				#ifdef ENABLE_UNKNOWN_DEBUG
				// if we get a code that is known, type it out to the screen so the user can see it and maybe reprogram the command table with it later
				printf_P(PSTR(" UK: 0x%04X%04X %d "), (unsigned int)((ir_code & 0xFFFF0000) >> 16), (unsigned int)(ir_code & 0xFFFF), bit_idx); // split into 16 bit chunks due to suspected stdio bug
				#endif
			}
			#ifdef ENABLE_TIMEBUFF_DEBUG
			for (int i = 0; i < time_buff_idx; i++) {
				printf_P(PSTR(" %d "), time_buff[i]); // debugs timings
			}
			#endif
		}
		else if (r == IRCAP_REPEATKEY)
		{
			if (last_keycode > 0
			&& last_keycode != KEYCODE_MUTE && last_keycode != KEYCODE_PLAYPAUSE // do not repeat these keys
			)
			{
				*((uint32_t*)&keyboard_report) = last_keycode;
				report_pending = 2;
			}
		}

		if (r != IRCAP_NOTHING)
		{
			TCNT1 = 0;
			if (bit_is_set(TIFR, TOV1)) TIFR |= _BV(TOV1);
		}

		if (TCNT1 >= TMR1_TIMEOUT_120MS || bit_is_set(TIFR, TOV1) || r == IRCAP_ERROR)
		{
			last_keycode = 0; // too long for repeat signal, invalidate this to reject noise

			if (report_pending != 4)
			{
				keyboard_report_reset();
				report_pending = 3;
				// this will send a blank keystroke only once
			}

			// prevent the timer from actually overflowing but also keeps the timed-out state
			TCNT1 = TMR1_TIMEOUT_120MS;
			if (bit_is_set(TIFR, TOV1)) TIFR |= _BV(TOV1);

			LED_PORTx &= ~LED_PINMASK; // LED off
		}

		#ifdef ENABLE_MMKEY_TRANSLATE
		// if button is pressed
		if (bit_is_clear(JMP_PINx, JMP_PINNUM) && toProg <= 1000)
		{
			toProg++;
			_delay_ms(1);
			// this delay can cause IR measurement to be wrong
			// but we don't care, who's going to send IR and press the button at the same time?
		}
		else // released or been held long enough
		{
			if (toProg > 1000)
			{
				// toggle between MMKEY translate modes
				if (eeprom_read_byte(MMKEY_TRANSLATE_EEADDR) == 0xFF) {
					eeprom_write_byte(MMKEY_TRANSLATE_EEADDR, 0x00);
					// flash LED to indicate new mode
					LED_PORTx &= ~LED_PINMASK; // LED off
					usb_polling_delay_ms(200);
					LED_PORTx |=  LED_PINMASK; // LED on
					usb_polling_delay_ms(200);
					LED_PORTx &= ~LED_PINMASK; // LED off
					usb_polling_delay_ms(200);
					LED_PORTx |=  LED_PINMASK; // LED on
					usb_polling_delay_ms(200);
					LED_PORTx &= ~LED_PINMASK; // LED off
				}
				else {
					eeprom_write_byte(MMKEY_TRANSLATE_EEADDR, 0xFF);
					// flash LED to indicate new mode
					LED_PORTx &= ~LED_PINMASK; // LED off
					usb_polling_delay_ms(200);
					LED_PORTx |=  LED_PINMASK; // LED on
					usb_polling_delay_ms(200);
					LED_PORTx &= ~LED_PINMASK; // LED off
				}
				while (bit_is_clear(JMP_PINx, JMP_PINNUM)) usbPoll(); // wait for release
			}

			toProg = 0; // if released, then forget it
			// also helps debounce
		}
		#endif
	}

	return 0;
}

ircap_res_t ir_cap(uint32_t* ir_code_ptr)
{
	ircap_res_t res = IRCAP_NOTHING;

	// wait for pulse
	if (bit_is_clear(IN_PINx, IN_PINNUM))
	{
		TCNT0 = 0; // new pulse measurement
		if (bit_is_set(TIFR, TOV0)) TIFR |= _BV(TOV0); // clear the overflow flag if it is set
		
		while (bit_is_clear(IN_PINx, IN_PINNUM))
		{
			// perform usb related background tasks
			usbPollWrapper(); // this needs to be called at least once every 10 ms
		}

		uint8_t tmpTCNT0 = TCNT0;	// quickly take a snapshot of the stopwatch before...
		TCNT0 = 0;					// resetting the stopwatch

		res = IRCAP_BUSY;

		if (bit_is_set(TIFR, TOV0))
		{
			// timer overflow, clear the flag
			TIFR |= _BV(TOV0);
			tmpTCNT0 = 255; // fake long pulse for next if-statement
		}

		if (tmpTCNT0 >= PULSEWIDTH_INITIAL_9MS && tmpTCNT0 <= (PULSEWIDTH_INITIAL_9MS + PULSEWIDTH_2MS))
		{
			bit_idx = -2; // reset bit index since it is the initial pulse
			#ifdef ENABLE_TIMEBUFF_DEBUG
			time_buff_idx = 0;
			#endif
		}
		else if (tmpTCNT0 >= PULSEWIDTH_ON_MIN && tmpTCNT0 <= (PULSEWIDTH_INITIAL_9MS / 8))
		{
			bit_idx++; // this becomes 0 if it was -1 before (when the off pulse is valid)
		}
		else
		{
			#ifdef ENABLE_UNKNOWN_DEBUG
			printf_P(PSTR(" e1 %d %d "), bit_idx, tmpTCNT0);
			#endif
			bit_idx = INDICATE_ERROR;
		}

		#ifdef ENABLE_TIMEBUFF_DEBUG
		time_buff[time_buff_idx++] = tmpTCNT0;
		#endif

		while (bit_is_set(IN_PINx, IN_PINNUM) && bit_is_clear(TIFR, TOV0) && TCNT0 < (PULSEWIDTH_5MS + PULSEWIDTH_2MS))
		{
			// perform usb related background tasks
			usbPollWrapper(); // this needs to be called at least once every 10 ms
		}

		tmpTCNT0 = TCNT0; // quickly take a snapshot of the stopwatch

		#ifdef ENABLE_TIMEBUFF_DEBUG
		time_buff[time_buff_idx++] = tmpTCNT0;
		#endif

		if (bit_is_set(TIFR, TOV0))
		{
			// timer overflow, clear the flag
			TIFR |= _BV(TOV0);
			tmpTCNT0 = 255; // fake long pulse for next if-statement
		}

		if (tmpTCNT0 < PULSEWIDTH_3MS && bit_idx >= 0)
		{
			// determine whether 1 or 0
			if (tmpTCNT0 < PULSEWIDTH_BIT_THRESHOLD) {
				(*ir_code_ptr) &= ~(1UL << bit_idx);
			}
			else {
				(*ir_code_ptr) |=  (1UL << bit_idx);
			}
		}
		else if (tmpTCNT0 >= PULSEWIDTH_3MS && bit_idx > 0)
		{
			// new command
			res = IRCAP_NEWKEY;
		}
		else if (bit_idx == -2 && tmpTCNT0 >= PULSEWIDTH_2MS && tmpTCNT0 <= PULSEWIDTH_3MS)
		{
			bit_idx = -5; // signal that repeat key is possible
		}
		else if (tmpTCNT0 >= PULSEWIDTH_3MS && bit_idx == -4)
		{
			// repeated command
			res = IRCAP_REPEATKEY;
			bit_idx++;
		}
		else if (bit_idx == -2 && tmpTCNT0 >= PULSEWIDTH_4MS && tmpTCNT0 <= PULSEWIDTH_5MS)
		{
			// this is the 4.5ms off time after the 9ms on time
			bit_idx = -1;
			last_keycode = 0;
		}
		else
		{
			#ifdef ENABLE_UNKNOWN_DEBUG
			printf_P(PSTR(" e2 %d %d "), bit_idx, tmpTCNT0);
			#endif
			bit_idx = INDICATE_ERROR;
		}
	}

	if (bit_idx <= INDICATE_ERROR) {
		last_keycode = 0;
		return IRCAP_ERROR;
	}

	return res;
}

uint32_t mmkey_translate(uint32_t kc) {
#ifdef ENABLE_MMKEY_TRANSLATE
  if (eeprom_read_byte(MMKEY_TRANSLATE_EEADDR) != 0xFF) {
    switch (kc)
      {
      case KEYCODE_EQUAL:
      case KEYCODE_ARROW_UP:
	return KEYCODE_VOL_UP;
      case KEYCODE_MINUS:
      case KEYCODE_ARROW_DOWN:
	return KEYCODE_VOL_DOWN;
      case KEYCODE_ARROW_RIGHT:
	return KEYCODE_SCAN_NEXT_TRACK;
      case KEYCODE_ARROW_LEFT:
	return KEYCODE_SCAN_PREV_TRACK;
      case KEYCODE_X:
	return KEYCODE_STOP;
      case KEYCODE_SPACE:
	return KEYCODE_PLAYPAUSE;
      case KEYCODE_ESC:
	return KEYCODE_KB_MENU;
	/*
	  case KEYCODE_APP:
	  return XBMC_CONTEXTUALMENU;
	  case KEYCODE_SYS_POWER:
	  case KEYCODE_SYS_SLEEP:
	  return XBMC_SHUTDOWNMENU;
	  case KEYCODE_MUTE:
	  return kc;
	*/
      }
  }
#endif

  return kc;
}

// this function does a search of the IR-button command pair table for the IR code, returning the corresponding keycode
// or 0 if not found or error
uint32_t ir_to_kb(uint32_t ircode)
{
	uint32_t r = usr_ir_to_kb(ircode);


	#ifdef ENABLE_DEFAULT_CODES
	if (r == 0)
	{
		// search through table of IR-button command pairs
		for (int i = 0; ; i++)
		{
			usbPoll();
			uint32_t tblVal = pgm_read_dword(&((uint32_t*)ir_but_tbl)[i * 2]);
			if (tblVal == 0 || tblVal == 0xFFFFFFFF) {
				// null termination found or flash is empty
				break;
			}
			if (tblVal == ircode)
			{
			  return mmkey_translate(pgm_read_dword(&((uint32_t*)ir_but_tbl)[i * 2 + 1])); // found, return the key
			}
		}
	}
	#endif

	#ifdef ENABLE_APPLE_DEFAULTS
	if (r == 0)
	{
		r = apple_code_check(ircode);
	}
	#endif


	r = mmkey_translate(r);

	return r; // not found
}

// a wrapper for usbPoll, which must be called often
// sends new report when needed
void usbPollWrapper()
{
	if (usbInterruptIsReady() && report_pending != 0 && report_pending != 4)
	{
		uint8_t s = 8;
		if (keyboard_report.report_id == 2) s = 3;
		if (keyboard_report.report_id == 3) s = 2;
		if (keyboard_report.report_id == 4) s = 4;
		usbSetInterrupt((uint8_t*)(&keyboard_report), s);

		if (report_pending == 3) {
			// only once
			report_pending = 4;
		}
		else {
			report_pending = 0;
		}
	}

	usbPoll();
}

// translates ASCII to appropriate keyboard report, taking into consideration the status of caps lock
void ASCII_to_keycode(uint8_t ascii)
{
	keyboard_report.report_id = 1;
	keyboard_report.keycode[0] = 0x00;
	keyboard_report.modifier = 0x00;
	
	// see scancode.doc appendix C
	
	if (ascii >= 'A' && ascii <= 'Z')
	{
		keyboard_report.keycode[0] = 4 + ascii - 'A'; // set letter
		if (bit_is_set(LED_state, 1)) // if caps is on
		{
			keyboard_report.modifier = 0x00; // no shift
		}
		else
		{
			keyboard_report.modifier = _BV(1); // hold shift // hold shift
		}
	}
	else if (ascii >= 'a' && ascii <= 'z')
	{
		keyboard_report.keycode[0] = 4 + ascii - 'a'; // set letter
		if (bit_is_set(LED_state, 1)) // if caps is on
		{
			keyboard_report.modifier = _BV(1); // hold shift // hold shift
		}
		else
		{
			keyboard_report.modifier = 0x00; // no shift
		}
	}
	else if (ascii >= '0' && ascii <= '9')
	{
		keyboard_report.modifier = 0x00;
		if (ascii == '0')
		{
			keyboard_report.keycode[0] = 0x27;
		}
		else
		{
			keyboard_report.keycode[0] = 30 + ascii - '1'; 
		}
	}
	else
	{
		switch (ascii) // convert ascii to keycode according to documentation
		{
			case '!':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 1;
				break;
			case '@':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 2;
				break;
			case '#':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 3;
				break;
			case '$':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 4;
				break;
			case '%':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 5;
				break;
			case '^':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 6;
				break;
			case '&':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 7;
				break;
			case '*':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 8;
				break;
			case '(':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 29 + 9;
				break;
			case ')':
				keyboard_report.modifier = _BV(1); // hold shift
				keyboard_report.keycode[0] = 0x27;
				break;
			case '~':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '`':
				keyboard_report.keycode[0] = 0x35;
				break;
			case '_':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '-':
				keyboard_report.keycode[0] = 0x2D;
				break;
			case '+':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '=':
				keyboard_report.keycode[0] = 0x2E;
				break;
			case '{':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '[':
				keyboard_report.keycode[0] = 0x2F;
				break;
			case '}':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case ']':
				keyboard_report.keycode[0] = 0x30;
				break;
			case '|':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '\\':
				keyboard_report.keycode[0] = 0x31;
				break;
			case ':':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case ';':
				keyboard_report.keycode[0] = 0x33;
				break;
			case '"':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '\'':
				keyboard_report.keycode[0] = 0x34;
				break;
			case '<':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case ',':
				keyboard_report.keycode[0] = 0x36;
				break;
			case '>':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '.':
				keyboard_report.keycode[0] = 0x37;
				break;
			case '?':
				keyboard_report.modifier = _BV(1); // hold shift
				// fall through
			case '/':
				keyboard_report.keycode[0] = 0x38;
				break;
			case ' ':
				keyboard_report.keycode[0] = 0x2C;
				break;
			case '\t':
				keyboard_report.keycode[0] = 0x2B;
				break;
			case '\n':
				keyboard_report.keycode[0] = 0x28;
				break;
		}
	}
}

void send_report_once()
{
	// perform usb background tasks until the report can be sent, then send it
	while (1)
	{
		usbPoll(); // this needs to be called at least once every 10 ms
		if (usbInterruptIsReady())
		{
			usbSetInterrupt((uint8_t*)(&keyboard_report), sizeof(keyboard_report)); // send
			break;

			// see http://vusb.wikidot.com/driver-api
		}
	}
}

// stdio's stream will use this funct to type out characters in a string
void type_out_char(uint8_t ascii, FILE * stream)
{
	keyboard_report_reset(); // release keys
	send_report_once();
	ASCII_to_keycode(ascii);
	send_report_once();
	keyboard_report_reset(); // release keys
	send_report_once();
}

ISR(BADISR_vect)
{
}

#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */
// section copied from EasyLogger
/* Calibrate the RC oscillator to 8.25 MHz. The core clock of 16.5 MHz is
 * derived from the 66 MHz peripheral clock by dividing. Our timing reference
 * is the Start Of Frame signal (a single SE0 bit) available immediately after
 * a USB RESET. We first do a binary search for the OSCCAL value and then
 * optimize this value with a neighboorhod search.
 * This algorithm may also be used to calibrate the RC oscillator directly to
 * 12 MHz (no PLL involved, can therefore be used on almost ALL AVRs), but this
 * is wide outside the spec for the OSCCAL value and the required precision for
 * the 12 MHz clock! Use the RC oscillator calibrated to 12 MHz for
 * experimental purposes only!
 */
static void calibrateOscillator(void)
{
	uchar       step = 128;
	uchar       trialValue = 0, optimumValue;
	int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

	/* do a binary search: */
	do{
		OSCCAL = trialValue + step;
		x = usbMeasureFrameLength();    /* proportional to current real frequency */
		if(x < targetValue)             /* frequency still too low */
			trialValue += step;
		step >>= 1;
	}while(step > 0);
	/* We have a precision of +/- 1 for optimum OSCCAL here */
	/* now do a neighborhood search for optimum value */
	optimumValue = trialValue;
	optimumDev = x; /* this is certainly far away from optimum */
	for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
		x = usbMeasureFrameLength() - targetValue;
		if(x < 0)
			x = -x;
		if(x < optimumDev){
			optimumDev = x;
			optimumValue = OSCCAL;
		}
	}
	OSCCAL = optimumValue;
}
/*
Note: This calibration algorithm may try OSCCAL values of up to 192 even if
the optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
You may replace this search algorithm with any other algorithm you like if
you have additional constraints such as a maximum CPU clock.
For version 5.x RC oscillators (those with a split range of 2x128 steps, e.g.
ATTiny25, ATTiny45, ATTiny85), it may be useful to search for the optimum in
both regions.
*/

void usbEventResetReady(void)
{
	calibrateOscillator();
	eeprom_update_byte(OSCCAL_EEADDR, OSCCAL);   /* store the calibrated value in EEPROM */
}
#endif
