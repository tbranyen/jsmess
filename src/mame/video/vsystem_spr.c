// Video System Sprites
// todo:
//  unify these functions (secondary stage lookup differs between games, use callback)

//  according to gstriker this is probably the Fujitsu CG10103

// Aero Fighters (newer hardware)
// Quiz & Variety Sukusuku Inufuku
// 3 On 3 Dunk Madness
// Super Slams
// Formula 1 Grand Prix 2
// (Lethal) Crash Race
// Grand Striker
// V Goal Soccer
// Tecmo World Cup '94
// Tao Taido

/* old notes */

/*** Fujitsu CG10103 **********************************************/

/*
    Fujitsu CG10103 sprite generator
    --------------------------------

- Tile based
- 16x16 4bpp tiles
- Up to 7x7 in each block
- 5 bit of palette selection for the mixer
- Scaling (x/y)
- Flipping
- Indipendent sorting list
- 1 bit of pri for the mixer

Note that this chip can be connected to a VS9210 which adds a level of indirection for
tile numbers. Basically, the VS9210 indirects the tilet number through a table in its attached
memory, before accessing the ROMs.


    Sorting list format (VideoRAM offset 0)
    ---------------------------------------

de-- ---f ssss ssss

e=end of list
f=sprite present in this position
s=sprite index
d=disable sprite?


TODO:
Priorities should be right, but they probably need to be orthogonal with the mixer priorities.
Zoom factor is not correct, the scale is probably non-linear
Horizontal wrapping is just a hack. The chip probably calculates if it needs to draw the sprite at the
  normal position, or wrapped along X/Y.
Abstracts the VS9210

*/



#include "emu.h"
#include "vsystem_spr.h"






const device_type VSYSTEM_SPR = &device_creator<vsystem_spr_device>;

vsystem_spr_device::vsystem_spr_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, VSYSTEM_SPR, "vsystem_spr_device", tag, owner, clock)
{
	m_transpen = 15;
	m_pal_base = 0;
	m_xoffs = 0;
	m_yoffs = 0;
	m_pdraw = false;

	m_newtilecb =  vsystem_tile_indirection_delegate(FUNC(vsystem_spr_device::tile_callback_noindirect), this);
}

UINT32 vsystem_spr_device::tile_callback_noindirect(UINT32 tile)
{
	return tile;
}


// static
void vsystem_spr_device::set_tile_indirect_callback(device_t &device,vsystem_tile_indirection_delegate newtilecb)
{
	vsystem_spr_device &dev = downcast<vsystem_spr_device &>(device);
	dev.m_newtilecb = newtilecb;
}


// static
void vsystem_spr_device::set_offsets(device_t &device, int xoffs, int yoffs)
{
	vsystem_spr_device &dev = downcast<vsystem_spr_device &>(device);
	dev.m_xoffs = xoffs;
	dev.m_yoffs = yoffs;
}

// static
void vsystem_spr_device::set_pdraw(device_t &device, bool pdraw)
{
	vsystem_spr_device &dev = downcast<vsystem_spr_device &>(device);
	dev.m_pdraw = pdraw;
}

void vsystem_spr_device::device_start()
{

}

void vsystem_spr_device::device_reset()
{

}

void vsystem_spr_device::get_sprite_attributes(UINT16* ram)
{
	/*
        attr_start + 0x0000
        ---- ---x xxxx xxxx oy
        ---- xxx- ---- ---- ysize
        xxxx ---- ---- ---- zoomy

        attr_start + 0x0001
        ---- ---x xxxx xxxx ox
        ---- xxx- ---- ---- xsize
        xxxx ---- ---- ---- zoomx

        attr_start + 0x0002
        -x-- ---- ---- ---- flipx
        x--- ---- ---- ---- flipy
        --xx xxxx ---- ---- color
        --xx ---- ---- ---- priority? (upper color bits)
        ---- ---- ---- ---x map start (msb)

        attr_start + 0x0003
        xxxx xxxx xxxx xxxx map start (lsb)
    */

	curr_sprite.oy =    (ram[0] & 0x01ff);
	curr_sprite.ysize = (ram[0] & 0x0e00) >> 9;
	curr_sprite.zoomy = (ram[0] & 0xf000) >> 12;

	curr_sprite.ox =    (ram[1] & 0x01ff);
	curr_sprite.xsize = (ram[1] & 0x0e00) >> 9;
	curr_sprite.zoomx = (ram[1] & 0xf000) >> 12;

	curr_sprite.flipx = (ram[2] & 0x4000);
	curr_sprite.flipy = (ram[2] & 0x8000);
	curr_sprite.color = (ram[2] & 0x3f00) >> 8;
	curr_sprite.pri   = (ram[2] & 0x3000) >> 12;
	curr_sprite.map   = (ram[2] & 0x0001) << 16;

	curr_sprite.map  |= (ram[3] & 0xffff);
}


