#ifndef SKELETON_H_
#define SKELETON_H

extern void process_ping_SWI(void);
extern void EDMA_interrupt_service(void);
extern void config_EDMA(void);
extern void config_interrupts(void);
extern void SWI_LEDToggle(void);
extern void tsk_led_toggle(void);
extern void tsk_ledon_speechpause(void);
extern void tsk_ledoff_speechpause(void);

typedef enum{    // enumeration for simple communication
	processNone, //=0 dont process anything
	processPing, //=1
	processPong, //=2
	processPung  //=3
}enum_process;

	
#endif /*SKELETON_H*/
