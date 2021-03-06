/*

Ohio Scientific Superboard II Model 600

PCB Layout
----------

OHIO SCIENTIFIC MODEL 600 REV D

|-------------------------------------------------------------------------------------------|
|                   2114    2114                                                            |
|                                   LS138               CN1                                 |
|       IC1         2114    2114                                                            |
|                                   LS04            LS75    LS75                            |
|                   2114    2114                                                            |
|                                   LS138                                                   |
|                   2114    2114                    LS125   LS125                           |
|                                   LS138                                                   |
| -                 2114    2114                    8T28    8T28                            |
| |                                                                                         |
|C|                 2114    2114                                                            |
|N|                                                                                         |
|3|                 2114    2114                        6502                                |
| |                                                                                         |
| -                 2114    2114                                                            |
|                                                                                           |
|                           2114    8T28                ROM0                                |
|           LS163   LS157                   LS174                                           |
|                           2114    8T28                ROM1                                |
|   CA3130  LS163   LS157                   LS02                                            |
|                           2114    8T28                ROM2                                |
|   LS14    LS163   LS157                   LS04                                            |
|                                   SW1                 ROM3                                |
|   7417    7404    LS20    ROM5    7408    LS139                                           |
|                                                       ROM4                                |
| - 74123   7474    LS163   LS165   7474                                                    |
|C|                                                                                         |
|N|         7476    7400    LS157   LS93    LS04        6850                                |
|2|                                                                                         |
| - 7403    74123   3.7MHz  LS86    LS163   LS20                                    CN4     |
|-------------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    ROM0-5  -
    6502    -
    6850    - Asynchronous Communications Interface Adapter
    8T28    - 4-Bit Bidirectional Bus Transceiver
    CA3130  - Operational Amplifier
    CN1     - OSI-48 bus connector
    CN2     -
    CN3     -
    CN4     - sound connector
    SW1     -

*/

/*

Compukit UK101

PCB Layout
----------

|-------------------------------------------------------------------------------------------|
|                   2114    2114                                                            |
|                                   LS138               CN1                                 |
|       IC1         2114    2114                                                            |
|                                   7404            LS75    LS75                            |
|                   2114    2114                                                            |
|                                   LS138                                                   |
|                   2114    2114                    LS125   LS125                           |
|                                   LS138                                                   |
| -                 2114*   2114*                   8T28*   8T28*                           |
| |                                                                                         |
|C|                 2114*   2114*                                                           |
|N|     CN4                                                                                 |
|3|                 2114*   2114*                       6502                                |
| |                                                                                         |
| -                 2114*   2114*                                                           |
|                                                                                           |
|                           2114    8T28                ROM0                                |
|           74163   LS157                                                                   |
|                           2114    8T28                ROM1                                |
|   IC66'   74163   LS157                   7402                                            |
|                                                       ROM2                                |
|   IC67'   74163   LS157                   7404                                            |
|                                                       ROM3                                |
|   IC68'   7404    7420    ROM5            LS139                                           |
|                                                       ROM4                                |
| - LS123   7474    74163   74165   LS123                                                   |
|C|                                                                                         |
|N|         7476    7400            7493    7404        6850                                |
|2|                                                                                         |
| - 7403    LS123   8MHz            74163   LS20                                            |
|-------------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    ROM0-5  -
    6502    -
    6850    - Asynchronous Communications Interface Adapter
    8T28    - 4-Bit Bidirectional Bus Transceiver
    CN1     - OSI-48 Bus Connector
    CN2     -
    CN3     -
    CN4     - UHF Modulator Output
    *       - present when 8KB of RAM installed
    '       - present when cassette option installed

*/

/*

Ohio Scientific Single Sided Floppy Interface

PCB Layout
----------

OSI 470 REV B

|-------------------------------------------------------------------|
|                                                                   |
|                                               8T26    8T26        |
|                                                                   |
|       7417                                                        |
|                                                                   |
|                       6820                6850        7400  U2C   |
|       7417                                                        |
|                                                                   |
|                                                                   |
|       7417                                                7404    |
|                                                                   |
|                                                                   |
|       7417                                                        |
|                                                                   |
|                   7400    7404    74123   74123   7410            |
|   4MHz                                                            |
|                                                                   |
|                                                                   |
|   7400    7493    74390   74390   74390   7404    7430    7404    |
|                                                                   |
|                                                                   |
|                   U6A                                             |
|                                                                   |
|-------------------------------------------------------------------|

Notes:
    All IC's shown.

    6850    - Asynchronous Communications Interface Adapter
    6820    - Peripheral Interface Adapter
    8T26    - 4-Bit Bidirectional Bus Transceiver
    U2C     - PROTO?
    U6A     - PROTO?

*/

