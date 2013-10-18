#include "main.h"
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdio.h>

// sorry, i can only place string tables in flash by declaring them seperately
const PROGMEM char descstr_menu[]			 = "escape";
const PROGMEM char descstr_select[]		 = "enter";
const PROGMEM char descstr_playpause[]		 = "space";
const PROGMEM char descstr_volup[]			 = "up arrow";
const PROGMEM char descstr_voldn[]			 = "down arrow";
const PROGMEM char descstr_next[]			 = "right arrow";
const PROGMEM char descstr_prev[]			 = "left arrow";
/*
const PROGMEM char descstr_stop[]			 = "stop";
const PROGMEM char descstr_mute[]			 = "mute";
const PROGMEM char descstr_power[]			 = "power";
const PROGMEM char descstr_rec[]			 = "rec";
const PROGMEM char descstr_epg[]			 = "epg";
const PROGMEM char descstr_livetv[]		 = "live tv";
const PROGMEM char descstr_stereo[]		 = "stereo";
const PROGMEM char descstr_snapshot[]		 = "snapshot";
const PROGMEM char descstr_zoom[]			 = "zoom";
const PROGMEM char descstr_recall[]		 = "recall";
const PROGMEM char descstr_teletext[]		 = "teletext";
const PROGMEM char descstr_source[]		 = "source";
const PROGMEM char descstr_fav[]			 = "favorite";
//*/

#include "kbrd_codes.h"
// pair up the action and the description here
const PROGMEM code_desc_t code_desc_tbl[] = { // remember to change the count!
	{ .c = KEYCODE_ARROW_UP ,			.s = descstr_volup },
	{ .c = KEYCODE_ARROW_DOWN ,		.s = descstr_voldn },
	{ .c = KEYCODE_ARROW_RIGHT ,	.s = descstr_next },
	{ .c = KEYCODE_ARROW_LEFT ,		.s = descstr_prev },
	{ .c = KEYCODE_SPACE ,		.s = descstr_playpause },
	{ .c = KEYCODE_ENTER ,			.s = descstr_select },
	{ .c = KEYCODE_ESC ,			.s = descstr_menu },
	/*
	{ .c = KEYCODE_STOP ,			.s = descstr_stop },
	{ .c = KEYCODE_SYS_SLEEP ,		.s = descstr_power },
	{ .c = KEYCODE_R ,				.s = descstr_rec },
	{ .c = KEYCODE_BACKSPACE ,		.s = descstr_recall },
	{ .c = XMBC_EPGTVGUIDE ,		.s = descstr_epg },
	{ .c = KEYCODE_L ,				.s = descstr_livetv },
	{ .c = KEYCODE_F2 ,				.s = descstr_stereo },
	{ .c = KEYCODE_PRINTSCREEN ,	.s = descstr_snapshot },
	{ .c = KEYCODE_Z ,				.s = descstr_zoom },
	{ .c = XMBC_NEXTSUBTITLE ,		.s = descstr_teletext },
	{ .c = KEYCODE_S ,				.s = descstr_source },
	{ .c = KEYCODE_F ,				.s = descstr_fav },
	//*/

	{ .c = 0 , .s = 0 }, // null terminated to signal end of table
};

static char desc_buff[32];

void usr_prog()
{
	printf_P(PSTR("\nWelcome to IR Keyboard Programming Mode\n"));

	for (int i = 0; ; i++)
	{
		code_desc_t cd;
		memcpy_PF((void*)&cd, (uint_farptr_t)&(code_desc_tbl[i]), sizeof(code_desc_t));
		if (cd.c == 0 || cd.c == 0xFFFFFFFF) {
			// null termination found or flash memory empty
			break;
		}
		strcpy_PF((void*)desc_buff, (uint_farptr_t)cd.s);
		printf_P(PSTR("Press \"%s\""), desc_buff); // prompt the user
		static uint32_t ir_code;
		TCNT1 = 0;
		if (bit_is_set(TIFR, TOV1)) TIFR |= _BV(TOV1);
		uint8_t tmr1_ovf_cnt = 0;
		while (1)
		{
			usbPollWrapper();

			ircap_res_t r = ir_cap(&ir_code);

			if (r != IRCAP_NOTHING) {
				tmr1_ovf_cnt = 0;
			}

			if (bit_is_set(TIFR, TOV1)) {
				TIFR |= _BV(TOV1);
				tmr1_ovf_cnt++;
			}

			if (r == IRCAP_NEWKEY) {
				eeprom_update_dword((uint32_t*)(i * sizeof(uint32_t)), ir_code);
				#ifdef ENABLE_PROG_DEBUG
				printf_P(PSTR(" [Read 0x%04X%04X]"),
					(unsigned int)((ir_code & 0xFFFF0000) >> 16), 
					(unsigned int)(ir_code & 0xFFFF));
				// split into 16 bit chunks due to suspected stdio bug
				#endif

				printf_P(PSTR(" OK!\n"));
				break;
			}
			else if (tmr1_ovf_cnt >= TMR1_TIMEOUT_5S) { // took too long
				uint32_t ic = eeprom_read_dword((uint32_t*)(i * sizeof(uint32_t)));
				if (ic == 0 && ic == 0xFFFFFFFF) {
					eeprom_update_dword((uint32_t*)(i * sizeof(uint32_t)), 0x01); // insert empty placeholder that is neither null nor empty
				}
				printf_P(PSTR(", nevermind\n"));
				break;
			}
		}
	}

	printf_P(PSTR("All Done!\n"));
}

uint32_t usr_ir_to_kb(uint32_t ir)
{
	for (int i = 0; ; i++)
	{
		usbPollWrapper();
		uint32_t ic = eeprom_read_dword((uint32_t*)(i * sizeof(uint32_t)));
		if (ic == 0 || ic == 0xFFFFFFFF) {
			// null termination found or empty
			break;
		}
		if (ic == ir) {
			code_desc_t cd;
			memcpy_PF((void*)&cd, (uint_farptr_t)&(code_desc_tbl[i]), sizeof(code_desc_t));
			return cd.c;
		}
	}

	return 0;
}
