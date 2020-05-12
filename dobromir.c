#include <string.h>

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv/usbdrv.h"
#include "usbdrv/oddebug.h"        /* This is also an example for using debug macros */

#define BOOL uint8_t
#define TRUE 1
#define FALSE 0

static uint8_t report[4] = {0};
static uint8_t report_out[4] = {0};

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor [USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
	0x05, 0x01,     // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,     // USAGE (Game pad)
	0xa1, 0x01,     // COLLECTION (Application)
//	0x85, 0x01,     //   REPORT_ID (1)
	0x09, 0x01,     //   USAGE (Pointer)
	0xa1, 0x00,     //   COLLECTION (Physical)
	0x09, 0x30,     //     USAGE (X)
	0x09, 0x31,     //     USAGE (Y)
	0x15, 0x81,     //     LOGICAL_MINIMUM (-127)
	0x25, 0x7f,     //     LOGICAL_MAXIMUM (127)
	0x75, 0x08,     //     REPORT_SIZE (8)
	0x95, 0x02,     //     REPORT_COUNT (2)
	0x81, 0x02,     //     INPUT (Data,Var,Abs)
	0xc0,           //   END_COLLECTION

	0x09, 0x32,     //   USAGE (Z)
	0x15, 0x00,     //   LOGICAL_MINIMUM (0)
	0x25, 0xff,     //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,     //   REPORT_SIZE (8)
	0x95, 0x01,     //   REPORT_COUNT (1)
	0x81, 0x02,     //   INPUT (Data,Var,Abs)


	0x05, 0x09,     //   USAGE_PAGE (Button)
	0x19, 0x01,     //   USAGE_MINIMUM (Button 1)
	0x29, 0x08,     //   USAGE_MAXIMUM (Button 8)
	0x15, 0x00,     //   LOGICAL_MINIMUM (0)
	0x25, 0x01,     //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,     //   REPORT_SIZE (1)
	0x95, 0x08,     //   REPORT_COUNT (8)
	0x81, 0x02,     //   INPUT (Data,Var,Abs)
	
	0x09, 0x21,     //   USAGE (Output Report Data)
	0x75, 0x08,     //   REPORT SIZE (8)
	0x95, 0x14,     //   REPORT COUNT (20)
	0x91, 0x02,     //   OUTPUT (Data,Var,Abs)
	
	0xc0            // END_COLLECTION	
};

volatile uint8_t tmp = 1;

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t const* rq = (usbRequest_t const*) data;

	if ( (rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS )
		return 0;
	
	switch ( rq->bRequest )
	{
	case USBRQ_HID_GET_REPORT: // HID joystick only has to handle this
		usbMsgPtr = (usbMsgPtr_t) report_out;
		return sizeof report_out;
//		return USB_NO_MSG;
	
	case USBRQ_HID_SET_REPORT: // LEDs on joystick?
//	case 99:
		tmp = !tmp;
		// return 1;
		return USB_NO_MSG;
	
	default:
		return 0;
	}
}

uchar usbFunctionWrite(uchar* data, uchar len)
{
	return 1;
}

#define SHIFT_CLOCK PC5
#define SHIFT_OUTPUT PC4
#define SHIFT_LOAD PC3
#define BUTTON PC2
#define BUTTON_LED PC1
#define POWER_LED PB0

void setupInputs(void)
{
	DDRC = 0; // all inputs
	DDRC |= _BV(SHIFT_CLOCK);
	DDRC |= _BV(SHIFT_LOAD);
	DDRC |= _BV(BUTTON_LED);
	DDRB |= _BV(POWER_LED);
	
	// PORTC |= _BV(BUTTON);
	//PORTC = 0;
	PORTC = 0xFF;
	PORTC &= ~_BV(SHIFT_CLOCK);
	PORTC ^= _BV(BUTTON_LED);
	
	PORTB |= _BV(POWER_LED);
}

inline BOOL isInputEnabled(uint8_t reg, uint8_t inputNb)
{
	return !(reg & _BV(inputNb));
}

inline uint8_t getInputVal(uint8_t ddr, uint8_t pin)
{
	//1-inactive, 0-active, then filter by inputs
	return (~pin) & (~ddr);
}

static void readInputs(void)
{
//	report[0] = 1;
	uint8_t prevButtons = report[3];

	// joystick
	report[0] = 128;
	report[1] = 66;

	// axis z
//	report[2] = 100;
	
	// buttons
	report[3] = 0;
	
	PORTC &= ~_BV(SHIFT_LOAD);
	PORTC |= _BV(SHIFT_LOAD);
		
	for (int i = 0; i <= 7; i++)
	{
		PORTC |= _BV(SHIFT_CLOCK);
		if (PINC & _BV(SHIFT_OUTPUT))
			report[3] |= 1 << i;
			
		PORTC &= ~_BV(SHIFT_CLOCK);
	}

	if (report[3] != prevButtons)
	{
		report[2] += 20;
	}

	//report[2] = getInputVal(DDRC, PINC);
	/*
	if (isInputEnabled(PINC, BUTTON))
	{
		report[2] = 0xff;
	}
	else
	{
		report[2] = 0;
	}
	*/
	//report[2] = 0xff;
}

/* ------------------------------------------------------------------------- */

int main(void)
{
//uchar   i;

    // wdt_enable(WDTO_1S);
	
	setupInputs();

	
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    //odDebugInit();
    //DBG1(0x00, 0, 0);       /* debug output: main starts */
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    int i = 500;
    while(--i){             /* fake USB disconnect for > 250 ms */
      //  wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    //LED_PORT_DDR |= _BV(LED_BIT);   /* make the LED bit an output */
    sei();
    //DBG1(0x01, 0, 0);       /* debug output: main loop starts */
	
    for(;;)
	{                /* main event loop */
		usbPoll();
		// if (isInputEnabled(PINC, BUTTON))
		// if (getInputVal(DDRC, PINC) & _BV(BUTTON))
		if (PINC & _BV(BUTTON))
		{
			PORTC |= _BV(BUTTON_LED);
		}
		else // if (PINC & _BV(BUTTON_LED))
		{
			PORTC &= ~_BV(BUTTON_LED);
		}
		
		if (usbInterruptIsReady())
		{

		//	PORTB ^= _BV(POWER_LED);
			readInputs();
			if (memcmp(report_out, report, sizeof report ) != 0)
			// if (1)
			{
				memcpy( report_out, report, sizeof report );
				usbSetInterrupt( report_out, sizeof report_out );
				if (tmp)
					PORTB ^= _BV(POWER_LED);
			}
		}
        // wdt_reset();
		_delay_ms(5);
        
    }
	
	return 0;
}

/* ------------------------------------------------------------------------- */