/*

    TODO:

    - fix uk101 video to 64x16
    - floppy PIA is actually a 6820
    - break key
    - power on reset
    - Superboard II revisions A/C/D
    - uk101 medium resolution graphics
    - uk101 ay-3-8910 sound
    - cassette
    - faster cassette
    - floppy
    - wemon?

*/

#include "includes/osi.h"

/* Sound */

static const discrete_dac_r1_ladder osi600_dac =
{
	4,			// size of ladder
	{ 180, 180, 180, 180, 0, 0, 0, 0 }, // R68, R69, R70, R71
	5,			// 5V
	RES_K(1),	// R67
	0,			// no rGnd
	0			// no cFilter
};

static DISCRETE_SOUND_START( osi600_discrete_interface )
	DISCRETE_INPUT_DATA(NODE_01)

	DISCRETE_DAC_R1(NODE_02, NODE_01, DEFAULT_TTL_V_LOGIC_1, &osi600_dac)
	DISCRETE_CRFILTER(NODE_03, NODE_02, 1.0/(1.0/RES_K(1)+1.0/180+1.0/180+1.0/180+1.0/180), CAP_U(0.1))
	DISCRETE_OUTPUT(NODE_03, 100)
	DISCRETE_GAIN(NODE_04, NODE_03, 32767.0/5)
	DISCRETE_OUTPUT(NODE_04, 100)
DISCRETE_SOUND_END

static const discrete_dac_r1_ladder osi600c_dac =
{
	8,			// size of ladder
	{ RES_K(68), RES_K(33), RES_K(16), RES_K(8.2), RES_K(3.9), RES_K(2), RES_K(1), 510 }, // R73, R71, R70, R67, R68, R69, R75, R74
	5,			// 5V
	510,		// R86
	0,			// no rGnd
	CAP_U(33)	// C63
};

static DISCRETE_SOUND_START( osi600c_discrete_interface )
	DISCRETE_INPUT_DATA(NODE_01)
	DISCRETE_INPUT_LOGIC(NODE_10)

	DISCRETE_DAC_R1(NODE_02, NODE_01, DEFAULT_TTL_V_LOGIC_1, &osi600_dac)
	DISCRETE_ONOFF(NODE_03, NODE_10, NODE_02)
	DISCRETE_OUTPUT(NODE_03, 100)
DISCRETE_SOUND_END

/* Keyboard */

READ8_MEMBER( sb2m600_state::keyboard_r )
{
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	UINT8 data = 0xff;
	int bit;

	for (bit = 0; bit < 8; bit++)
	{
		if (!BIT(m_keylatch, bit)) data &= input_port_read(machine(), keynames[bit]);
	}

	return data;
}

WRITE8_MEMBER( sb2m600_state::keyboard_w )
{
	m_keylatch = data;

	discrete_sound_w(m_discrete, NODE_01, (data >> 2) & 0x0f);
}

WRITE8_MEMBER( uk101_state::keyboard_w )
{
	m_keylatch = data;
}

WRITE8_MEMBER( sb2m600_state::ctrl_w )
{
	/*

        bit     signal          description

        0       _32             screen size (0=32x32, 1=64x16)
        1       COLOR EN        color enable
        2       BK0
        3       BK1
        4       DAC DISABLE     DAC sound enable
        5
        6
        7

    */

	m_32 = BIT(data, 0);
	m_coloren = BIT(data, 1);

	discrete_sound_w(m_discrete, NODE_10, BIT(data, 4));
}

WRITE8_MEMBER( c1p_state::osi630_ctrl_w )
{
	/*

        bit     description

        0       AC control enable
        1       tone generator enable
        2       modem select (0 = printer, 1 = modem)
        3
        4
        5
        6
        7

    */

	beep_set_state(m_beep, BIT(data, 1));
}

WRITE8_MEMBER( c1p_state::osi630_sound_w )
{
	if (data) beep_set_frequency(m_beep, 49152 / data);
}

/* Disk Drive */

