/******************************************************************************

    drivers/compis.c
    machine driver

    Per Ola Ingvarsson
    Tomas Karlsson

    Hardware:
        - Intel 80186 CPU 8MHz, integrated DMA(8237?), PIC(8259?), PIT(8253?)
                - Intel 80130 OSP Operating system processor (PIC 8259, PIT 8254)
        - Intel 8274 MPSC Multi-protocol serial communications controller (NEC 7201)
        - Intel 8255 PPI Programmable peripheral interface
        - Intel 8253 PIT Programmable interval timer
        - Intel 8251 USART Universal synchronous asynchronous receiver transmitter
        - National 58174 Real-time clock (compatible with 58274)
    Peripheral:
        - Intel 82720 GDC Graphic display processor (NEC uPD 7220)
        - Intel 8272 FDC Floppy disk controller (Intel iSBX-218A)
        - Western Digital WD1002-05 Winchester controller

    Memory map:

    00000-3FFFF RAM LMCS (Low Memory Chip Select)
    40000-4FFFF RAM MMCS 0 (Midrange Memory Chip Select)
    50000-5FFFF RAM MMCS 1 (Midrange Memory Chip Select)
    60000-6FFFF RAM MMCS 2 (Midrange Memory Chip Select)
    70000-7FFFF RAM MMCS 3 (Midrange Memory Chip Select)
    80000-EFFFF NOP
    F0000-FFFFF ROM UMCS (Upper Memory Chip Select)

 ******************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/i8255.h"
#include "machine/ctronics.h"
#include "includes/compis.h"
#include "imagedev/flopdrv.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/mm58274c.h"
#include "machine/upd765.h"
#include "formats/cpis_dsk.h"
#include "video/upd7220.h"

/* TODO: this is likely to come from a RAMdac ... */
static const unsigned char COMPIS_palette[16*3] =
{
	0, 0, 0,
	0, 0, 0,
	33, 200, 66,
	94, 220, 120,
	84, 85, 237,
	125, 118, 252,
	212, 82, 77,
	66, 235, 245,
	252, 85, 84,
	255, 121, 120,
	212, 193, 84,
	230, 206, 128,
	33, 176, 59,
	201, 91, 186,
	204, 204, 204,
	255, 255, 255
};

static PALETTE_INIT( compis_gdc )
{
	int i;

	for ( i = 0; i < 16; i++ ) {
		palette_set_color_rgb(machine, i, COMPIS_palette[i*3], COMPIS_palette[i*3+1], COMPIS_palette[i*3+2]);
	}
}

void compis_state::video_start()
{
	// find memory regions
//  m_char_rom = machine.region("pcg")->base();

	VIDEO_START_NAME(generic_bitmapped)(machine());
}

bool compis_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	/* graphics */
	m_hgdc->update_screen(&bitmap, &cliprect);

	return 0;
}

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
	int xi,gfx;
	UINT8 pen;

	gfx = vram[address];

	for(xi=0;xi<8;xi++)
	{
		pen = ((gfx >> xi) & 1) ? 15 : 0;

		*BITMAP_ADDR16(bitmap, y, x + xi) = pen;
	}
}