void vsystem_spr_device::common_sprite_drawgfx(int gfxrgn, running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	gfx_element *gfx = machine.gfx[gfxrgn];
	int priority_mask = 0x00;

	curr_sprite.oy += m_yoffs;
	curr_sprite.ox += m_xoffs;

	if (m_pdraw)
	{
		switch (curr_sprite.pri)
		{
			default:
			case 0:	priority_mask = 0x00; break;
			case 3:	priority_mask = 0xfe; break;
			case 2:	priority_mask = 0xfc; break;
			case 1:	priority_mask = 0xf0; break;
		}
	}

	curr_sprite.zoomx = 32 - curr_sprite.zoomx;
	curr_sprite.zoomy = 32 - curr_sprite.zoomy;

	int ystart, yend, yinc;
	
	if (!curr_sprite.flipy)	{ ystart = 0; yend = curr_sprite.ysize+1; yinc = 1; }
	else                    { ystart = curr_sprite.ysize; yend = -1; yinc = -1; }

	int ycnt = ystart;
	while (ycnt != yend)
	{
		int xstart, xend, xinc;

		if (!curr_sprite.flipx)	{ xstart = 0; xend = curr_sprite.xsize+1; xinc = 1; }
		else                    { xstart = curr_sprite.xsize; xend = -1; xinc = -1; }

		int xcnt = xstart;
		while (xcnt != xend)
		{
			int startno = m_newtilecb(curr_sprite.map++);
			if (m_pdraw)
			{
				pdrawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, curr_sprite.ox + xcnt * curr_sprite.zoomx/2,        curr_sprite.oy + ycnt * curr_sprite.zoomy/2,        curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, machine.priority_bitmap,priority_mask, m_transpen);
				pdrawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, -0x200+curr_sprite.ox + xcnt * curr_sprite.zoomx/2, curr_sprite.oy + ycnt * curr_sprite.zoomy/2,        curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, machine.priority_bitmap,priority_mask, m_transpen);		
				pdrawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, curr_sprite.ox + xcnt * curr_sprite.zoomx/2,        -0x200+curr_sprite.oy + ycnt * curr_sprite.zoomy/2, curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, machine.priority_bitmap,priority_mask, m_transpen);
				pdrawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, -0x200+curr_sprite.ox + xcnt * curr_sprite.zoomx/2, -0x200+curr_sprite.oy + ycnt * curr_sprite.zoomy/2, curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, machine.priority_bitmap,priority_mask, m_transpen);		
			}
			else
			{
				drawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, curr_sprite.ox + xcnt * curr_sprite.zoomx/2,        curr_sprite.oy + ycnt * curr_sprite.zoomy/2,        curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, m_transpen);
				drawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, -0x200+curr_sprite.ox + xcnt * curr_sprite.zoomx/2, curr_sprite.oy + ycnt * curr_sprite.zoomy/2,        curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, m_transpen);		
				drawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, curr_sprite.ox + xcnt * curr_sprite.zoomx/2,        -0x200+curr_sprite.oy + ycnt * curr_sprite.zoomy/2, curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, m_transpen);
				drawgfxzoom_transpen(bitmap, cliprect, gfx, startno, curr_sprite.color + m_pal_base, curr_sprite.flipx, curr_sprite.flipy, -0x200+curr_sprite.ox + xcnt * curr_sprite.zoomx/2, -0x200+curr_sprite.oy + ycnt * curr_sprite.zoomy/2, curr_sprite.zoomx << 11, curr_sprite.zoomy << 11, m_transpen);		
			}
			xcnt+=xinc;
		}
		ycnt+=yinc;
	}
	
}



void vsystem_spr_device::draw_sprites_inufuku( UINT16* spriteram, int spriteram_bytes, running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	int offs;
	int end = 0;

	for (offs = 0; offs < (spriteram_bytes / 16 ); offs++)
	{
		if (spriteram[offs] & 0x4000) break;
	}
	end = offs;

	for (offs = end - 1; offs >= 0; offs--)
	{
		if ((spriteram[offs] & 0x8000) == 0x0000)
		{
			int attr_start;

			attr_start = 4 * (spriteram[offs] & 0x03ff);

			get_sprite_attributes(&spriteram[attr_start]);

			curr_sprite.map &= 0x7fff;

			common_sprite_drawgfx(2, machine, bitmap, cliprect);
		}
	}
}


void vsystem_spr_device::draw_sprites_suprslam( UINT16* spriteram, int spriteram_bytes, running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	UINT16 *source = spriteram;
	UINT16 *source2 = spriteram;
	UINT16 *finish = source + 0x2000/2;

	while (source < finish)
	{
		UINT32 sprnum = source[0] & 0x03ff;
		if (source[0] == 0x4000) break;
		sprnum *= 4;
		source++;


		get_sprite_attributes(&source2[sprnum]);

		curr_sprite.map &= 0x7fff;					

		common_sprite_drawgfx(1, machine, bitmap, cliprect);
	}
}