/*
    C000 FLOPIN         FLOPPY DISK STATUS PORT

    BIT FUNCTION
    0   DRIVE 0 READY (0 IF READY)
    1   TRACK 0 (0 IF AT TRACK 0)
    2   FAULT (0 IF FAULT)
    3
    4   DRIVE 1 READY (0 IF READY)
    5   WRITE PROTECT (0 IF WRITE PROTECT)
    6   DRIVE SELECT (1 = A OR C, 0 = B OR D)
    7   INDEX (0 IF AT INDEX HOLE)

    C002 FLOPOT         FLOPPY DISK CONTROL PORT

    BIT FUNCTION
    0   WRITE ENABLE (0 ALLOWS WRITING)
    1   ERASE ENABLE (0 ALLOWS ERASING)
        ERASE ENABLE IS ON 200us AFTER WRITE IS ON
        ERASE ENABLE IS OFF 530us AFTER WRITE IS OFF
    2   STEP BIT : INDICATES DIRECTION OF STEP (WAIT 10us FIRST)
        0 INDICATES STEP TOWARD 76
        1 INDICATES STEP TOWARD 0
    3   STEP (TRANSITION FROM 1 TO 0)
        MUST HOLD AT LEAST 10us, MIN 8us BETWEEN
    4   FAULT RESET (0 RESETS)
    5   SIDE SELECT (1 = A OR B, 0 = C OR D)
    6   LOW CURRENT (0 FOR TRKS 43-76, 1 FOR TRKS 0-42)
    7   HEAD LOAD (0 TO LOAD : MUST WAIT 40ms AFTER)

    C010 ACIA           DISK CONTROLLER ACIA STATUS PORT
    C011 ACIAIO         DISK CONTROLLER ACIA I/O PORT
*/

static WRITE_LINE_DEVICE_HANDLER(osi470_index_callback)
{
	sb2m600_state *driver_state = device->machine().driver_data<sb2m600_state>();

	driver_state->m_fdc_index = state;
}

READ8_MEMBER( c1pmf_state::osi470_pia_pa_r )
{

	/*

        bit     description

        0       _READY DRIVE 1
        1       _TRACK 00
        2       _FAULT
        3       _SECTOR
        4       _READY DRIVE 2
        5       _WRITE PROTECT
        6
        7       _INDEX

    */

	return (m_fdc_index << 7);
}

WRITE8_MEMBER( c1pmf_state::osi470_pia_pa_w )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5
        6       drive select
        7

    */
}

WRITE8_MEMBER( c1pmf_state::osi470_pia_pb_w )
{
	/*

        bit     description

        0       _WRITE ENABLE
        1       _ERASE ENABLE
        2       _STEP IN
        3       _STEP
        4       _FAULT RESET
        5       side select
        6       _LOW CURRENT
        7       _HEAD LOAD

    */
}

WRITE_LINE_MEMBER( c1pmf_state::osi470_pia_cb2_w )
{
}

static const pia6821_interface osi470_pia_intf =
{
	DEVCB_DRIVER_MEMBER(c1pmf_state, osi470_pia_pa_r),
	DEVCB_NULL, // read8_machine_func in_b_func,
	DEVCB_NULL, // read8_machine_func in_ca1_func,
	DEVCB_NULL, // read8_machine_func in_cb1_func,
	DEVCB_NULL, // read8_machine_func in_ca2_func,
	DEVCB_NULL, // read8_machine_func in_cb2_func,
	DEVCB_DRIVER_MEMBER(c1pmf_state, osi470_pia_pa_w),
	DEVCB_DRIVER_MEMBER(c1pmf_state, osi470_pia_pb_w),
	DEVCB_NULL, // write8_machine_func out_ca2_func,
	DEVCB_DRIVER_LINE_MEMBER(c1pmf_state, osi470_pia_cb2_w),
	DEVCB_NULL, // void (*irq_a_func)(int state),
	DEVCB_NULL, // void (*irq_b_func)(int state),
};

static const pia6821_interface pia_dummy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Memory Maps */