static UPD7220_INTERFACE( hgdc_intf )
{
	"screen",
	hgdc_display_pixels,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* TODO: why it writes to ROM region? */
static WRITE8_HANDLER( vram_w )
{
	UINT8 *vram = space->machine().region("vram")->base();

	vram[offset] = data;
}

static ADDRESS_MAP_START( compis_mem , AS_PROGRAM, 16 )
	AM_RANGE( 0x00000, 0x3ffff) AM_RAM
	AM_RANGE( 0x40000, 0x4ffff) AM_RAM
	AM_RANGE( 0x50000, 0x5ffff) AM_RAM
	AM_RANGE( 0x60000, 0x6ffff) AM_RAM
	AM_RANGE( 0x70000, 0x7ffff) AM_RAM

//  AM_RANGE( 0x80000, 0xeffff) AM_NOP
	AM_RANGE( 0xe8000, 0xeffff) AM_ROM AM_REGION("bios",0) AM_WRITE8(vram_w,0xffff)
	AM_RANGE( 0xf0000, 0xfffff) AM_ROM AM_REGION("bios",0) AM_WRITE8(vram_w,0xffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( compis_io, AS_IO, 16 )
	AM_RANGE( 0x0000, 0x0007) AM_DEVREADWRITE8_MODERN("ppi8255", i8255_device, read, write, 0xff00)
	AM_RANGE( 0x0080, 0x0087) AM_DEVREADWRITE8("pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE( 0x0100, 0x011b) AM_DEVREADWRITE8("mm58274c", mm58274c_r, mm58274c_w, 0xffff)
	AM_RANGE( 0x0280, 0x0283) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0xffff) /* 80150/80130 */
//  AM_RANGE( 0x0288, 0x028e) AM_DEVREADWRITE("pit8254", compis_osp_pit_r, compis_osp_pit_w ) /* PIT 8254 (80150/80130)  */
	AM_RANGE( 0x0310, 0x031f) AM_READWRITE( compis_usart_r, compis_usart_w )	/* USART 8251 Keyboard      */
	AM_RANGE( 0x0330, 0x0333) AM_DEVREADWRITE8_MODERN("upd7220", upd7220_device, read, write, 0x00ff) /* GDC 82720 PCS6:6     */
	AM_RANGE( 0x0340, 0x0343) AM_READWRITE8( compis_fdc_r, compis_fdc_w, 0xffff )	/* iSBX0 (J8) FDC 8272      */
	AM_RANGE( 0x0350, 0x0351) AM_READ(compis_fdc_dack_r)	/* iSBX0 (J8) DMA ACK       */
	AM_RANGE( 0xff00, 0xffff) AM_READWRITE( compis_i186_internal_port_r, compis_i186_internal_port_w)/* CPU 80186         */
//{ 0x0100, 0x017e, compis_null_r },    /* RTC              */
//{ 0x0180, 0x01ff, compis_null_r },    /* PCS3?            */
//{ 0x0200, 0x027f, compis_null_r },    /* Reserved         */
//{ 0x0280, 0x02ff, compis_null_r },    /* 80150 not used?      */
//{ 0x0300, 0x0300, compis_null_r },    /* Cassette  motor      */
//{ 0x0301, 0x030f, compis_null_r},     /* DMA ACK Graphics     */
//{ 0x0310, 0x031e, compis_null_r },    /* SCC 8274 Int Ack     */
//{ 0x0320, 0x0320, compis_null_r },    /* SCC 8274 Serial port     */
//{ 0x0321, 0x032f, compis_null_r },    /* DMA Terminate        */
//{ 0x0331, 0x033f, compis_null_r },    /* DMA Terminate        */
//{ 0x0341, 0x034f, compis_null_r },    /* J8 CS1 (16-bit)      */
//{ 0x0350, 0x035e, compis_null_r },    /* J8 CS1 (8-bit)       */
//{ 0x0360, 0x036e, compis_null_r },    /* J9 CS0 (8/16-bit)        */
//{ 0x0361, 0x036f, compis_null_r },    /* J9 CS1 (16-bit)      */
//{ 0x0370, 0x037e, compis_null_r },    /* J9 CS1 (8-bit)       */
//{ 0x0371, 0x037f, compis_null_r },    /* J9 CS1 (8-bit)       */
//{ 0xff20, 0xffff, compis_null_r },    /* CPU 80186            */
ADDRESS_MAP_END

/* TODO */
static ADDRESS_MAP_START( keyboard_io, AS_IO, 8 )
  AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_NOP
  AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_NOP
  AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_NOP
  AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) AM_NOP
ADDRESS_MAP_END

/* COMPIS Keyboard */

/* 2008-05 FP:
Small note about natural keyboard: currently,
- Both "SShift" keys (left and right) are not mapped
- Keypad '00' and '000' are not mapped
- "Compis !" is mapped to 'F3'
- "Compis ?" is mapped to 'F4'
- "Compis |" is mapped to 'F5'
- "Compis S" is mapped to 'F6'
- "Avbryt" is mapped to 'F7'
- "Inpassa" is mapped to 'Insert'
- "S?k" is mapped to "Print Screen"
- "Utpl?na"is mapped to 'Delete'
- "Start / Stop" is mapped to 'Pause'
- "TabL" is mapped to 'Page Up'
- "TabR" is mapped to 'Page Down'
*/

static INPUT_PORTS_START (compis)
	PORT_START("ROW0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)		PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC2\xB4 `") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('`')
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)		PORT_CHAR('\t')
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("ROW1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(a_RING " " A_RING) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(u_UMLAUT " " U_UMLAUT) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00FC) PORT_CHAR(0x00DC)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')

	PORT_START("ROW2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(o_UMLAUT " " O_UMLAUT) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(a_UMLAUT " " A_UMLAUT) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("'' *") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('*')
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')

	PORT_START("ROW3")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SShift (Left)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RCONTROL)	PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SShift (Right)") PORT_CODE(KEYCODE_RALT)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INPASSA") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S" O_UMLAUT "K") PORT_CODE(KEYCODE_PRTSCR) PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UTPL" A_RING "NA") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("START-STOP") PORT_CODE(KEYCODE_PAUSE) PORT_CHAR(UCHAR_MAMEKEY(PAUSE))
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START("ROW4")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("AVBRYT") PORT_CODE(KEYCODE_SCRLOCK) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TABL") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(PGUP))
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TABR") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("COMPIS !") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("COMPIS ?") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("COMPIS |") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1)		PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2)		PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("COMPIS S") PORT_CODE(KEYCODE_NUMLOCK) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)		PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)		PORT_CHAR(UCHAR_MAMEKEY(9_PAD))

	PORT_START("ROW5")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)		PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)		PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)		PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)		PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)		PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)		PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)		PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 00") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad 000") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad Enter") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))

	PORT_START("DSW0")
	PORT_DIPNAME( 0x18, 0x00, "S8 Test mode")
	PORT_DIPSETTING( 0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING( 0x08, "Remote" )
	PORT_DIPSETTING( 0x10, "Stand alone" )
	PORT_DIPSETTING( 0x18, "Reserved" )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "iSBX-218A DMA")
	PORT_DIPSETTING( 0x01, "Enabled" )
	PORT_DIPSETTING( 0x00, "Disabled" )
