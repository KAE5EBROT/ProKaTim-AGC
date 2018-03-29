#ifndef SKELETON_H_
#define SKELETON_H

extern void process_ping_SWI(void);
extern void EDMA_interrupt_service(void);
extern void config_EDMA(void);
extern void config_interrupts(void);
extern void SWI_LEDToggle(void);
extern void tsk_led_toggle(void);

typedef enum{
	processNone,
	processPing,
	processPong,
	processPung
}enum_process;

	
#endif /*SKELETON_H*/
