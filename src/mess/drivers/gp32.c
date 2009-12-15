/**************************************************************************
 *
 * gp32.c - Game Park GP32
 * Skeleton by R. Belmont
 *
 * CPU: Samsung S3C2400X01 SoC
 * S3C2400X01 consists of:
 *    ARM920T CPU core + MMU
 *    LCD controller
 *    DMA controller
 *    Interrupt controller
 *    USB controller
 *    and more.
 *
 **************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/arm7/arm7.h"
#include "cpu/arm7/arm7core.h"
#include "machine/smartmed.h"
#include "includes/gp32.h"
#include "sound/dac.h"
#include "video/generic.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void ATTR_PRINTF(3,4) verboselog( running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if (VERBOSE_LEVEL >= n_level)
	{
		va_list v;
		char buf[32768];
		va_start( v, s_fmt);
		vsprintf( buf, s_fmt, v);
		va_end( v);
		logerror( "%s: %s", cpuexec_describe_context( machine), buf);
	}
}

#define CLOCK_MULTIPLIER 1

#define BIT(x,n) (((x)>>(n))&1)
#define BITS(x,m,n) (((x)>>(n))&((1<<((m)-(n)+1))-1))

#define MPLLCON  1
#define UPLLCON  2

static UINT32 *s3c240x_ram;
static UINT8 *eeprom_data;

static UINT32 s3c240x_get_hclk( int reg);

// LCD CONTROLLER

static UINT32 s3c240x_lcd_regs[0x400/4];
static emu_timer *s3c240x_lcd_timer;

static struct
{
	UINT32 vramaddr_cur;
	UINT32 vramaddr_max;
	UINT32 offsize;
	UINT32 pagewidth_cur;
	UINT32 pagewidth_max;
	UINT32 bppmode;
	UINT32 bswp, hwswp;
	UINT32 hozval, lineval;
	int vpos, hpos;
} s3c240x_lcd;

#define BPPMODE_TFT_01	0x08
#define BPPMODE_TFT_02	0x09
#define BPPMODE_TFT_04	0x0A
#define BPPMODE_TFT_08	0x0B
#define BPPMODE_TFT_16	0x0C

//             76543210 76543210 76543210 76543210
// 5551 16-bit 00000000 00000000 RRRRRGGG GGBBBBB0
// 5551 32-bit 00000000 RRRRRI00 GGGGGI00 BBBBBI00
// 565  16-bit 00000000 00000000 RRRRRGGG GGBBBBB0
// 565  32-bit 00000000 RRRRR000 GGGGG000 BBBBB000

static void s3c240x_lcd_dma_reload( running_machine *machine)
{
	s3c240x_lcd.vramaddr_cur = s3c240x_lcd_regs[5] << 1;
	s3c240x_lcd.vramaddr_max = ((s3c240x_lcd_regs[5] & 0xFFE00000) | s3c240x_lcd_regs[6]) << 1;
	s3c240x_lcd.offsize = BITS( s3c240x_lcd_regs[7], 21, 11);
	s3c240x_lcd.pagewidth_cur = 0;
	s3c240x_lcd.pagewidth_max = BITS( s3c240x_lcd_regs[7], 10, 0);
	verboselog( machine, 3, "LCD - vramaddr %08X %08X offsize %08X pagewidth %08X\n", s3c240x_lcd.vramaddr_cur, s3c240x_lcd.vramaddr_max, s3c240x_lcd.offsize, s3c240x_lcd.pagewidth_max);
}

static void s3c240x_lcd_dma_init( running_machine *machine)
{
	s3c240x_lcd_dma_reload( machine);
	s3c240x_lcd.bppmode = BITS( s3c240x_lcd_regs[0], 4, 1);
	s3c240x_lcd.bswp = BIT( s3c240x_lcd_regs[4], 1);
	s3c240x_lcd.hwswp = BIT( s3c240x_lcd_regs[4], 0);
	s3c240x_lcd.lineval = BITS( s3c240x_lcd_regs[1], 23, 14);
	s3c240x_lcd.hozval = BITS( s3c240x_lcd_regs[2], 18, 8);
}

static UINT32 s3c240x_lcd_dma_read( running_machine *machine)
{
	UINT8 *vram, data[4];
	int i;
	for (i = 0; i < 2; i++)
	{
		vram = (UINT8 *)s3c240x_ram + s3c240x_lcd.vramaddr_cur - 0x0C000000;
		data[i*2+0] = vram[0];
		data[i*2+1] = vram[1];
		s3c240x_lcd.vramaddr_cur += 2;
		s3c240x_lcd.pagewidth_cur++;
		if (s3c240x_lcd.pagewidth_cur >= s3c240x_lcd.pagewidth_max)
		{
			s3c240x_lcd.vramaddr_cur += s3c240x_lcd.offsize << 1;
			s3c240x_lcd.pagewidth_cur = 0;
		}
	}
	if (s3c240x_lcd.hwswp == 0)
	{
		if (s3c240x_lcd.bswp == 0)
		{
			return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0] << 0);
		}
		else
		{
			return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3] << 0);
		}
	}
	else
	{
		if (s3c240x_lcd.bswp == 0)
		{
			return (data[1] << 24) | (data[0] << 16) | (data[3] << 8) | (data[2] << 0);
		}
		else
		{
			return (data[2] << 24) | (data[3] << 16) | (data[0] << 8) | (data[1] << 0);
		}
	}
}

static void s3c240x_lcd_render_01( running_machine *machine)
{
	bitmap_t *bitmap = machine->generic.tmpbitmap;
	UINT32 *scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
	int i, j;
	for (i = 0; i < 4; i++)
	{
		UINT32 data = s3c240x_lcd_dma_read( machine);
		for (j = 0; j < 32; j++)
		{
			if (data & 0x80000000)
			{
				*scanline++ = RGB_BLACK;
			}
			else
			{
				*scanline++ = RGB_WHITE;
			}
			data = data << 1;
			s3c240x_lcd.hpos++;
			if (s3c240x_lcd.hpos >= (s3c240x_lcd.pagewidth_max << 4))
			{
				s3c240x_lcd.vpos = (s3c240x_lcd.vpos + 1) % (s3c240x_lcd.lineval + 1);
				s3c240x_lcd.hpos = 0;
				scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
			}
		}
	}
}

static void s3c240x_lcd_render_04( running_machine *machine)
{
	bitmap_t *bitmap = machine->generic.tmpbitmap;
	UINT32 *scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
	int i, j;
	for (i = 0; i < 4; i++)
	{
		UINT32 data = s3c240x_lcd_dma_read( machine);
		for (j = 0; j < 8; j++)
		{
			*scanline++ = palette_get_color( machine, (data >> 28) & 0xFF);
			data = data << 4;
			s3c240x_lcd.hpos++;
			if (s3c240x_lcd.hpos >= (s3c240x_lcd.pagewidth_max << 2))
			{
				s3c240x_lcd.vpos = (s3c240x_lcd.vpos + 1) % (s3c240x_lcd.lineval + 1);
				s3c240x_lcd.hpos = 0;
				scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
			}
		}
	}
}

static void s3c240x_lcd_render_08( running_machine *machine)
{
	bitmap_t *bitmap = machine->generic.tmpbitmap;
	UINT32 *scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
	int i, j;
	for (i = 0; i < 4; i++)
	{
		UINT32 data = s3c240x_lcd_dma_read( machine);
		for (j = 0; j < 4; j++)
		{
			*scanline++ = palette_get_color( machine, (data >> 24) & 0xFF);
			data = data << 8;
			s3c240x_lcd.hpos++;
			if (s3c240x_lcd.hpos >= (s3c240x_lcd.pagewidth_max << 1))
			{
				s3c240x_lcd.vpos = (s3c240x_lcd.vpos + 1) % (s3c240x_lcd.lineval + 1);
				s3c240x_lcd.hpos = 0;
				scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
			}
		}
	}
}

static void s3c240x_lcd_render_16( running_machine *machine)
{
	bitmap_t *bitmap = machine->generic.tmpbitmap;
	UINT32 *scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
	int i, j;
	for (i = 0; i < 4; i++)
	{
		UINT32 data = s3c240x_lcd_dma_read( machine);
		for (j = 0; j < 2; j++)
		{
			UINT8 r, g, b;
			r = BITS( data, 31, 27) << 3;
			g = BITS( data, 26, 22) << 3;
			b = BITS( data, 21, 17) << 3;
			*scanline++ = MAKE_RGB( r, g, b);
			data = data << 16;
			s3c240x_lcd.hpos++;
			if (s3c240x_lcd.hpos >= (s3c240x_lcd.pagewidth_max << 0))
			{
				s3c240x_lcd.vpos = (s3c240x_lcd.vpos + 1) % (s3c240x_lcd.lineval + 1);
				s3c240x_lcd.hpos = 0;
				scanline = BITMAP_ADDR32( bitmap, s3c240x_lcd.vpos, s3c240x_lcd.hpos);
			}
		}
	}
}

static TIMER_CALLBACK( s3c240x_lcd_timer_exp )
{
	const device_config *screen = machine->primary_screen;
	verboselog( machine, 2, "LCD timer callback\n");
	s3c240x_lcd.vpos = video_screen_get_vpos( screen);
	s3c240x_lcd.hpos = video_screen_get_hpos( screen);
	verboselog( machine, 3, "LCD - vpos %d hpos %d\n", s3c240x_lcd.vpos, s3c240x_lcd.hpos);
	if (s3c240x_lcd.vramaddr_cur >= s3c240x_lcd.vramaddr_max)
	{
		s3c240x_lcd_dma_reload( machine);
	}
	verboselog( machine, 3, "LCD - vramaddr %08X\n", s3c240x_lcd.vramaddr_cur);
	while (s3c240x_lcd.vramaddr_cur < s3c240x_lcd.vramaddr_max)
	{
		switch (s3c240x_lcd.bppmode)
		{
			case BPPMODE_TFT_01 : s3c240x_lcd_render_01( machine); break;
			case BPPMODE_TFT_04 : s3c240x_lcd_render_04( machine); break;
			case BPPMODE_TFT_08 : s3c240x_lcd_render_08( machine); break;
			case BPPMODE_TFT_16 : s3c240x_lcd_render_16( machine); break;
			default : verboselog( machine, 0, "s3c240x_lcd_timer_exp: bppmode %d not supported\n", s3c240x_lcd.bppmode); break;
		}
		if ((s3c240x_lcd.vpos == 0) && (s3c240x_lcd.hpos == 0)) break;
	}
	timer_adjust_oneshot( s3c240x_lcd_timer, video_screen_get_time_until_pos( screen, s3c240x_lcd.vpos, s3c240x_lcd.hpos), 0);
}

static VIDEO_START( gp32 )
{
	VIDEO_START_CALL(generic_bitmapped);
}

static VIDEO_UPDATE( gp32 )
{
	running_machine *machine = screen->machine;
	VIDEO_UPDATE_CALL(generic_bitmapped);
	s3c240x_lcd_dma_init( machine);
	return 0;
}

static READ32_HANDLER( s3c240x_lcd_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_lcd_regs[offset];
	switch (offset)
	{
		// LCDCON1
		case 0x00 / 4 :
		{
			// make sure line counter is going
			UINT32 lineval = BITS( s3c240x_lcd_regs[1], 23, 14);
			data = (data & ~0xFFFC0000) | ((lineval - video_screen_get_vpos( machine->primary_screen)) << 18);
		}
		break;
	}
	verboselog( machine, 9, "(LCD) %08X -> %08X (PC %08X)\n", 0x14A00000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static void s3c240x_lcd_configure( running_machine *machine)
{
	const device_config *screen = machine->primary_screen;
	UINT32 vspw, vbpd, lineval, vfpd, hspw, hbpd, hfpd, hozval, clkval, hclk;
	double framerate, vclk;
	rectangle visarea;
	vspw = BITS( s3c240x_lcd_regs[1], 5, 0);
	vbpd = BITS( s3c240x_lcd_regs[1], 31, 24);
	lineval = BITS( s3c240x_lcd_regs[1], 23, 14);
	vfpd = BITS( s3c240x_lcd_regs[1], 13, 6);
	hspw = BITS( s3c240x_lcd_regs[3], 7, 0);
	hbpd = BITS( s3c240x_lcd_regs[2], 25, 19);
	hfpd = BITS( s3c240x_lcd_regs[2], 7, 0);
	hozval = BITS( s3c240x_lcd_regs[2], 18, 8);
	clkval = BITS( s3c240x_lcd_regs[0], 17, 8);
	hclk = s3c240x_get_hclk( MPLLCON);
	verboselog( machine, 3, "LCD - vspw %d vbpd %d lineval %d vfpd %d hspw %d hbpd %d hfpd %d hozval %d clkval %d hclk %d\n", vspw, vbpd, lineval, vfpd, hspw, hbpd, hfpd, hozval, clkval, hclk);
	vclk = (double)(hclk / ((clkval + 1) * 2));
	verboselog( machine, 3, "LCD - vclk %f\n", vclk);
	framerate = vclk / (((vspw + 1) + (vbpd + 1) + (lineval + 1) + (vfpd + 1)) * ((hspw + 1) + (hbpd + 1) + (hfpd + 1) + (hozval + 1)));
	verboselog( machine, 3, "LCD - framerate %f\n", framerate);
	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = hozval;
	visarea.max_y = lineval;
	verboselog( machine, 3, "LCD - visarea min_x %d min_y %d max_x %d max_y %d\n", visarea.min_x, visarea.min_y, visarea.max_x, visarea.max_y);
	video_screen_configure( screen, hozval + 1, lineval + 1, &visarea, HZ_TO_ATTOSECONDS( framerate));
}

static void s3c240x_lcd_start( running_machine *machine)
{
	const device_config *screen = machine->primary_screen;
	verboselog( machine, 1, "LCD start\n");
	s3c240x_lcd_configure( machine);
	s3c240x_lcd_dma_init( machine);
	timer_adjust_oneshot( s3c240x_lcd_timer, video_screen_get_time_until_pos( screen, 0, 0), 0);
}

static void s3c240x_lcd_stop( running_machine *machine)
{
	verboselog( machine, 1, "LCD stop\n");
	timer_adjust_oneshot( s3c240x_lcd_timer, attotime_never, 0);
}

static void s3c240x_lcd_recalc( running_machine *machine)
{
	if (s3c240x_lcd_regs[0] & 1)
	{
		s3c240x_lcd_start( machine);
	}
	else
	{
		s3c240x_lcd_stop( machine);
	}
}

static WRITE32_HANDLER( s3c240x_lcd_w )
{
	running_machine *machine = space->machine;
	UINT32 old_value = s3c240x_lcd_regs[offset];
	verboselog( machine, 9, "(LCD) %08X <- %08X (PC %08X)\n", 0x14A00000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_lcd_regs[offset]);
	switch (offset)
	{
		// LCDCON1
		case 0x00 / 4 :
		{
			if ((old_value & 1) != (data & 1))
			{
				s3c240x_lcd_recalc( machine);
			}
		}
		break;
	}
}

// LCD PALETTE

static UINT32 s3c240x_lcd_palette[0x400/4];

static READ32_HANDLER( s3c240x_lcd_palette_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_lcd_palette[offset];
	verboselog( machine, 9, "(LCD) %08X -> %08X (PC %08X)\n", 0x14A00400 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_lcd_palette_w )
{
	running_machine *machine = space->machine;
	UINT8 r, g, b;
	verboselog( machine, 9, "(LCD) %08X <- %08X (PC %08X)\n", 0x14A00400 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_lcd_palette[offset]);
	if (mem_mask != 0xffffffff)
	{
		verboselog( machine, 0, "s3c240x_lcd_palette_w: unknown mask %08x\n", mem_mask);
	}
	r = BITS( data, 15, 11) << 3;
	g = BITS( data, 10,  6) << 3;
	b = BITS( data,  5,  1) << 3;
	palette_set_color_rgb( machine, offset, r, g, b);
}

// CLOCK & POWER MANAGEMENT

static UINT32 s3c240x_clkpow_regs[0x18/4];

static UINT32 s3c240x_get_fclk( int reg)
{
	UINT32 data, mdiv, pdiv, sdiv;
	data = s3c240x_clkpow_regs[reg]; // MPLLCON or UPLLCON
	mdiv = BITS( data, 19, 12);
	pdiv = BITS( data, 9, 4);
	sdiv = BITS( data, 1, 0);
	return (UINT32)((double)((mdiv + 8) * 12000000) / (double)((pdiv + 2) * (1 << sdiv)));
}

static UINT32 s3c240x_get_hclk( int reg)
{
	switch (s3c240x_clkpow_regs[5] & 0x3) // CLKDIVN
	{
		case 0 : return s3c240x_get_fclk( reg) / 1;
		case 1 : return s3c240x_get_fclk( reg) / 1;
		case 2 : return s3c240x_get_fclk( reg) / 2;
		case 3 : return s3c240x_get_fclk( reg) / 2;
	}
	return 0;
}

static UINT32 s3c240x_get_pclk( int reg)
{
	switch (s3c240x_clkpow_regs[5] & 0x3) // CLKDIVN
	{
		case 0 : return s3c240x_get_fclk( reg) / 1;
		case 1 : return s3c240x_get_fclk( reg) / 2;
		case 2 : return s3c240x_get_fclk( reg) / 2;
		case 3 : return s3c240x_get_fclk( reg) / 4;
	}
	return 0;
}

static READ32_HANDLER( s3c240x_clkpow_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_clkpow_regs[offset];
	verboselog( machine, 9, "(CLKPOW) %08X -> %08X (PC %08X)\n", 0x14800000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_clkpow_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(CLKPOW) %08X <- %08X (PC %08X)\n", 0x14800000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_clkpow_regs[offset]);
	switch (offset)
	{
		// MPLLCON
		case 0x04 / 4 :
		{
			cputag_set_clock( machine, "maincpu", s3c240x_get_fclk( MPLLCON) * CLOCK_MULTIPLIER);
		}
		break;
	}
}

// INTERRUPT CONTROLLER

static UINT32 s3c240x_irq_regs[0x18/4];

static void s3c240x_check_pending_irq( running_machine *machine)
{
	if (s3c240x_irq_regs[0] != 0)
	{
		UINT32 int_type = 0, temp;
		temp = s3c240x_irq_regs[0];
		while (!(temp & 1))
		{
			int_type++;
			temp = temp >> 1;
		}
		s3c240x_irq_regs[4] |= (1 << int_type); // INTPND
		s3c240x_irq_regs[5] = int_type; // INTOFFSET
		cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
	}
	else
	{
		cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, CLEAR_LINE);
	}
}

static void s3c240x_request_irq( running_machine *machine, UINT32 int_type)
{
	verboselog( machine, 5, "request irq %d\n", int_type);
	verboselog( machine, 5, "(1) %08X %08X %08X %08X %08X %08X\n", s3c240x_irq_regs[0], s3c240x_irq_regs[1], s3c240x_irq_regs[2], s3c240x_irq_regs[3], s3c240x_irq_regs[4], s3c240x_irq_regs[5]);
	if (s3c240x_irq_regs[0] == 0)
	{
		s3c240x_irq_regs[0] |= (1 << int_type); // SRCPND
		s3c240x_irq_regs[4] |= (1 << int_type); // INTPND
		s3c240x_irq_regs[5] = int_type; // INTOFFSET
		cpu_set_input_line( cputag_get_cpu( machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
	}
	else
	{
		s3c240x_irq_regs[0] |= (1 << int_type); // SRCPND
		s3c240x_check_pending_irq( machine);
	}
}


static READ32_HANDLER( s3c240x_irq_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_irq_regs[offset];
	verboselog( machine, 9, "(IRQ) %08X -> %08X (PC %08X)\n", 0x14400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_irq_w )
{
	running_machine *machine = space->machine;
	UINT32 old_value = s3c240x_irq_regs[offset];
	verboselog( machine, 9, "(IRQ) %08X <- %08X (PC %08X)\n", 0x14400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_irq_regs[offset]);
	switch (offset)
	{
		// SRCPND
		case 0x00 / 4 :
		{
			s3c240x_irq_regs[0] = (old_value & ~data); // clear only the bit positions of SRCPND corresponding to those set to one in the data
			verboselog( machine, 5, "(2) %08X %08X %08X %08X %08X %08X\n", s3c240x_irq_regs[0], s3c240x_irq_regs[1], s3c240x_irq_regs[2], s3c240x_irq_regs[3], s3c240x_irq_regs[4], s3c240x_irq_regs[5]);
			s3c240x_check_pending_irq( machine);
		}
		break;
		// INTPND
		case 0x10 / 4 :
		{
			s3c240x_irq_regs[4] = (old_value & ~data); // clear only the bit positions of INTPND corresponding to those set to one in the data
			verboselog( machine, 5, "(3) %08X %08X %08X %08X %08X %08X\n", s3c240x_irq_regs[0], s3c240x_irq_regs[1], s3c240x_irq_regs[2], s3c240x_irq_regs[3], s3c240x_irq_regs[4], s3c240x_irq_regs[5]);
		}
		break;
	}
}

// PWM TIMER

#if 0
static const char *timer_reg_names[] =
{
	"Timer config 0",
	"Timer config 1",
	"Timer control",
	"Timer count buffer 0",
	"Timer compare buffer 0",
	"Timer count observation 0",
	"Timer count buffer 1",
	"Timer compare buffer 1",
	"Timer count observation 1",
	"Timer count buffer 2",
	"Timer compare buffer 2",
	"Timer count observation 2",
	"Timer count buffer 3",
	"Timer compare buffer 3",
	"Timer count observation 3",
	"Timer count buffer 4",
	"Timer compare buffer 4",
	"Timer count observation 4",
};
#endif

static emu_timer *s3c240x_pwm_timer[5];
static UINT32 s3c240x_pwm_regs[0x44/4];

static READ32_HANDLER( s3c240x_pwm_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_pwm_regs[offset];
	verboselog( machine, 9, "(PWM) %08X -> %08X (PC %08X)\n", 0x15100000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static void s3c240x_pwm_start( running_machine *machine, int timer)
{
	const int mux_table[] = { 2, 4, 8, 16};
	const int prescaler_shift[] = { 0, 0, 8, 8, 8};
	const int mux_shift[] = { 0, 4, 8, 12, 16};
	const int tcon_shift[] = { 0, 8, 12, 16, 20};
	const UINT32 *regs = &s3c240x_pwm_regs[3+timer*3];
	UINT32 prescaler, mux, cnt, cmp, auto_reload;
	double freq, hz;
	verboselog( machine, 1, "PWM %d start\n", timer);
	prescaler = (s3c240x_pwm_regs[0] >> prescaler_shift[timer]) & 0xFF;
	mux = (s3c240x_pwm_regs[1] >> mux_shift[timer]) & 0x0F;
	freq = s3c240x_get_pclk( MPLLCON) / (prescaler + 1) / mux_table[mux];
	cnt = BITS( regs[0], 15, 0);
	if (timer != 4)
	{
		cmp = BITS( regs[1], 15, 0);
		auto_reload = BIT( s3c240x_pwm_regs[2], tcon_shift[timer] + 3);
	}
	else
	{
		cmp = 0;
		auto_reload = BIT( s3c240x_pwm_regs[2], tcon_shift[timer] + 2);
	}
	hz = freq / (cnt - cmp + 1);
	verboselog( machine, 5, "PWM %d - FCLK=%d HCLK=%d PCLK=%d prescaler=%d div=%d freq=%f cnt=%d cmp=%d auto_reload=%d hz=%f\n", timer, s3c240x_get_fclk( MPLLCON), s3c240x_get_hclk( MPLLCON), s3c240x_get_pclk( MPLLCON), prescaler, mux_table[mux], freq, cnt, cmp, auto_reload, hz);
	if (auto_reload)
	{
		timer_adjust_periodic( s3c240x_pwm_timer[timer], ATTOTIME_IN_HZ( hz), timer, ATTOTIME_IN_HZ( hz));
	}
	else
	{
		timer_adjust_oneshot( s3c240x_pwm_timer[timer], ATTOTIME_IN_HZ( hz), timer);
	}
}

static void s3c240x_pwm_stop( running_machine *machine, int timer)
{
	verboselog( machine, 1, "PWM %d stop\n", timer);
	timer_adjust_oneshot( s3c240x_pwm_timer[timer], attotime_never, 0);
}

static void s3c240x_pwm_recalc( running_machine *machine, int timer)
{
	const int tcon_shift[] = { 0, 8, 12, 16, 20};
	if (s3c240x_pwm_regs[2] & (1 << tcon_shift[timer]))
	{
		s3c240x_pwm_start( machine, timer);
	}
	else
	{
		s3c240x_pwm_stop( machine, timer);
	}
}

static WRITE32_HANDLER( s3c240x_pwm_w )
{
	running_machine *machine = space->machine;
	UINT32 old_value = s3c240x_pwm_regs[offset];
	verboselog( machine, 9, "(PWM) %08X <- %08X (PC %08X)\n", 0x15100000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_pwm_regs[offset]);
	switch (offset)
	{
		// TCON
		case 0x08 / 4 :
		{
			if ((data & 1) != (old_value & 1))
			{
				s3c240x_pwm_recalc( machine, 0);
			}
			if ((data & 0x100) != (old_value & 0x100))
			{
				s3c240x_pwm_recalc( machine, 1);
			}
			if ((data & 0x1000) != (old_value & 0x1000))
			{
				s3c240x_pwm_recalc( machine, 2);
			}
			if ((data & 0x10000) != (old_value & 0x10000))
			{
				s3c240x_pwm_recalc( machine, 3);
			}
			if ((data & 0x100000) != (old_value & 0x100000))
			{
				s3c240x_pwm_recalc( machine, 4);
			}
		}
	}
}

static TIMER_CALLBACK( s3c240x_pwm_timer_exp )
{
	int ch = param;
	const int ch_int[] = { INT_TIMER0, INT_TIMER1, INT_TIMER2, INT_TIMER3, INT_TIMER4 };
	verboselog( machine, 2, "PWM %d timer callback\n", ch);
	s3c240x_request_irq( machine, ch_int[ch]);
}

// DMA

static emu_timer *s3c240x_dma_timer[4];
static UINT32 s3c240x_dma_regs[0x7c/4];

static void s3c240x_dma_reload( running_machine *machine, int dma)
{
	UINT32 *regs = &s3c240x_dma_regs[dma<<3];
	regs[3] = (regs[3] & ~0x000FFFFF) | BITS( regs[2], 19, 0);
	regs[4] = (regs[4] & ~0x1FFFFFFF) | BITS( regs[0], 28, 0);
	regs[5] = (regs[5] & ~0x1FFFFFFF) | BITS( regs[1], 28, 0);
}

static void s3c240x_dma_trigger( running_machine *machine, int dma)
{
	UINT32 *regs = &s3c240x_dma_regs[dma<<3];
	UINT32 curr_tc, curr_src, curr_dst;
  const address_space *space = cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int dsz, inc_src, inc_dst, servmode;
	const UINT32 ch_int[] = { INT_DMA0, INT_DMA1, INT_DMA2, INT_DMA3};
	verboselog( machine, 5, "DMA %d trigger\n", dma);
	curr_tc = BITS( regs[3], 19, 0);
	curr_src = BITS( regs[4], 28, 0);
	curr_dst = BITS( regs[5], 28, 0);
	dsz = BITS( regs[2], 21, 20);
	servmode = BIT( regs[2], 26);
	inc_src = BIT( regs[0], 29);
	inc_dst = BIT( regs[1], 29);
	verboselog( machine, 5, "DMA %d - curr_src %08X curr_dst %08X curr_tc %d dsz %d\n", dma, curr_src, curr_dst, curr_tc, dsz);
	while (curr_tc > 0)
	{
		curr_tc--;
		switch (dsz)
		{
			case 0 : memory_write_byte( space, curr_dst, memory_read_byte( space, curr_src)); break;
			case 1 : memory_write_word( space, curr_dst, memory_read_word( space, curr_src)); break;
			case 2 : memory_write_dword( space, curr_dst, memory_read_dword( space, curr_src)); break;
		}
		if (inc_src == 0) curr_src += (1 << dsz);
		if (inc_dst == 0) curr_dst += (1 << dsz);
		if (servmode == 0) break;
	}
	// update curr_src
	regs[4] = (regs[4] & ~0x1FFFFFFF) | curr_src;
	// update curr_dst
	regs[5] = (regs[5] & ~0x1FFFFFFF) | curr_dst;
	// update curr_tc
	regs[3] = (regs[3] & ~0x000FFFFF) | curr_tc;
	// ...
	if (curr_tc == 0)
	{
		int _int, reload;
		reload = BIT( regs[2], 22);
		if (!reload)
		{
			s3c240x_dma_reload( machine, dma);
		}
		else
		{
			regs[6] &= ~(1 << 1); // clear on/off
		}
		_int = BIT( regs[2], 28);
		if (_int)
		{
			s3c240x_request_irq( machine, ch_int[dma]);
		}
	}
}

static void s3c240x_dma_start( running_machine *machine, int dma)
{
	UINT32 addr_src, addr_dst, tc;
	UINT32 *regs = &s3c240x_dma_regs[dma<<3];
	UINT32 dsz, tsz, reload;
	int inc_src, inc_dst, _int, servmode, swhwsel, hwsrcsel;
	verboselog( machine, 1, "DMA %d start\n", dma);
	addr_src = BITS( regs[0], 28, 0);
	addr_dst = BITS( regs[1], 28, 0);
	tc = BITS( regs[2], 19, 0);
	inc_src = BIT( regs[0], 29);
	inc_dst = BIT( regs[1], 29);
	tsz = BIT( regs[2], 27);
	_int = BIT( regs[2], 28);
	servmode = BIT( regs[2], 26);
	hwsrcsel = BITS( regs[2], 25, 24);
	swhwsel = BIT( regs[2], 23);
	reload = BIT( regs[2], 22);
	dsz = BITS( regs[2], 21, 20);
	verboselog( machine, 5, "DMA %d - addr_src %08X inc_src %d addr_dst %08X inc_dst %d int %d tsz %d servmode %d hwsrcsel %d swhwsel %d reload %d dsz %d tc %d\n", dma, addr_src, inc_src, addr_dst, inc_dst, _int, tsz, servmode, hwsrcsel, swhwsel, reload, dsz, tc);
	verboselog( machine, 5, "DMA %d - copy %08X bytes from %08X (%s) to %08X (%s)\n", dma, tc << dsz, addr_src, inc_src ? "fix" : "inc", addr_dst, inc_dst ? "fix" : "inc");
	s3c240x_dma_reload( machine, dma);
	if (swhwsel == 0)
	{
		s3c240x_dma_trigger( machine, dma);
	}
}

static void s3c240x_dma_stop( running_machine *machine, int dma)
{
	verboselog( machine, 1, "DMA %d stop\n", dma);
}

static void s3c240x_dma_recalc( running_machine *machine, int dma)
{
	if (s3c240x_dma_regs[(dma<<3)+6] & 2)
	{
		s3c240x_dma_start( machine, dma);
	}
	else
	{
		s3c240x_dma_stop( machine, dma);
	}
}

static READ32_HANDLER( s3c240x_dma_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_dma_regs[offset];
	verboselog( machine, 9, "(DMA) %08X -> %08X (PC %08X)\n", 0x14600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_dma_w )
{
	running_machine *machine = space->machine;
	UINT32 old_value = s3c240x_dma_regs[offset];
	verboselog( machine, 9, "(DMA) %08X <- %08X (PC %08X)\n", 0x14600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_dma_regs[offset]);
	switch (offset)
	{
		// DCON0
		case 0x08 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x18/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG0
		case 0x18 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( machine, 0);
		}
		break;
		// DCON1
		case 0x28 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x38/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG1
		case 0x38 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( machine, 1);
		}
		break;
		// DCON2
		case 0x48 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x58/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG2
		case 0x58 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( machine, 2);
		}
		break;
		// DCON3
		case 0x68 / 4 :
		{
			if (((data >> 22) & 1) != 0) // reload
			{
				s3c240x_dma_regs[0x78/4] &= ~(1 << 1); // clear on/off
			}
		}
		break;
		// DMASKTRIG3
		case 0x78 / 4 :
		{
			if ((old_value & 2) != (data & 2)) s3c240x_dma_recalc( machine, 3);
		}
		break;
	}
}

static TIMER_CALLBACK( s3c240x_dma_timer_exp )
{
	int ch = param;
	verboselog( machine, 2, "DMA %d timer callback\n", ch);
}

// SMARTMEDIA

static struct {
	int add_latch;
	int chip;
	int cmd_latch;
	int do_read;
	int do_write;
	int read;
	int wp;
	int busy;
	UINT8 datarx;
	UINT8 datatx;
} smc;

static void smc_reset( running_machine *machine)
{
	verboselog( machine, 5, "smc_reset\n");
	smc.add_latch = 0;
	smc.chip = 0;
	smc.cmd_latch = 0;
	smc.do_read = 0;
	smc.do_write = 0;
	smc.read = 0;
	smc.wp = 0;
	smc.busy = 0;
}

static void smc_init( running_machine *machine)
{
	verboselog( machine, 5, "smc_init\n");
	smc_reset( machine);
}

static UINT8 smc_read( running_machine *machine)
{
	const device_config *smartmedia = devtag_get_device( machine, "smartmedia");
	UINT8 data;
	data = smartmedia_data_r( smartmedia);
	verboselog( machine, 5, "smc_read %08X\n", data);
	return data;
}

static void smc_write( running_machine *machine, UINT8 data)
{
	verboselog( machine, 5, "smc_write %08X\n", data);
	if ((smc.chip) && (!smc.read))
	{
		const device_config *smartmedia = devtag_get_device( machine, "smartmedia");
		if (smc.cmd_latch)
		{
			verboselog( machine, 5, "smartmedia_command_w %08X\n", data);
			smartmedia_command_w( smartmedia, data);
		}
		else if (smc.add_latch)
		{
			verboselog( machine, 5, "smartmedia_address_w %08X\n", data);
			smartmedia_address_w( smartmedia, data);
		}
		else
		{
			verboselog( machine, 5, "smartmedia_data_w %08X\n", data);
 			smartmedia_data_w( smartmedia, data);
		}
	}
}

static void smc_update( running_machine *machine)
{
	if (!smc.chip)
	{
		smc_reset( machine);
	}
	else
	{
		if ((smc.do_write) && (!smc.read))
		{
			smc_write( machine, smc.datatx);
		}
		else if ((!smc.do_write) && (smc.do_read) && (smc.read) && (!smc.cmd_latch) && (!smc.add_latch))
		{
			smc.datarx = smc_read( machine);
		}
	}
}

// I2S

#define I2S_L3C ( 1 )
#define I2S_L3M ( 2 )
#define I2S_L3D ( 3 )

static struct {
	int l3d;
	int l3m;
	int l3c;
} i2s;

static void i2s_reset( running_machine *machine)
{
	verboselog( machine, 5, "i2s_reset\n");
	i2s.l3d = 0;
	i2s.l3m = 0;
	i2s.l3c = 0;
}

static void i2s_init( running_machine *machine)
{
	verboselog( machine, 5, "i2s_init\n");
	i2s_reset( machine);
}

static void i2s_write( running_machine *machine, int line, int data)
{
	switch (line)
	{
		case I2S_L3C :
		{
			if (data != i2s.l3c)
			{
				verboselog( machine, 5, "I2S L3C %d\n", data);
				i2s.l3c = data;
			}
		}
		break;
		case I2S_L3M :
		{
			if (data != i2s.l3m)
			{
				verboselog( machine, 5, "I2S L3M %d\n", data);
				i2s.l3m = data;
			}
		}
		break;
		case I2S_L3D :
		{
			if (data != i2s.l3d)
			{
				verboselog( machine, 5, "I2S L3D %d\n", data);
				i2s.l3d = data;
			}
		}
		break;
	}
}

// I/O PORT

static UINT32 s3c240x_gpio[0x60/4];

static READ32_HANDLER( s3c240x_gpio_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_gpio[offset];
	switch (offset)
	{
		// PBCON
		case 0x08 / 4 :
		{
			// smartmedia
			data = (data & ~0x00000001);
			if (!smc.read) data = data | 0x00000001;
		}
		break;
		// PBDAT
		case 0x0C / 4 :
		{
			// smartmedia
			data = (data & ~0x000000FF) | (smc.datarx & 0xFF);
			// buttons
			data = (data & ~0x0000FF00) | (input_port_read( machine, "IN0") & 0x0000FF00);
		}
		break;
		// PDDAT
		case 0x24 / 4 :
		{
			const device_config *smartmedia = devtag_get_device( machine, "smartmedia");
			// smartmedia
			data = (data & ~0x000003C0);
			if (!smc.busy) data = data | 0x00000200;
			if (!smc.do_read) data = data | 0x00000100;
			if (!smc.chip) data = data | 0x00000080;
			if (!smartmedia_protected( smartmedia)) data = data | 0x00000040;
		}
		break;
		// PEDAT
		case 0x30 / 4 :
		{
			const device_config *smartmedia = devtag_get_device( machine, "smartmedia");
			// smartmedia
			data = (data & ~0x0000003C);
			if (smc.cmd_latch) data = data | 0x00000020;
			if (smc.add_latch) data = data | 0x00000010;
			if (!smc.do_write) data = data | 0x00000008;
			if (!smartmedia_present( smartmedia)) data = data | 0x00000004;
			// buttons
			data = (data & ~0x000000C0) | (input_port_read( machine, "IN1") & 0x000000C0);
		}
		break;
	}
	verboselog( machine, 9, "(GPIO) %08X -> %08X (PC %08X)\n", 0x15600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_gpio_w )
{
	running_machine *machine = space->machine;
//	UINT32 old_value = s3c240x_gpio_regs[offset];
	COMBINE_DATA(&s3c240x_gpio[offset]);
	verboselog( machine, 9, "(GPIO) %08X <- %08X (PC %08X)\n", 0x15600000 + (offset << 2), data, cpu_get_pc( space->cpu));
	switch (offset)
	{
		// PBCON
		case 0x08 / 4 :
		{
			// smartmedia
			smc.read = ((data & 0x00000001) == 0);
			smc_update( machine);
		}
		break;
		// PBDAT
		case 0x0C / 4 :
		{
			// smartmedia
			smc.datatx = data & 0xFF;
		}
		break;
		// PDDAT
		case 0x24 / 4 :
		{
			// smartmedia
			smc.do_read = ((data & 0x00000100) == 0);
			smc.chip = ((data & 0x00000080) == 0);
			smc.wp = ((data & 0x00000040) == 0);
			smc_update( machine);
		}
		break;
		// PEDAT
		case 0x30 / 4 :
		{
			// smartmedia
			smc.cmd_latch = ((data & 0x00000020) != 0);
			smc.add_latch = ((data & 0x00000010) != 0);
			smc.do_write  = ((data & 0x00000008) == 0);
			smc_update( machine);
			// sound
			i2s_write( machine, I2S_L3D, (data & 0x00000800) ? 1 : 0);
			i2s_write( machine, I2S_L3M, (data & 0x00000400) ? 1 : 0);
			i2s_write( machine, I2S_L3C, (data & 0x00000200) ? 1 : 0);
		}
		break;
/*
		// PGDAT
		case 0x48 / 4 :
		{
			int i2ssdo;
			i2ssdo = BIT( data, 3);
		}
		break;
*/
	}
}

