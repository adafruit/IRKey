#include <stdint.h>
#include <nec_defaults.h>

#define APPLECODE_ID 0x87EE // this was discovered by experimentation

static uint32_t apple_code_check(uint32_t ircode)
{
	if ((ircode & 0xFFFF) != APPLECODE_ID) // doesn't match means not Apple
	{
		return 0;
	}

	//ircode &= 0x00FF0000;
	ircode >>= 16; // just makes the switch case more efficient
	uint8_t c = ircode;
	switch (c)
	{
		// discovered by experimentation
		case 0x0A:
			return BUT_APPLE_UP;
		case 0x0C:
			return BUT_APPLE_DOWN;
		case 0x09:
			return BUT_APPLE_LEFT;
		case 0x06:
			return BUT_APPLE_RIGHT;
		case 0x5F:
			return BUT_APPLE_PLAYPAUSE;
		case 0x03:
			return BUT_APPLE_MENU;
		case 0x5C:
			return BUT_APPLE_SELECT;
		default:
			return 0;
	}
}