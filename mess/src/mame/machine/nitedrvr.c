/***************************************************************************

    Atari Night Driver hardware

***************************************************************************/

#include "emu.h"
#include "includes/nitedrvr.h"
#include "sound/discrete.h"

/***************************************************************************
Steering

When D7 is high, the steering wheel has moved.
If D6 is low, it moved left.  If D6 is high, it moved right.
Be sure to keep returning a direction until steering_reset is called,
because D6 and D7 are apparently checked at different times, and a
change in-between can affect the direction you move.
***************************************************************************/

static int nitedrvr_steering( running_machine &machine )
{
	nitedrvr_state *state = machine.driver_data<nitedrvr_state>();
	int this_val = input_port_read(machine, "STEER");
	int delta = this_val - state->m_last_steering_val;

	state->m_last_steering_val = this_val;

	if (delta > 128)
		delta -= 256;
	else if (delta < -128)
		delta += 256;

	/* Divide by four to make our steering less sensitive */
	state->m_steering_buf += (delta / 4);

	if (state->m_steering_buf > 0)
	{
		state->m_steering_buf--;
		state->m_steering_val = 0xc0;
	}
	else if (state->m_steering_buf < 0)
	{
		state->m_steering_buf++;
		state->m_steering_val = 0x80;
	}
	else
	{
		state->m_steering_val = 0x00;
	}

	return state->m_steering_val;
}

/***************************************************************************
nitedrvr_steering_reset
***************************************************************************/

READ8_HANDLER( nitedrvr_steering_reset_r )
{
	nitedrvr_state *state = space->machine().driver_data<nitedrvr_state>();
	state->m_steering_val = 0;
	return 0;
}

WRITE8_HANDLER( nitedrvr_steering_reset_w )
{
	nitedrvr_state *state = space->machine().driver_data<nitedrvr_state>();
	state->m_steering_val = 0;
}


/***************************************************************************
nitedrvr_in0_r

Night Driver looks for the following:
    A: $00
        D4 - OPT1
        D5 - OPT2
        D6 - OPT3
        D7 - OPT4
    A: $01
        D4 - TRACK SET
        D5 - BONUS TIME ALLOWED
        D6 - VBLANK
        D7 - !TEST
    A: $02
        D4 - !GEAR 1
        D5 - !GEAR 2
        D6 - !GEAR 3
        D7 - SPARE
    A: $03
        D4 - SPARE
        D5 - DIFFICULT BONUS
        D6 - STEER A
        D7 - STEER B

Fill in the steering and gear bits in a special way.
***************************************************************************/

READ8_HANDLER( nitedrvr_in0_r )
{
	nitedrvr_state *state = space->machine().driver_data<nitedrvr_state>();
	int gear = input_port_read(space->machine(), "GEARS");

	if (gear & 0x10)				state->m_gear = 1;
	else if (gear & 0x20)			state->m_gear = 2;
	else if (gear & 0x40)			state->m_gear = 3;
	else if (gear & 0x80)			state->m_gear = 4;

	switch (offset & 0x03)
	{
		case 0x00:						/* No remapping necessary */
			return input_port_read(space->machine(), "DSW0");
		case 0x01:						/* No remapping necessary */
			return input_port_read(space->machine(), "DSW1");
		case 0x02:						/* Remap our gear shift */
			if (state->m_gear == 1)
				return 0xe0;
			else if (state->m_gear == 2)
				return 0xd0;
			else if (state->m_gear == 3)
				return 0xb0;
			else
				return 0x70;
		case 0x03:						/* Remap our steering */
			return (input_port_read(space->machine(), "DSW2") | nitedrvr_steering(space->machine()));
		default:
			return 0xff;
	}
}

/***************************************************************************
nitedrvr_in1_r

Night Driver looks for the following:
    A: $00
        D6 - SPARE
        D7 - COIN 1
    A: $01
        D6 - SPARE
        D7 - COIN 2
    A: $02
        D6 - SPARE
        D7 - !START
    A: $03
        D6 - SPARE
        D7 - !ACC
    A: $04
        D6 - SPARE
        D7 - EXPERT
    A: $05
        D6 - SPARE
        D7 - NOVICE
    A: $06
        D6 - SPARE
        D7 - Special Alternating Signal
    A: $07
        D6 - SPARE
        D7 - Ground

Fill in the track difficulty switch and special signal in a special way.
***************************************************************************/

READ8_HANDLER( nitedrvr_in1_r )
{
	nitedrvr_state *state = space->machine().driver_data<nitedrvr_state>();
	int port = input_port_read(space->machine(), "IN0");

	state->m_ac_line = (state->m_ac_line + 1) % 3;

	if (port & 0x10)				state->m_track = 0;
	else if (port & 0x20)			state->m_track = 1;
	else if (port & 0x40)			state->m_track = 2;

	switch (offset & 0x07)
	{
		case 0x00:
			return ((port & 0x01) << 7);
		case 0x01:
			return ((port & 0x02) << 6);
		case 0x02:
			return ((port & 0x04) << 5);
		case 0x03:
			return ((port & 0x08) << 4);
		case 0x04:
			if (state->m_track == 1) return 0x80; else return 0x00;
		case 0x05:
			if (state->m_track == 0) return 0x80; else return 0x00;
		case 0x06:
			/* TODO: fix alternating signal? */
			if (state->m_ac_line==0) return 0x80; else return 0x00;
		case 0x07:
			return 0x00;
		default:
			return 0xff;
	}
}