// MEMORY CONTROLLER

static UINT32 s3c240x_memcon_regs[0x34/4];

static READ32_HANDLER( s3c240x_memcon_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_memcon_regs[offset];
	verboselog( machine, 9, "(MEMCON) %08X -> %08X (PC %08X)\n", 0x14000000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_memcon_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(MEMCON) %08X <- %08X (PC %08X)\n", 0x14000000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_memcon_regs[offset]);
}

// USB HOST CONTROLLER

static UINT32 s3c240x_usb_host_regs[0x5C/4];

static READ32_HANDLER( s3c240x_usb_host_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_usb_host_regs[offset];
	verboselog( machine, 9, "(USB H) %08X -> %08X (PC %08X)\n", 0x14200000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_usb_host_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(USB H) %08X <- %08X (PC %08X)\n", 0x14200000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_usb_host_regs[offset]);
}

// UART 0

static UINT32 s3c240x_uart_0_regs[0x2C/4];

static READ32_HANDLER( s3c240x_uart_0_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_uart_0_regs[offset];
	switch (offset)
	{
		// UTRSTAT0
		case 0x10 / 4 :
		{
			data = (data & ~0x00000006) | 0x00000004 | 0x00000002; // [bit 2] Transmitter empty / [bit 1] Transmit buffer empty
		}
		break;
	}
	verboselog( machine, 9, "(UART 0) %08X -> %08X (PC %08X)\n", 0x15000000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_uart_0_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(UART 0) %08X <- %08X (PC %08X)\n", 0x15000000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_uart_0_regs[offset]);
}

// UART 1

static UINT32 s3c240x_uart_1_regs[0x2C/4];

static READ32_HANDLER( s3c240x_uart_1_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_uart_1_regs[offset];
	switch (offset)
	{
		// UTRSTAT1
		case 0x10 / 4 :
		{
			data = (data & ~0x00000006) | 0x00000004 | 0x00000002; // [bit 2] Transmitter empty / [bit 1] Transmit buffer empty
		}
		break;
	}
	verboselog( machine, 9, "(UART 1) %08X -> %08X (PC %08X)\n", 0x15004000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_uart_1_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(UART 1) %08X <- %08X (PC %08X)\n", 0x15004000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_uart_1_regs[offset]);
}

// USB DEVICE

static UINT32 s3c240x_usb_device_regs[0xBC/4];

static READ32_HANDLER( s3c240x_usb_device_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_usb_device_regs[offset];
	verboselog( machine, 9, "(USB D) %08X -> %08X (PC %08X)\n", 0x15200140 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_usb_device_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(USB D) %08X <- %08X (PC %08X)\n", 0x15200140 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_usb_device_regs[offset]);
}

// WATCHDOG TIMER

static UINT32 s3c240x_watchdog_regs[0x0C/4];

static READ32_HANDLER( s3c240x_watchdog_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_watchdog_regs[offset];
	verboselog( machine, 9, "(WDOG) %08X -> %08X (PC %08X)\n", 0x15300000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_watchdog_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(WDOG) %08X <- %08X (PC %08X)\n", 0x15300000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_watchdog_regs[offset]);
}

// EEPROM

static UINT8 eeprom_read( running_machine *machine, UINT16 address)
{
	UINT8 data;
	data = eeprom_data[address];
	verboselog( machine, 5, "EEPROM %04X -> %02X\n", address, data);
	return data;
}

static void eeprom_write( running_machine *machine, UINT16 address, UINT8 data)
{
	verboselog( machine, 5, "EEPROM %04X <- %02X\n", address, data);
	eeprom_data[address] = data;
}

// IIC

static struct
{
	UINT8 data[4];
	int data_index;
	UINT16 address;
} s3c240x_iic;

static emu_timer *s3c240x_iic_timer;
static UINT32 s3c240x_iic_regs[0x10/4];

/*
static UINT8 i2cmem_read_byte( running_machine *machine, int last)
{
	UINT8 data = 0;
	int i;
	i2cmem_write( machine, 0, I2CMEM_SDA, 1);
	for (i = 0; i < 8; i++)
	{
		i2cmem_write( machine, 0, I2CMEM_SCL, 1);
    data = (data << 1) + (i2cmem_read( machine, 0, I2CMEM_SDA) ? 1 : 0);
		i2cmem_write( machine, 0, I2CMEM_SCL, 0);
	}
	i2cmem_write( machine, 0, I2CMEM_SDA, last);
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
	return data;
}
*/

/*
static void i2cmem_write_byte( running_machine *machine, UINT8 data)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		i2cmem_write( machine, 0, I2CMEM_SDA, (data & 0x80) ? 1 : 0);
		data = data << 1;
		i2cmem_write( machine, 0, I2CMEM_SCL, 1);
		i2cmem_write( machine, 0, I2CMEM_SCL, 0);
	}
	i2cmem_write( machine, 0, I2CMEM_SDA, 1); // ack bit
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
}
*/

/*
static void i2cmem_start( running_machine *machine)
{
	i2cmem_write( machine, 0, I2CMEM_SDA, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SDA, 0);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
}
*/

/*
static void i2cmem_stop( running_machine *machine)
{
	i2cmem_write( machine, 0, I2CMEM_SDA, 0);
	i2cmem_write( machine, 0, I2CMEM_SCL, 1);
	i2cmem_write( machine, 0, I2CMEM_SDA, 1);
	i2cmem_write( machine, 0, I2CMEM_SCL, 0);
}
*/

static void iic_start( running_machine *machine)
{
	verboselog( machine, 1, "IIC start\n");
	s3c240x_iic.data_index = 0;
	timer_adjust_oneshot( s3c240x_iic_timer, ATTOTIME_IN_MSEC( 1), 0);
}

static void iic_stop( running_machine *machine)
{
	verboselog( machine, 1, "IIC stop\n");
	timer_adjust_oneshot( s3c240x_iic_timer, attotime_never, 0);
}

static void iic_resume( running_machine *machine)
{
	verboselog( machine, 1, "IIC resume\n");
	timer_adjust_oneshot( s3c240x_iic_timer, ATTOTIME_IN_MSEC( 1), 0);
}

static READ32_HANDLER( s3c240x_iic_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_iic_regs[offset];
	switch (offset)
	{
		// IICSTAT
		case 0x04 / 4 :
		{
			data = data & ~0x0000000F;
		}
		break;
	}
	verboselog( machine, 9, "(IIC) %08X -> %08X (PC %08X)\n", 0x15400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_iic_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(IIC) %08X <- %08X (PC %08X)\n", 0x15400000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_iic_regs[offset]);
	switch (offset)
	{
		// ADDR_IICCON
		case 0x00 / 4 :
		{
			int interrupt_pending_flag;
//			const int div_table[] = { 16, 512};
//			int enable_interrupt, transmit_clock_value, tx_clock_source_selection
//			double clock;
//			transmit_clock_value = (data >> 0) & 0xF;
//			tx_clock_source_selection = (data >> 6) & 1;
//			enable_interrupt = (data >> 5) & 1;
//			clock = (double)(s3c240x_get_pclk( MPLLCON) / div_table[tx_clock_source_selection] / (transmit_clock_value + 1));
			interrupt_pending_flag = BIT( data, 4);
			if (interrupt_pending_flag == 0)
			{
				int start_stop_condition;
				start_stop_condition = BIT( s3c240x_iic_regs[1], 5);
				if (start_stop_condition != 0)
				{
					iic_resume( machine);
				}
			}
		}
		break;
		// IICSTAT
		case 0x04 / 4 :
		{
			int start_stop_condition;
			start_stop_condition = BIT( data, 5);
			if (start_stop_condition != 0)
			{
				iic_start( machine);
			}
			else
			{
				iic_stop( machine);
			}
		}
		break;
	}
}

static TIMER_CALLBACK( s3c240x_iic_timer_exp )
{
	int enable_interrupt, mode_selection;
	verboselog( machine, 2, "IIC timer callback\n");
	mode_selection = BITS( s3c240x_iic_regs[1], 7, 6);
	switch (mode_selection)
	{
		// master receive mode
		case 2 :
		{
			if (s3c240x_iic.data_index == 0)
			{
				UINT8 data_shift = s3c240x_iic_regs[3] & 0xFF;
				verboselog( machine, 5, "IIC write %02X\n", data_shift);
			}
			else
			{
				UINT8 data_shift = eeprom_read( machine, s3c240x_iic.address);
				verboselog( machine, 5, "IIC read %02X\n", data_shift);
				s3c240x_iic_regs[3] = (s3c240x_iic_regs[3] & ~0xFF) | data_shift;
			}
			s3c240x_iic.data_index++;
		}
		break;
		// master transmit mode
		case 3 :
		{
			UINT8 data_shift = s3c240x_iic_regs[3] & 0xFF;
			verboselog( machine, 5, "IIC write %02X\n", data_shift);
			s3c240x_iic.data[s3c240x_iic.data_index++] = data_shift;
			if (s3c240x_iic.data_index == 3)
			{
				s3c240x_iic.address = (s3c240x_iic.data[1] << 8) | s3c240x_iic.data[2];
			}
			if ((s3c240x_iic.data_index == 4) && (s3c240x_iic.data[0] == 0xA0))
			{
				eeprom_write( machine, s3c240x_iic.address, data_shift);
			}
		}
		break;
	}
	enable_interrupt = BIT( s3c240x_iic_regs[0], 5);
	if (enable_interrupt)
	{
		s3c240x_request_irq( machine, INT_IIC);
	}
}

// IIS

static struct 
{
	UINT16 fifo[16/2];
	int fifo_index;
} s3c240x_iis;

static emu_timer *s3c240x_iis_timer;
static UINT32 s3c240x_iis_regs[0x14/4];

static void s3c240x_iis_start( running_machine *machine)
{
	const UINT32 codeclk_table[] = { 256, 384};
	double freq;
	int prescaler_enable, prescaler_control_a, prescaler_control_b, codeclk;
	verboselog( machine, 1, "IIS start\n");
	prescaler_enable = BIT( s3c240x_iis_regs[0], 1);
	prescaler_control_a = BITS( s3c240x_iis_regs[2], 9, 5);
	prescaler_control_b = BITS( s3c240x_iis_regs[2], 4, 0);
	codeclk = BIT( s3c240x_iis_regs[1], 2);
	freq = (double)(s3c240x_get_pclk( MPLLCON) / (prescaler_control_a + 1) / codeclk_table[codeclk]) * 2; // why do I have to multiply by two?
	verboselog( machine, 5, "IIS - pclk %d psc_enable %d psc_a %d psc_b %d codeclk %d freq %f\n", s3c240x_get_pclk( MPLLCON), prescaler_enable, prescaler_control_a, prescaler_control_b, codeclk_table[codeclk], freq);
	timer_adjust_periodic( s3c240x_iis_timer, ATTOTIME_IN_HZ( freq), 0, ATTOTIME_IN_HZ( freq));
}

static void s3c240x_iis_stop( running_machine *machine)
{
	verboselog( machine, 1, "IIS stop\n");
	timer_adjust_oneshot( s3c240x_iis_timer, attotime_never, 0);
}

static void s3c240x_iis_recalc( running_machine *machine)
{
	if (s3c240x_iis_regs[0] & 1)
	{
		s3c240x_iis_start( machine);
	}
	else
	{
		s3c240x_iis_stop( machine);
	}
}

static READ32_HANDLER( s3c240x_iis_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_iis_regs[offset];
/*
	switch (offset)
	{
		// IISCON
		case 0x00 / 4 :
		{
			//data = data & ~1; // for mp3 player
		}
		break;
	}
*/
	verboselog( machine, 9, "(IIS) %08X -> %08X (PC %08X)\n", 0x15508000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_iis_w )
{
	running_machine *machine = space->machine;
	UINT32 old_value = s3c240x_iis_regs[offset];
	verboselog( machine, 9, "(IIS) %08X <- %08X (PC %08X)\n", 0x15508000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_iis_regs[offset]);
	switch (offset)
	{
		// IISCON
		case 0x00 / 4 :
		{
			if ((old_value & 1) != (data & 1)) s3c240x_iis_recalc( machine);
		}
		break;
		// IISFIF
		case 0x10 / 4 :
		{
			if (ACCESSING_BITS_16_31)
			{
				s3c240x_iis.fifo[s3c240x_iis.fifo_index++] = BITS( data, 31, 16);
			}
			if (ACCESSING_BITS_0_15)
			{
				s3c240x_iis.fifo[s3c240x_iis.fifo_index++] = BITS( data, 15, 0);
			}
			if (s3c240x_iis.fifo_index == 2)
			{
				const device_config *dac[2];
				dac[0] = devtag_get_device( machine, "dac1");
				dac[1] = devtag_get_device( machine, "dac2");
				s3c240x_iis.fifo_index = 0;
    		dac_signed_data_16_w( dac[0], s3c240x_iis.fifo[0] + 0x8000);
    		dac_signed_data_16_w( dac[1], s3c240x_iis.fifo[1] + 0x8000);
			}
		}
		break;
	}
}

static TIMER_CALLBACK( s3c240x_iis_timer_exp )
{
	UINT32 dcon;
	int hwsrcsel, swhwsel;
	verboselog( machine, 2, "IIS timer callback\n");
	dcon = s3c240x_dma_regs[0x48/4];
	hwsrcsel = BITS( dcon, 25, 24);
	swhwsel = BIT( dcon, 23);
	if ((swhwsel == 1) && (hwsrcsel == 0))
	{
		UINT32 dmasktrig;
		int on_off;
		dmasktrig = s3c240x_dma_regs[0x58/4];
		on_off = BIT( dmasktrig, 1);
		if (on_off)
		{
			s3c240x_dma_trigger( machine, 2);
		}
	}
}

// RTC

static UINT32 s3c240x_rtc_regs[0x4C/4];

static READ32_HANDLER( s3c240x_rtc_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_rtc_regs[offset];
	verboselog( machine, 9, "(RTC) %08X -> %08X (PC %08X)\n", 0x15700040 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_rtc_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(RTC) %08X <- %08X (PC %08X)\n", 0x15700040 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_rtc_regs[offset]);
}

// A/D CONVERTER

static UINT32 s3c240x_adc_regs[0x08/4];

static READ32_HANDLER( s3c240x_adc_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_adc_regs[offset];
	verboselog( machine, 9, "(ADC) %08X -> %08X (PC %08X)\n", 0x15800000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_adc_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(ADC) %08X <- %08X (PC %08X)\n", 0x15800000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_adc_regs[offset]);
}

// SPI

static UINT32 s3c240x_spi_regs[0x18/4];

static READ32_HANDLER( s3c240x_spi_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_spi_regs[offset];
	verboselog( machine, 9, "(SPI) %08X -> %08X (PC %08X)\n", 0x15900000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_spi_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(SPI) %08X <- %08X (PC %08X)\n", 0x15900000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_spi_regs[offset]);
}

// MMC INTERFACE

static UINT32 s3c240x_mmc_regs[0x40/4];

static READ32_HANDLER( s3c240x_mmc_r )
{
	running_machine *machine = space->machine;
	UINT32 data = s3c240x_mmc_regs[offset];
	verboselog( machine, 9, "(MMC) %08X -> %08X (PC %08X)\n", 0x15A00000 + (offset << 2), data, cpu_get_pc( space->cpu));
	return data;
}

static WRITE32_HANDLER( s3c240x_mmc_w )
{
	running_machine *machine = space->machine;
	verboselog( machine, 9, "(MMC) %08X <- %08X (PC %08X)\n", 0x15A00000 + (offset << 2), data, cpu_get_pc( space->cpu));
	COMBINE_DATA(&s3c240x_mmc_regs[offset]);
}

// ...

static void s3c240x_machine_start( running_machine *machine)
{
	s3c240x_pwm_timer[0] = timer_alloc( machine, s3c240x_pwm_timer_exp, (void *)(FPTR)0);
	s3c240x_pwm_timer[1] = timer_alloc( machine, s3c240x_pwm_timer_exp, (void *)(FPTR)1);
	s3c240x_pwm_timer[2] = timer_alloc( machine, s3c240x_pwm_timer_exp, (void *)(FPTR)2);
	s3c240x_pwm_timer[3] = timer_alloc( machine, s3c240x_pwm_timer_exp, (void *)(FPTR)3);
	s3c240x_pwm_timer[4] = timer_alloc( machine, s3c240x_pwm_timer_exp, (void *)(FPTR)4);
	s3c240x_dma_timer[0] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)0);
	s3c240x_dma_timer[1] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)1);
	s3c240x_dma_timer[2] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)2);
	s3c240x_dma_timer[3] = timer_alloc( machine, s3c240x_dma_timer_exp, (void *)(FPTR)3);
	s3c240x_iic_timer = timer_alloc( machine, s3c240x_iic_timer_exp, (void *)(FPTR)0);
	s3c240x_iis_timer = timer_alloc( machine, s3c240x_iis_timer_exp, (void *)(FPTR)0);
	s3c240x_lcd_timer = timer_alloc( machine, s3c240x_lcd_timer_exp, (void *)(FPTR)0);
	eeprom_data = auto_alloc_array( machine, UINT8, 0x2000);
	smc_init( machine);
	i2s_init( machine);
}

static void s3c240x_machine_reset( running_machine *machine)
{
	smc_reset( machine);
	i2s_reset( machine);
	s3c240x_iis.fifo_index = 0;
	s3c240x_iic.data_index = 0;
}

static NVRAM_HANDLER( gp32 )
{
	if (read_or_write)
	{
		if (file)
		{
			mame_fwrite( file, eeprom_data, 0x2000);
		}
	}
	else
	{
		if (file)
		{
			mame_fread( file, eeprom_data, 0x2000);
		}
		else
		{
			memset( eeprom_data, 0xFF, 0x2000);
		}
	}
}

static ADDRESS_MAP_START( gp32_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM
	AM_RANGE(0x0c000000, 0x0c7fffff) AM_RAM AM_BASE(&s3c240x_ram)
	AM_RANGE(0x14000000, 0x1400003b) AM_READWRITE(s3c240x_memcon_r, s3c240x_memcon_w)
	AM_RANGE(0x14200000, 0x1420005b) AM_READWRITE(s3c240x_usb_host_r, s3c240x_usb_host_w)
	AM_RANGE(0x14400000, 0x14400017) AM_READWRITE(s3c240x_irq_r, s3c240x_irq_w)
	AM_RANGE(0x14600000, 0x1460007b) AM_READWRITE(s3c240x_dma_r, s3c240x_dma_w)
	AM_RANGE(0x14800000, 0x14800017) AM_READWRITE(s3c240x_clkpow_r, s3c240x_clkpow_w)
	AM_RANGE(0x14a00000, 0x14a003ff) AM_READWRITE(s3c240x_lcd_r, s3c240x_lcd_w)
	AM_RANGE(0x14a00400, 0x14a007ff) AM_READWRITE(s3c240x_lcd_palette_r, s3c240x_lcd_palette_w)
	AM_RANGE(0x15000000, 0x1500002b) AM_READWRITE(s3c240x_uart_0_r, s3c240x_uart_0_w)
	AM_RANGE(0x15004000, 0x1500402b) AM_READWRITE(s3c240x_uart_1_r, s3c240x_uart_1_w)
	AM_RANGE(0x15100000, 0x15100043) AM_READWRITE(s3c240x_pwm_r, s3c240x_pwm_w)
	AM_RANGE(0x15200140, 0x152001fb) AM_READWRITE(s3c240x_usb_device_r, s3c240x_usb_device_w)
	AM_RANGE(0x15300000, 0x1530000b) AM_READWRITE(s3c240x_watchdog_r, s3c240x_watchdog_w)
	AM_RANGE(0x15400000, 0x1540000f) AM_READWRITE(s3c240x_iic_r, s3c240x_iic_w)
	AM_RANGE(0x15508000, 0x15508013) AM_READWRITE(s3c240x_iis_r, s3c240x_iis_w)
	AM_RANGE(0x15600000, 0x1560005b) AM_READWRITE(s3c240x_gpio_r, s3c240x_gpio_w)
	AM_RANGE(0x15700040, 0x1570008b) AM_READWRITE(s3c240x_rtc_r, s3c240x_rtc_w)
	AM_RANGE(0x15800000, 0x15800007) AM_READWRITE(s3c240x_adc_r, s3c240x_adc_w)
	AM_RANGE(0x15900000, 0x15900017) AM_READWRITE(s3c240x_spi_r, s3c240x_spi_w)
	AM_RANGE(0x15a00000, 0x15a0003f) AM_READWRITE(s3c240x_mmc_r, s3c240x_mmc_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( gp32 )
	PORT_START("IN0")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("R") PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("L") PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("B") PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("A") PORT_PLAYER(1)
	PORT_START("IN1")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SELECT ) PORT_NAME("SELECT") PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START ) PORT_NAME("START") PORT_PLAYER(1)
INPUT_PORTS_END

static MACHINE_START( gp32 )
{
	s3c240x_machine_start(machine);
}

static MACHINE_RESET( gp32 )
{
	s3c240x_machine_reset(machine);
}

static MACHINE_DRIVER_START( gp32 )
	MDRV_CPU_ADD("maincpu", ARM9, 40000000)
	MDRV_CPU_PROGRAM_MAP(gp32_map)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(240, 320)
	MDRV_SCREEN_VISIBLE_AREA(0, 239, 0, 319)
	/* 320x240 is 4:3 but ROT270 causes an aspect ratio of 3:4 by default */
	MDRV_DEFAULT_LAYOUT(layout_lcd_rot)

	MDRV_VIDEO_START(gp32)
	MDRV_VIDEO_UPDATE(gp32)

	MDRV_MACHINE_START(gp32)
	MDRV_MACHINE_RESET(gp32)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("dac1", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MDRV_SOUND_ADD("dac2", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)

	MDRV_NVRAM_HANDLER(gp32)

	MDRV_SMARTMEDIA_ADD("smartmedia")
MACHINE_DRIVER_END

ROM_START( gp32 )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_SYSTEM_BIOS( 0, "157e", "Firmware 1.5.7 (English)" )
	ROMX_LOAD( "gp32157e.bin", 0x000000, 0x080000, CRC(b1e35643) SHA1(1566bc2a27980602e9eb501cf8b2d62939bfd1e5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "100k", "Firmware 1.0.0 (Korean)" )
	ROMX_LOAD( "gp32100k.bin", 0x000000, 0x080000, CRC(d9925ac9) SHA1(3604d0d7210ed72eddd3e3e0c108f1102508423c), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "156k", "Firmware 1.5.6 (Korean)" )
	ROMX_LOAD( "gp32156k.bin", 0x000000, 0x080000, CRC(667fb1c8) SHA1(d179ab8e96411272b6a1d683e59da752067f9da8), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "166m", "Firmware 1.6.6 (European)" )
	ROMX_LOAD( "gp32166m.bin", 0x000000, 0x080000, CRC(4548a840) SHA1(1ad0cab0af28fb45c182e5e8c87ead2aaa4fffe1), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "mfv2", "Mr. Spiv Multi Firmware V2" )
	ROMX_LOAD( "gp32mfv2.bin", 0x000000, 0x080000, CRC(7ddaaaeb) SHA1(5a85278f721beb3b00125db5c912d1dc552c5897), ROM_BIOS(5) )
//	ROM_SYSTEM_BIOS( 5, "test", "test" )
//	ROMX_LOAD( "test.bin", 0x000000, 0x080000, CRC(00000000) SHA1(0000000000000000000000000000000000000000), ROM_BIOS(6) )
ROM_END

CONS(2001, gp32, 0, 0, gp32, gp32, 0, 0, "Game Park", "GP32", ROT270|GAME_NOT_WORKING|GAME_NO_SOUND)
