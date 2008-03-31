/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef PC_H_
#define PC_H_

/*----------- defined in machine/pc.c -----------*/

void mess_init_pc_common(UINT32 flags);

DRIVER_INIT( pccga );
DRIVER_INIT( pcmda );
DRIVER_INIT( europc );
DRIVER_INIT( bondwell );
DRIVER_INIT( pc200 );
DRIVER_INIT( pc1512 );
DRIVER_INIT( pc1640 );
DRIVER_INIT( pc_vga );
DRIVER_INIT( t1000hx );

MACHINE_RESET( pc_mda );
MACHINE_RESET( pc_cga );
MACHINE_RESET( pc_t1t );
MACHINE_RESET( pc_aga );
MACHINE_RESET( pc_pc1512 );
MACHINE_RESET( pc_vga );

INTERRUPT_GEN( pc_cga_frame_interrupt );
INTERRUPT_GEN( pc_pc1512_frame_interrupt );
INTERRUPT_GEN( pc_mda_frame_interrupt );
INTERRUPT_GEN( tandy1000_frame_interrupt );
INTERRUPT_GEN( pc_aga_frame_interrupt );
INTERRUPT_GEN( pc_vga_frame_interrupt );


#endif /* PC_H_ */