/***************************************************************************
nitedrvr_out0_w

Sound bits:

D0 = !SPEED1
D1 = !SPEED2
D2 = !SPEED3
D3 = !SPEED4
D4 = SKID1
D5 = SKID2
***************************************************************************/

WRITE8_HANDLER( nitedrvr_out0_w )
{
	nitedrvr_state *state = space->machine().driver_data<nitedrvr_state>();

	discrete_sound_w(state->m_discrete, NITEDRVR_MOTOR_DATA, data & 0x0f);	// Motor freq data
	discrete_sound_w(state->m_discrete, NITEDRVR_SKID1_EN, data & 0x10);	// Skid1 enable
	discrete_sound_w(state->m_discrete, NITEDRVR_SKID2_EN, data & 0x20);	// Skid2 enable
}

/***************************************************************************
nitedrvr_out1_w

D0 = !CRASH - also drives a video invert signal
D1 = ATTRACT
D2 = Spare (Not used)
D3 = Not used?
D4 = LED START
D5 = Spare (Not used)
***************************************************************************/

WRITE8_HANDLER( nitedrvr_out1_w )
{
	nitedrvr_state *state = space->machine().driver_data<nitedrvr_state>();

	set_led_status(space->machine(), 0, data & 0x10);

	state->m_crash_en = data & 0x01;

	discrete_sound_w(state->m_discrete, NITEDRVR_CRASH_EN, state->m_crash_en);	// Crash enable
	discrete_sound_w(state->m_discrete, NITEDRVR_ATTRACT_EN, data & 0x02);		// Attract enable (sound disable)

	if (!state->m_crash_en)
	{
		/* Crash reset, set counter high and enable output */
		state->m_crash_data_en = 1;
		state->m_crash_data = 0x0f;
		/* Invert video */
		palette_set_color(space->machine(), 1, MAKE_RGB(0x00,0x00,0x00)); /* BLACK */
		palette_set_color(space->machine(), 0, MAKE_RGB(0xff,0xff,0xff)); /* WHITE */
	}
	discrete_sound_w(state->m_discrete, NITEDRVR_BANG_DATA, state->m_crash_data_en ? state->m_crash_data : 0);	// Crash Volume
}


TIMER_DEVICE_CALLBACK( nitedrvr_crash_toggle_callback )
{
	nitedrvr_state *state = timer.machine().driver_data<nitedrvr_state>();

	if (state->m_crash_en && state->m_crash_data_en)
	{
		state->m_crash_data--;
		discrete_sound_w(state->m_discrete, NITEDRVR_BANG_DATA, state->m_crash_data);	// Crash Volume
		if (!state->m_crash_data)
			state->m_crash_data_en = 0;	// Done counting?

		if (state->m_crash_data & 0x01)
		{
			/* Invert video */
			palette_set_color(timer.machine(), 1, MAKE_RGB(0x00,0x00,0x00)); /* BLACK */
			palette_set_color(timer.machine(), 0, MAKE_RGB(0xff,0xff,0xff)); /* WHITE */
		}
		else
		{
			/* Normal video */
			palette_set_color(timer.machine(), 0, MAKE_RGB(0x00,0x00,0x00)); /* BLACK */
			palette_set_color(timer.machine(), 1, MAKE_RGB(0xff,0xff,0xff)); /* WHITE */
		}
	}
}

MACHINE_START( nitedrvr )
{
	nitedrvr_state *state = machine.driver_data<nitedrvr_state>();

	state->m_maincpu = machine.device("maincpu");
	state->m_discrete = machine.device("discrete");

	state->save_item(NAME(state->m_gear));
	state->save_item(NAME(state->m_track));
	state->save_item(NAME(state->m_steering_buf));
	state->save_item(NAME(state->m_steering_val));
	state->save_item(NAME(state->m_crash_en));
	state->save_item(NAME(state->m_crash_data));
	state->save_item(NAME(state->m_crash_data_en));
	state->save_item(NAME(state->m_ac_line));
	state->save_item(NAME(state->m_last_steering_val));
}

MACHINE_RESET( nitedrvr )
{
	nitedrvr_state *state = machine.driver_data<nitedrvr_state>();

	state->m_gear = 1;
	state->m_track = 0;
	state->m_steering_buf = 0;
	state->m_steering_val = 0;
	state->m_crash_en = 0;
	state->m_crash_data = 0x0f;
	state->m_crash_data_en = 0;
	state->m_ac_line = 0;
	state->m_last_steering_val = 0;
}
