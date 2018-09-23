//
// Copyright 2018, Jim Studt jim@studt.net
// SPDX-License-Identifier: BSD-3-Clause
//

#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

// Maybe you want to pass this in on the compile command?
// NOTE: This is the Broadcom pin number, not the Wiring pin number
#ifndef DHTPIN
#define DHTPIN 4
#endif

static unsigned cleanupPin = UINT_MAX;
static bool verbose = false;

int read_dht11( unsigned pin) {
    // set pin to output, we keep it at LOW all the time and use the pullup.
    gpioSetMode( pin, PI_OUTPUT);

    // send a low pulse, > 18ms according to datasheet
    gpioDelay( 19*1000);  // may well be longer, we probably get pre-empted.
    
    // turn pin back to input
    gpioSetMode( pin, PI_INPUT);

    return 0;
}



static void cleanup(void) {
    // This gets registered with atexit() to run at program termination.
    // Be aware that there are signals which will kill us too dead to run
    // this function, so you might leave a pin turned on.
    if ( verbose) fprintf(stderr,"... cleanup()\n");
    
    if ( cleanupPin != UINT_MAX) gpioSetPullUpDown( cleanupPin, PI_PUD_OFF);
    gpioTerminate();
}

enum pulse_state { PS_IDLE = 0, PS_PREAMBLE_STARTED, PS_DIGITS };

static void pulse_reader( int gpio, int level, uint32_t tick) {

    // *******
    // PAY ATTENTION: This runs in a different thread from your main thread.
    //
    // Do not interact with things owned by the main thread without understanding
    // locks and threaded programming.
    // *******

    // ******
    // Be Aware: the tick is extremly erratic. Notice that for the 80µS pulses
    //           I am testing 70 < tick < 95. The spread is really that wide.
    //           If you run with 'verbose' on you can see the pulse lengths and
    //           see for yourself. I feel this is some problem in the Pi and the
    //           library rather than the device, but I haven't pulled out a scope
    //           to check.
    // ******
    
    static uint32_t lastTick = 0;
    static enum pulse_state state = PS_IDLE;
    static uint64_t accum = 0;
    static int count = 0;
    uint32_t len = tick - lastTick; // not handling rollover, you will get a bad read on that one
    lastTick = tick;

    switch ( state) {
      case PS_IDLE:
	// In this state we haven't made sense out of anything we have seen.
	// An 80µS low pulse could be the start of a preamble.
	if (level == 1 && len > 70 && len < 95) state = PS_PREAMBLE_STARTED;
	else state = PS_IDLE;
	break;
      case PS_PREAMBLE_STARTED:
	// An 80µS high pulse completes the preamble. Anything else and we are back to idle.
	if (level == 0 && len > 70 && len < 95) {
	    state = PS_DIGITS;
	    accum = 0;
	    count= 0;
	} else state = PS_IDLE;
	break;
      case PS_DIGITS:
	// As long as we receive digits, accumulate them into our 64 bit number
	if (level == 1 && len >= 35 && len <= 65) ;  // ok, it is the low before the content
	else if (level == 0 && len >= 15 && len <= 35) { accum <<= 1; count++; }
	else if (level == 0 && len >= 60 && len <= 80) { accum = (accum<<1)+1; count++; }
	else state = PS_IDLE;  // not a valid bit, back to idle.

	// When we get our 40th bit we can process the data
	if ( count == 40) {
	    state = PS_IDLE;  // done with bits, going back to idle

	    uint8_t parity = (accum & 0xff);
	    uint8_t tempLow = ((accum>>8) & 0xff);
	    uint8_t tempHigh = ((accum>>16) & 0xff);
	    uint8_t humLow = ((accum>>24) & 0xff);
	    uint8_t humHigh = ((accum>>32) & 0xff);

	    uint8_t sum = tempLow + tempHigh + humLow + humHigh;
	    bool valid = (parity == sum);
	    
	    printf("got 40 digits %d.%d hum, %d.%d temp, %s\n", humHigh, humLow, tempHigh, tempLow, (valid ? "OK" : "BAD"));
	}

	break;
    }

	
    if ( verbose) printf("pulse %c %4uµS state=%d digits=%d\n", (level==0 ? 'H' : (level==1?'L':'W') ), len, state, count);
}

int main( int argc, char **argv) {
    unsigned pin = DHTPIN;
    
    // maybe set 'pin' from a command line arg here

    // Get the GPIO ready
    if (gpioInitialise() == PI_INIT_FAILED) {
	fprintf(stderr, "failed to initialize GPIO\n");
	exit(EXIT_FAILURE);
    }

    // Make sure we turn off gpio on our way out
    atexit(cleanup);
    
    // set pin to have a pullup resistor
    gpioSetMode( pin, PI_INPUT);
    gpioSetPullUpDown( pin, PI_PUD_UP);
    gpioWrite( pin, 0);           // We keep it set to LOW so anytime we make it an output it pulls down
    cleanupPin = pin;      // hang on to this so cleanup() can find it
    
    // set the watchdog to 50ms to cap long pulses
    gpioSetWatchdog(pin, 50);

    gpioSetAlertFunc( pin, pulse_reader);
    
    // Repeat until tired
    for (int i = 0; i < 50; i++) {

	// This just initiates the reading and returns, it will take around 20ms.
	// The actual reading takes place in the pulse_reader alert function in a
	// different thread and will complete in the future. Good luck with that.
	read_dht11(pin);

	gpioDelay( 100*1000); // wait 1/10 sec between tries
    }

    return 0;
}

