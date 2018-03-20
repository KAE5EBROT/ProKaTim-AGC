#ifndef SKELETON_H_
#define SKELETON_H

extern void process_ping_SWI(void);
extern void process_pong_SWI(void);
extern void EDMA_interrupt_service(void);
extern void config_EDMA(void);
extern void config_interrupts(void);
extern void LEDToggle_SWI(void);
extern void led_toggle_tsk(void);

	
#endif /*SKELETON_H*/