INPUT_PORTS_END


static const unsigned i86_address_mask = 0x000fffff;

static const mm58274c_interface compis_mm58274c_interface =
{
	0,	/*  mode 24*/
	1   /*  first day of week */
};

static const floppy_interface compis_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(compis),
	NULL,
	NULL
};

/* F4 Character Displayer */
static const gfx_layout compis_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	RGN_FRAC(1,1),			/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8, 8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( compis )
	GFXDECODE_ENTRY( "bios", 0x0000, compis_charlayout, 1, 7 )
GFXDECODE_END

static ADDRESS_MAP_START( upd7220_map, AS_0, 8 )
	AM_RANGE(0x00000, 0x3ffff) AM_DEVREADWRITE_MODERN("upd7220", upd7220_device, vram_r, vram_w)
ADDRESS_MAP_END

static MACHINE_CONFIG_START( compis, compis_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I80186, 8000000)	/* 8 MHz */
	MCFG_CPU_PROGRAM_MAP(compis_mem)
	MCFG_CPU_IO_MAP(compis_io)
	MCFG_CPU_VBLANK_INT("screen", compis_vblank_int)
	MCFG_CPU_CONFIG(i86_address_mask)

	MCFG_CPU_ADD("i8749", I8749, 1000000)
	MCFG_CPU_IO_MAP(keyboard_io)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_MACHINE_START(compis)
	MCFG_MACHINE_RESET(compis)

	MCFG_PIT8253_ADD( "pit8253", compis_pit8253_config )

	MCFG_PIT8254_ADD( "pit8254", compis_pit8254_config )

	MCFG_PIC8259_ADD( "pic8259_master", compis_pic8259_master_config )

	MCFG_PIC8259_ADD( "pic8259_slave", compis_pic8259_slave_config )

	MCFG_I8255_ADD( "ppi8255", compis_ppi_interface )

	/* video hardware */
	MCFG_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT(compis_gdc)
	MCFG_GFXDECODE(compis)

	MCFG_UPD7220_ADD("upd7220", XTAL_4MHz, hgdc_intf, upd7220_map) //unknown clock

	/* printer */
	MCFG_CENTRONICS_ADD("centronics", standard_centronics)

	/* uart */
	MCFG_MSM8251_ADD("uart", compis_usart_interface)

	/* rtc */
	MCFG_MM58274C_ADD("mm58274c", compis_mm58274c_interface)

	MCFG_UPD765A_ADD("upd765", compis_fdc_interface)

	MCFG_FLOPPY_2_DRIVES_ADD(compis_floppy_interface)