void vsystem_spr_device::draw_sprite_taotaido( UINT16* spriteram, int spriteram_bytes, running_machine &machine, UINT16 spriteno, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	UINT16 *source = &spriteram[spriteno*4];

	get_sprite_attributes(&source[0]);

	curr_sprite.map &= 0xffff;
	curr_sprite.color &= 0x1f;

	common_sprite_drawgfx(0, machine, bitmap, cliprect);
}

void vsystem_spr_device::draw_sprites_taotaido( UINT16* spriteram, int spriteram_bytes, running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	UINT16 *source = spriteram;
	UINT16 *finish = spriteram + spriteram_bytes/2;

	while( source<finish )
	{
		if (source[0] == 0x4000) break;

		draw_sprite_taotaido(spriteram, spriteram_bytes, machine, source[0]&0x3ff, bitmap, cliprect);

		source++;
	}
}



void vsystem_spr_device::draw_sprites_crshrace(UINT16* spriteram, int spriteram_bytes, running_machine &machine, bitmap_ind16 &bitmap,const rectangle &cliprect, int flipscreen)
{
	int offs;

	offs = 0;
	while (offs < 0x0400 && (spriteram[offs] & 0x4000) == 0)
	{
		int attr_start;

		attr_start = 4 * (spriteram[offs++] & 0x03ff);

		get_sprite_attributes(&spriteram[attr_start]);

		curr_sprite.color &= 0x1f;
		curr_sprite.map &= 0x7fff;

		common_sprite_drawgfx(2, machine, bitmap, cliprect);
	}
}



void vsystem_spr_device::draw_sprites_aerofght( UINT16* spriteram3, int spriteram_bytes, running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int pri )
{
	int offs;
	pri <<= 12;

	offs = 0;
	while (offs < 0x0400 && (spriteram3[offs] & 0x8000) == 0)
	{
		int attr_start = 4 * (spriteram3[offs] & 0x03ff);

		/* is the way I handle pri correct? Or should I just check bit 13? */
		if ((spriteram3[attr_start + 2] & 0x3000) == pri)
		{
			get_sprite_attributes(&spriteram3[attr_start]);

			curr_sprite.color &=0x1f;
			curr_sprite.map &= 0x3fff;
	
			common_sprite_drawgfx(2, machine, bitmap, cliprect);

		}
		offs++;
	}
}


void vsystem_spr_device::f1gp2_draw_sprites(UINT16* spritelist, int flipscreen, running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	int offs;

	offs = 0;
	while (offs < 0x0400 && (spritelist[offs] & 0x4000) == 0)
	{
		int attr_start;

		attr_start = 4 * (spritelist[offs++] & 0x01ff);

		get_sprite_attributes(&spritelist[attr_start]);

		curr_sprite.color &= 0x1f;
		curr_sprite.map &= 0x7fff;

		common_sprite_drawgfx(1, machine, bitmap, cliprect);
	}
}


void vsystem_spr_device::CG10103_draw_sprite(running_machine &machine, bitmap_ind16& bitmap, const rectangle &cliprect, UINT16* spr, int drawpri)
{
	get_sprite_attributes(&spr[0]);
	curr_sprite.color &=0x1f;
	curr_sprite.pri >>= 1;

	// Check if we want to draw this sprite now
	if (curr_sprite.pri != drawpri)
		return;

	common_sprite_drawgfx(m_gfx_region, machine, bitmap, cliprect);
}


void vsystem_spr_device::CG10103_draw(running_machine &machine, int numchip, bitmap_ind16& screen, const rectangle &cliprect, int pri)
{
	UINT16* splist;
	int i;

	splist = m_vram;

	// Parse the sorting list
	for (i=0;i<0x400;i++)
	{
		UINT16 cmd = *splist++;

		// End of list
		if (cmd & 0x4000)
			break;

		if (!(cmd & 0x8000))
		{
			// Extract sprite index
			int num = cmd & 0x3FF;

			// Draw the sprite
			CG10103_draw_sprite(machine, screen, cliprect, m_vram + num*4, pri);
		}
	}
}



void vsystem_spr_device::CG10103_set_pal_base(int pal_base)
{
	m_pal_base = pal_base;
}

void vsystem_spr_device::CG10103_set_gfx_region(int gfx_region)
{
	m_gfx_region = gfx_region;
}

void vsystem_spr_device::CG10103_set_transpen(int transpen)
{
	m_transpen = transpen;
}

void vsystem_spr_device::CG10103_set_ram(UINT16* vram)
{
	m_vram = vram;
}