static ADDRESS_MAP_START( osi600_mem, AS_PROGRAM, 8, sb2m600_state )
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK("bank1")
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE(m_video_ram)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE(keyboard_r, keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_DEVREADWRITE("acia_0", acia6850_device, status_read, control_write)
	AM_RANGE(0xf001, 0xf001) AM_DEVREADWRITE("acia_0", acia6850_device, data_read, data_write)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( uk101_mem, AS_PROGRAM, 8, uk101_state )
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK("bank1")
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE(m_video_ram)
	AM_RANGE(0xdf00, 0xdf00) AM_MIRROR(0x03ff) AM_READ_BASE(sb2m600_state, keyboard_r) AM_WRITE(keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_MIRROR(0x00fe) AM_DEVREADWRITE("acia_0", acia6850_device, status_read, control_write)
	AM_RANGE(0xf001, 0xf001) AM_MIRROR(0x00fe) AM_DEVREADWRITE("acia_0", acia6850_device, data_read, data_write)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c1p_mem, AS_PROGRAM, 8, c1p_state )
	AM_RANGE(0x0000, 0x4fff) AM_RAMBANK("bank1")
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc704, 0xc707) AM_DEVREADWRITE("pia_1", pia6821_device, read, write)
	AM_RANGE(0xc708, 0xc70b) AM_DEVREADWRITE("pia_2", pia6821_device, read, write)
	AM_RANGE(0xc70c, 0xc70f) AM_DEVREADWRITE("pia_3", pia6821_device, read, write)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE(m_video_ram)
	AM_RANGE(0xd400, 0xd7ff) AM_RAM AM_BASE(m_color_ram)
	AM_RANGE(0xd800, 0xd800) AM_WRITE_BASE(sb2m600_state, ctrl_w)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE_BASE(sb2m600_state, keyboard_r, keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_DEVREADWRITE("acia_0", acia6850_device, status_read, control_write)
	AM_RANGE(0xf001, 0xf001) AM_DEVREADWRITE("acia_0", acia6850_device, data_read, data_write)
	AM_RANGE(0xf7c0, 0xf7c0) AM_WRITE(osi630_sound_w)
	AM_RANGE(0xf7e0, 0xf7e0) AM_WRITE(osi630_ctrl_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c1pmf_mem, AS_PROGRAM, 8, c1pmf_state )
	AM_RANGE(0x0000, 0x4fff) AM_RAMBANK("bank1")
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xc003) AM_DEVREADWRITE("pia_0", pia6821_device, read, write) // FDC
	AM_RANGE(0xc010, 0xc010) AM_DEVREADWRITE("acia_1", acia6850_device, status_read, control_write)
	AM_RANGE(0xc011, 0xc011) AM_DEVREADWRITE("acia_1", acia6850_device, data_read, data_write)
	AM_RANGE(0xc704, 0xc707) AM_DEVREADWRITE("pia_1", pia6821_device, read, write)
	AM_RANGE(0xc708, 0xc70b) AM_DEVREADWRITE("pia_2", pia6821_device, read, write)
	AM_RANGE(0xc70c, 0xc70f) AM_DEVREADWRITE("pia_3", pia6821_device, read, write)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_BASE(m_video_ram)
	AM_RANGE(0xd400, 0xd7ff) AM_RAM AM_BASE(m_color_ram)
	AM_RANGE(0xd800, 0xd800) AM_WRITE_BASE(sb2m600_state, ctrl_w)
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE_BASE(sb2m600_state, keyboard_r, keyboard_w)
	AM_RANGE(0xf000, 0xf000) AM_DEVREADWRITE("acia_0", acia6850_device, status_read, control_write)
	AM_RANGE(0xf001, 0xf001) AM_DEVREADWRITE("acia_0", acia6850_device, data_read, data_write)
	AM_RANGE(0xf7c0, 0xf7c0) AM_WRITE_BASE(c1p_state, osi630_sound_w)
	AM_RANGE(0xf7e0, 0xf7e0) AM_WRITE_BASE(c1p_state, osi630_ctrl_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( osi600 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TAB) PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPEAT") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(10)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RUB OUT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
INPUT_PORTS_END

static INPUT_PORTS_START( uk101 )
	PORT_INCLUDE(osi600)

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('~')

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
INPUT_PORTS_END

/* Machine Start */

READ_LINE_MEMBER( sb2m600_state::cassette_rx )
{
	return ((m_cassette)->input() > 0.0) ? 1 : 0;
}

WRITE_LINE_MEMBER( sb2m600_state::cassette_tx )
{
	m_cassette->output(state ? +1.0 : -1.0);
}

static ACIA6850_INTERFACE( osi600_acia_intf )
{
	X1/32,
	X1/32,
	DEVCB_DRIVER_LINE_MEMBER(sb2m600_state, cassette_rx),
	DEVCB_DRIVER_LINE_MEMBER(sb2m600_state, cassette_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static ACIA6850_INTERFACE( uk101_acia_intf )
{
	500000, //
	500000, //
	DEVCB_DRIVER_LINE_MEMBER(sb2m600_state, cassette_rx),
	DEVCB_DRIVER_LINE_MEMBER(sb2m600_state, cassette_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static ACIA6850_INTERFACE( osi470_acia_intf )
{
	0,				// clocked in from the floppy drive
	XTAL_4MHz/8,	// 250 kHz
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

void sb2m600_state::machine_start()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	/* configure RAM banking */
	memory_configure_bank(machine(), "bank1", 0, 1, machine().region(M6502_TAG)->base(), 0);
	memory_set_bank(machine(), "bank1", 0);

	switch (ram_get_size(m_ram))
	{
	case 4*1024:
		program->install_readwrite_bank(0x0000, 0x0fff, "bank1");
		program->unmap_readwrite(0x1000, 0x1fff);
		break;

	case 8*1024:
		program->install_readwrite_bank(0x0000, 0x1fff, "bank1");
		break;
	}

	/* register for state saving */
	save_item(NAME(m_keylatch));
	save_pointer(NAME(m_video_ram), OSI600_VIDEORAM_SIZE);
}

void c1p_state::machine_start()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	/* configure RAM banking */
	memory_configure_bank(machine(), "bank1", 0, 1, machine().region(M6502_TAG)->base(), 0);
	memory_set_bank(machine(), "bank1", 0);

	switch (ram_get_size(m_ram))
	{
	case 8*1024:
		program->install_readwrite_bank(0x0000, 0x1fff, "bank1");
		program->unmap_readwrite(0x2000, 0x4fff);
		break;

	case 20*1024:
		program->install_readwrite_bank(0x0000, 0x4fff, "bank1");
		break;
	}

	/* register for state saving */
	save_item(NAME(m_keylatch));
	save_item(NAME(m_32));
	save_item(NAME(m_coloren));
	save_pointer(NAME(m_video_ram), OSI600_VIDEORAM_SIZE);
	save_pointer(NAME(m_color_ram), OSI630_COLORRAM_SIZE);
}

void c1pmf_state::machine_start()
{
	c1p_state::machine_start();
}

static FLOPPY_OPTIONS_START(osi)
	FLOPPY_OPTION(osi, "img", "OSI disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([1])
		TRACKS([36])
		SECTORS([10])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_interface osi_floppy_interface =
{
	DEVCB_LINE(osi470_index_callback),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD_40,
	FLOPPY_OPTIONS_NAME(osi),
	NULL,
	NULL
};

/* F4 Character Displayer */
static const gfx_layout osi_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( osi )
	GFXDECODE_ENTRY( "chargen", 0x0000, osi_charlayout, 0, 1 )
GFXDECODE_END

/* Machine Drivers */

static MACHINE_CONFIG_START( osi600, sb2m600_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(M6502_TAG, M6502, X1/4) // .98304 MHz
	MCFG_CPU_PROGRAM_MAP(osi600_mem)

	/* video hardware */
	MCFG_FRAGMENT_ADD(osi600_video)
	MCFG_GFXDECODE(osi)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MCFG_SOUND_CONFIG_DISCRETE(osi600_discrete_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cassette ACIA */
	MCFG_ACIA6850_ADD("acia_0", osi600_acia_intf)

	/* cassette */
	MCFG_CASSETTE_ADD(CASSETTE_TAG, default_cassette_interface)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4K")
	MCFG_RAM_EXTRA_OPTIONS("8K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( uk101, uk101_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(M6502_TAG, M6502, UK101_X1/8) // 1 MHz
	MCFG_CPU_PROGRAM_MAP(uk101_mem)

	/* video hardware */
	MCFG_FRAGMENT_ADD(uk101_video)
	MCFG_GFXDECODE(osi)

	/* cassette ACIA */
	MCFG_ACIA6850_ADD("acia_0", uk101_acia_intf)

	/* cassette */
	MCFG_CASSETTE_ADD(CASSETTE_TAG, default_cassette_interface)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4K")
	MCFG_RAM_EXTRA_OPTIONS("8K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( c1p, c1p_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(M6502_TAG, M6502, X1/4) // .98304 MHz
	MCFG_CPU_PROGRAM_MAP(c1p_mem)

	/* video hardware */
	MCFG_FRAGMENT_ADD(osi630_video)
	MCFG_GFXDECODE(osi)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MCFG_SOUND_CONFIG_DISCRETE(osi600c_discrete_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_PIA6821_ADD( "pia_1", pia_dummy_intf )
	MCFG_PIA6821_ADD( "pia_2", pia_dummy_intf )
	MCFG_PIA6821_ADD( "pia_3", pia_dummy_intf )

	/* cassette ACIA */
	MCFG_ACIA6850_ADD("acia_0", osi600_acia_intf)

	/* cassette */
	MCFG_CASSETTE_ADD(CASSETTE_TAG, default_cassette_interface)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("8K")
	MCFG_RAM_EXTRA_OPTIONS("20K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED_CLASS( c1pmf, c1p, c1pmf_state )
	MCFG_CPU_MODIFY(M6502_TAG)
	MCFG_CPU_PROGRAM_MAP(c1pmf_mem)

	MCFG_PIA6821_ADD( "pia_0", osi470_pia_intf )

	/* floppy ACIA */
	MCFG_ACIA6850_ADD("acia_1", osi470_acia_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, osi_floppy_interface)

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("20K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( sb2m600b )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "basus01.u9",  0xa000, 0x0800, CRC(f4f5dec0) SHA1(b41bf24b4470b6e969d32fe48d604637276f846e) )
	ROM_LOAD( "basus02.u10", 0xa800, 0x0800, CRC(0039ef6a) SHA1(1397f0dc170c16c8e0c7d02e63099e986e86385b) )
	ROM_LOAD( "basus03.u11", 0xb000, 0x0800, CRC(ca25f8c1) SHA1(f5e8ee93a5e0656657d0cc60ef44e8a24b8b0a80) )
	ROM_LOAD( "basus04.u12", 0xb800, 0x0800, CRC(8ee6030e) SHA1(71f210163e4268cba2dd78a97c4d8f5dcebf980e) )
	ROM_LOAD( "monde01.u13", 0xf800, 0x0800, CRC(95a44d2e) SHA1(4a0241c4015b94c436d0f0f58b3dd9d5207cd847) )

	ROM_REGION( 0x800, "chargen",0)
	ROM_LOAD( "chgsup2.u41", 0x0000, 0x0800, CRC(735f5e0a) SHA1(87c6271497c5b00a974d905766e91bb965180594) )
ROM_END

#define rom_c1p rom_sb2m600b
#define rom_c1pmf rom_sb2m600b

ROM_START( uk101 )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "basuk01.ic9",   0xa000, 0x0800, CRC(9d3caa92) SHA1(b2c3d1af0c4f3cead1dbd44aaf5a11680880f772) )
	ROM_LOAD( "basus02.ic10",  0xa800, 0x0800, CRC(0039ef6a) SHA1(1397f0dc170c16c8e0c7d02e63099e986e86385b) )
	ROM_LOAD( "basuk03.ic11",  0xb000, 0x0800, CRC(0d011242) SHA1(54bd33522a5d1991086eeeff3a4f73c026be45b6) )
	ROM_LOAD( "basuk04.ic12",  0xb800, 0x0800, CRC(667223e8) SHA1(dca78be4b98317413376d69119942d692e39575a) )
	ROM_LOAD( "monuk02.ic13",  0xf800, 0x0800, CRC(04ac5822) SHA1(2bbbcd0ca18103fd68afcf64a7483653b925d83e) )

	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "chguk101.ic41", 0x0000, 0x0800, CRC(fce2c84a) SHA1(baa66a7a48e4d62282671ef53abfaf450b888b70) )
ROM_END

/* Driver Initialization */

static TIMER_CALLBACK( setup_beep )
{
	device_t *speaker = machine.device(BEEPER_TAG);
	beep_set_state(speaker, 0);
	beep_set_frequency(speaker, 300);
}

static DRIVER_INIT( c1p )
{
	machine.scheduler().timer_set(attotime::zero, FUNC(setup_beep));
}


/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE   INPUT      INIT   COMPANY            FULLNAME
COMP( 1978, sb2m600b,	0,		0,		osi600,   osi600,    0,		"Ohio Scientific", "Superboard II Model 600 (Rev. B)", GAME_NOT_WORKING)
//COMP( 1980, sb2m600c, 0,          0,      osi600c,  osi600,    0,     "Ohio Scientific", "Superboard II Model 600 (Rev. C)", GAME_NOT_WORKING)
COMP( 1980, c1p,	sb2m600b,	0,		c1p,	  osi600,    c1p,	"Ohio Scientific", "Challenger 1P Series 2", GAME_NOT_WORKING)
COMP( 1980, c1pmf,	sb2m600b,	0,		c1pmf,	  osi600,    c1p,	"Ohio Scientific", "Challenger 1P MF Series 2", GAME_NOT_WORKING)
COMP( 1979, uk101,	sb2m600b,	0,		uk101,	  uk101,     0, 	"Compukit",        "UK101", GAME_NOT_WORKING | GAME_NO_SOUND)