MACHINE_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( compis )
	ROM_REGION16_LE( 0x10000, "bios", 0 )
	ROM_LOAD16_BYTE( "sa883003.u40", 0x0000, 0x4000, CRC(195ef6bf) SHA1(eaf8ae897e1a4b62d3038ff23777ce8741b766ef) )
	ROM_LOAD16_BYTE( "sa883003.u36", 0x0001, 0x4000, CRC(7c918f56) SHA1(8ba33d206351c52f44f1aa76cc4d7f292dcef761) )
	ROM_LOAD16_BYTE( "sa883003.u39", 0x8000, 0x4000, CRC(3cca66db) SHA1(cac36c9caa2f5bb42d7a6d5b84f419318628935f) )
	ROM_LOAD16_BYTE( "sa883003.u35", 0x8001, 0x4000, CRC(43c38e76) SHA1(f32e43604107def2c2259898926d090f2ed62104) )

	ROM_REGION( 0x800, "i8749", 0 )
	ROM_LOAD( "cmpkey13.u1", 0x0000, 0x0800, CRC(3f87d138) SHA1(c04e2d325b9c04818bc7c47c3bf32b13862b11ec) )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )

ROM_END

ROM_START( compis2 )
	ROM_REGION16_LE( 0x10000, "bios", 0 )
	ROM_DEFAULT_BIOS( "v303" )
	ROM_SYSTEM_BIOS( 0, "v302", "Compis II v3.02 (1986-09-09)" )
	ROMX_LOAD( "comp302.u39", 0x0000, 0x8000, CRC(16a7651e) SHA1(4cbd4ba6c6c915c04dfc913ec49f87c1dd7344e3), ROM_BIOS(1) | ROM_SKIP(1) )
	ROMX_LOAD( "comp302.u35", 0x0001, 0x8000, CRC(ae546bef) SHA1(572e45030de552bb1949a7facbc885b8bf033fc6), ROM_BIOS(1) | ROM_SKIP(1) )
	ROM_SYSTEM_BIOS( 1, "v303", "Compis II v3.03 (1987-03-09)" )
	ROMX_LOAD( "rysa094.u39", 0x0000, 0x8000, CRC(e7302bff) SHA1(44ea20ef4008849af036c1a945bc4f27431048fb), ROM_BIOS(2) | ROM_SKIP(1) )
	ROMX_LOAD( "rysa094.u35", 0x0001, 0x8000, CRC(b0694026) SHA1(eb6b2e3cb0f42fd5ffdf44f70e652ecb9714ce30), ROM_BIOS(2) | ROM_SKIP(1) )

	ROM_REGION( 0x800, "i8749", 0 )
	ROM_LOAD( "cmpkey13.u1", 0x0000, 0x0800, CRC(3f87d138) SHA1(c04e2d325b9c04818bc7c47c3bf32b13862b11ec) )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )
ROM_END

/*   YEAR   NAME        PARENT  COMPAT MACHINE  INPUT   INIT    COMPANY     FULLNAME */
COMP(1985,	compis,		0,		0,     compis,	compis,	compis,	"Telenova", "Compis" , GAME_NOT_WORKING | GAME_NO_SOUND)
COMP(1986,	compis2,	compis,	0,     compis,	compis,	compis,	"Telenova", "Compis II" , GAME_NOT_WORKING | GAME_NO_SOUND)
