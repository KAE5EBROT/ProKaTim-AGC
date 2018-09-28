/***********************************************************
*  skeleton.c
*  Example for ping-pong processing
*  Caution: It is intended, that this file ist not runnable. 
*  The file contains mistakes and omissions, which shall be
*  corrected and completed by the students.
*
*   F. Quint, HsKA //test
************************************************************/

#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include <csl_edma.h>
#include <dsk6713_led.h>
#include "config_AIC23.h"
#include "skeleton.h"
#include "Debug/ProKaTimSS2018cfg.h"
#include "math.h"

#define BUFFER_LEN 9600 /* 100ms * 48000Hz *2channels (left & right) = 4800 */

/* Ping-Pong buffers. Place them in the compiler section .datenpuffer */
/* How do you place the compiler section in the memory? p Seite 45 EDMA    */
#pragma DATA_SECTION(Buffer_in_ping, ".datenpuffer");
short Buffer_in_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_in_pong, ".datenpuffer");
short Buffer_in_pong[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_in_pung, ".datenpuffer");
short Buffer_in_pung[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_ping, ".datenpuffer");
short Buffer_out_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_pong, ".datenpuffer");
short Buffer_out_pong[BUFFER_LEN];

short* psprocessin1;
short* psprocessin2;
short* psprocessout;
double desired_power=2000;
double minimal_power=200;
int usedbuffer=960;
volatile int usedbuffer_gel=960;
#define GAINHIST 200
float gain_his[GAINHIST];
int gain_cnt = 0;
int improvement = 0;

//Configuration for McBSP1 (data-interface)
MCBSP_Config datainterface_config = {
		/* McBSP Control Register */
        MCBSP_FMKS(SPCR, FREE, NO)              |	// Freilauf 
        MCBSP_FMKS(SPCR, SOFT, NO)          	|	// p Clock stoppt nicht beim debuggen, aber ist glaub egal, weil wir den Clock nicht vorgeben
        MCBSP_FMKS(SPCR, FRST, YES)             |	// Framesync ist ein
        MCBSP_FMKS(SPCR, GRST, YES)             |	// Reset aus, damit laeuft der Samplerate- Generator
        MCBSP_FMKS(SPCR, XINTM, XRDY)           |	// p Sender Interrupt wird durch "XRDY-Bit" ausgeloest. Interrupt koennen wir vielleicht fuer EDMA nutzen.
        MCBSP_FMKS(SPCR, XSYNCERR, NO)          |	// empfaengerseitig keine Ueberwachung der Synchronisation
        MCBSP_FMKS(SPCR, XRST, YES)             |	// Sender laeuft (kein Reset- Status)
        MCBSP_FMKS(SPCR, DLB, OFF)              |	// Loopback (Kurschluss) nicht aktiv
        MCBSP_FMKS(SPCR, RJUST, RZF)            |	// rechtsbuendige Ausrichtung der Daten im Puffer
        MCBSP_FMKS(SPCR, CLKSTP, DISABLE)       |	// Clock startet ohne Verzoegerung auf fallenden Flanke (siehe auch PCR-Register)
        MCBSP_FMKS(SPCR, DXENA, OFF)            |	// DX- Enabler wird nicht verwendet
        MCBSP_FMKS(SPCR, RINTM, RRDY)           |	// Sender Interrupt wird durch "RRDY-Bit" ausgeloest
        MCBSP_FMKS(SPCR, RSYNCERR, NO)          |	// senderseitig keine Ueberwachung der Synchronisation
        MCBSP_FMKS(SPCR, RRST, YES),			// Empfaenger laeuft (kein Reset- Status)
		/* Empfangs-Control Register */
        MCBSP_FMKS(RCR, RPHASE, SINGLE)         |	// Nur eine Phase pro Frame
        MCBSP_FMKS(RCR, RFRLEN2, DEFAULT)       |	// Laenge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RWDLEN2, DEFAULT)       |	// Wortlaenge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(RCR, RFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die Uebertragung neu
        MCBSP_FMKS(RCR, RDATDLY, 0BIT)          |	// keine Verzoegerung (delay) der Daten
        MCBSP_FMKS(RCR, RFRLEN1, OF(1))         |	// Laenge der Phase 1 --> 1 Wort
        MCBSP_FMKS(RCR, RWDLEN1, 16BIT)         |	// p AIC erwartet 16 Bit David: Pro channel -> 32bit
        MCBSP_FMKS(RCR, RWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt
		/* Sende-Control Register */
        MCBSP_FMKS(XCR, XPHASE, SINGLE)         |	//
        MCBSP_FMKS(XCR, XFRLEN2, DEFAULT)       |	// Laenge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XWDLEN2, DEFAULT)       |	// Wortlaenge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(XCR, XFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die Uebertragung neu
        MCBSP_FMKS(XCR, XDATDLY, 0BIT)          |	// keine Verzoegerung (delay) der Daten
        MCBSP_FMKS(XCR, XFRLEN1, OF(1))         |	// Laenge der Phase 1 --> 1 Wort
        MCBSP_FMKS(XCR, XWDLEN1, 16BIT)         |	// Wortlaenge in Phase 1 --> 16 bit
        MCBSP_FMKS(XCR, XWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt
		/* Sample Rate Generator Register */
        MCBSP_FMKS(SRGR, GSYNC, DEFAULT)        |	// Einstellungen nicht relevant da
        MCBSP_FMKS(SRGR, CLKSP, DEFAULT)        |	// der McBSP1 als Slave laeuft
        MCBSP_FMKS(SRGR, CLKSM, DEFAULT)        |	// und den Takt von aussen 
        MCBSP_FMKS(SRGR, FSGM, FSG)         |	// vorgegeben bekommt.
        MCBSP_FMKS(SRGR, FPER, DEFAULT)         |	// --
        MCBSP_FMKS(SRGR, FWID, DEFAULT)         |	// --
        MCBSP_FMKS(SRGR, CLKGDV, DEFAULT),		// --
		/* Mehrkanal */
        MCBSP_MCR_DEFAULT,				// Mehrkanal wird nicht verwendet
        MCBSP_RCER_DEFAULT,				// dito
        MCBSP_XCER_DEFAULT,				// dito
		/* Pinout Control Register */
        MCBSP_FMKS(PCR, XIOEN, SP)              |	// Pin wird fuer serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, RIOEN, SP)              |	// Pin wird fuer serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, FSXM, EXTERNAL)         |	// Framesync- Signal fuer Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, FSRM, EXTERNAL)         |	// Framesync- Signal fuer Empfaenger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKXM, INPUT)           |	// Takt fuer Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKRM, INPUT)           |	// Takt fuer Empfaenger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKSSTAT, DEFAULT)      |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, DXSTAT, DEFAULT)        |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, FSXP, ACTIVEHIGH)       |	// Framesync senderseitig ist "activehigh"
        MCBSP_FMKS(PCR, FSRP, ACTIVEHIGH)       |	// Framesync empfaengerseitig ist "activehigh"
        MCBSP_FMKS(PCR, CLKXP, FALLING)         |	// Datum wird bei fallender Flanke gesendet
        MCBSP_FMKS(PCR, CLKRP, RISING)			// Datum wird bei steigender Flanke uebernommen
};

/* template for a EDMA configuration */
EDMA_Config configEDMARcv = {
    EDMA_FMKS(OPT, PRI, LOW)           |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size (linker und rechter Channel wird nacheinander �bertragen)
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode (Increment)
    EDMA_FMKS(OPT, TCINT,YES)          |  // EDMA interrupt erzeugen? Ja, Interrupt wird nach Sendem von einem Element gesetzt (Ele. synch.)
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

    EDMA_FMKS(CNT, FRMCNT, OF(0))      |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

    (Uint32)Buffer_in_ping,       		  // Ziel-Adresse (Casten von Short-> Uint32)

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

    EDMA_FMK (RLD, ELERLD, 0)          |  // Reload Element
    EDMA_FMK (RLD, LINK, 0)               // Reload Link
};

EDMA_Config configEDMAXmt = {
	EDMA_FMKS(OPT, PRI, LOW)          |   // auf beide Queues verteilen
	EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size   //32 bit ge�ndert, da immer nur nach 32 Bit XINT Interrupt kommt
	EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
	EDMA_FMKS(OPT, SUM, INC)           |  // Quell
	EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
	EDMA_FMKS(OPT, DUM, NONE)          |  // Ziel-update mode -> FEST (McBSP)!!!
	EDMA_FMKS(OPT, TCINT,YES)         |   // EDMA interrupt erzeugen? Ja, Interrupt wird nach Sendem von einem Frame (oder nach einem Job) gesetzt
	EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
	EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
	EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

	(Uint32)Buffer_out_ping,       		  // Quell-Adresse, p Casten von Short-> Uint32

	EDMA_FMKS(CNT, FRMCNT, OF(0))       |  // Anzahl Frames
	EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

	EDMA_FMKS(DST, DST, OF(0)),           // Ziel-Adresse

	EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
	EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

	EDMA_FMK (RLD, ELERLD, 0)     	  |  // Reload Element
	EDMA_FMK (RLD, LINK, 0)       	     // Reload Link
};

/* Transfer-Complete-Codes for EDMA-Jobs */
int tccRcvPing;
int tccRcvPong;
int tccRcvPung;

int tccXmtPing;
int tccXmtPong;

/* EDMA-Handles; are these really all? yes*/
EDMA_Handle hEdmaRcv;
EDMA_Handle hEdmaReloadRcvPing;
EDMA_Handle hEdmaReloadRcvPong;
EDMA_Handle hEdmaReloadRcvPung;
EDMA_Handle hEdmaXmt;
EDMA_Handle hEdmaReloadXmtPing;
EDMA_Handle hEdmaReloadXmtPong;
MCBSP_Handle hMcbsp=0;

main()
{
	CSL_init();


	/* Configure McBSP0 and AIC23 */
	Config_DSK6713_AIC23();

	/* Configure McBSP1*/
	hMcbsp = MCBSP_open(MCBSP_DEV1, MCBSP_OPEN_RESET); //Bevor ein McBSP Port verwendet werden kann, muss er erst mit dieser Funktion geoeffnet werden
    MCBSP_config(hMcbsp, &datainterface_config);	//Einstellungen uebergeben

    config_EDMA();

    config_interrupts();

    MCBSP_start(hMcbsp, MCBSP_RCV_START, 0xffffffff);	//start receive (RRST is set --> The serial port receiver is enabled)
    MCBSP_start(hMcbsp, MCBSP_XMIT_START, 0xffffffff);	//start transmit (XRST is set --> Serial port transmitter is enabled)

    MCBSP_write(hMcbsp,0x00);							//one shot

}

//
void config_EDMA(void)
{
/* ################ Config receive ################ */
	hEdmaRcv = EDMA_open(EDMA_CHA_REVT1, EDMA_OPEN_RESET);  	// open EDMA Channel for REVT1
	hEdmaReloadRcvPing = EDMA_allocTable(-1);               	// Reload-Parameters
	hEdmaReloadRcvPong = EDMA_allocTable(-1);               	// Reload-Parameters
	hEdmaReloadRcvPung = EDMA_allocTable(-1);               	// Reload-Parameters

	configEDMARcv.src = MCBSP_getRcvAddr(hMcbsp);         		// source addr , constant
	configEDMARcv.cnt =	EDMA_FMKS(CNT, FRMCNT, OF(0)) | EDMA_FMK (CNT, ELECNT, usedbuffer);

	tccRcvPing = EDMA_intAlloc(-1);                        		// next available TCC Transfer Complete Code
	tccRcvPong = EDMA_intAlloc(-1);								// indicate which channel (parameter set) is ready
	tccRcvPung = EDMA_intAlloc(-1);

	/* configure rcvchannel*/
	configEDMARcv.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);			// set TCC
	configEDMARcv.dst = (Uint32)(Buffer_in_ping);				// set destination address
	EDMA_config(hEdmaRcv, &configEDMARcv);						// write config to channel
	EDMA_config(hEdmaReloadRcvPing, &configEDMARcv);			/*configure reloadrcvping*/

	/* configure reloadrcvpong*/
	configEDMARcv.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPong);			// set TCC
	configEDMARcv.dst = (Uint32)(Buffer_in_pong);				// set destination address
	EDMA_config(hEdmaReloadRcvPong, &configEDMARcv);			// write config to reload

	/* configure reloadrcvpung*/
	configEDMARcv.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPung);			// set TCC
	configEDMARcv.dst = (Uint32)(Buffer_in_pung);				// set destination address
	EDMA_config(hEdmaReloadRcvPung, &configEDMARcv);			// write config to reload

	/* link transfers ping -> pong -> ping */
	EDMA_link(hEdmaRcv,hEdmaReloadRcvPong);  			/* is that all? EDMA_linking */
	EDMA_link(hEdmaReloadRcvPing,hEdmaReloadRcvPong);
	EDMA_link(hEdmaReloadRcvPong,hEdmaReloadRcvPung);
	EDMA_link(hEdmaReloadRcvPung,hEdmaReloadRcvPing);

	/* do you want to hear music? When yes you need a EDMA writing */

/* ################ Config transmit ################ */
	hEdmaXmt = EDMA_open(EDMA_CHA_XEVT1, EDMA_OPEN_RESET);  	// open EDMA Channel for XEVT1
	hEdmaReloadXmtPing = EDMA_allocTable(-1);              		// Reload-Parameters
	hEdmaReloadXmtPong = EDMA_allocTable(-1);              		// Reload-Parameters

	configEDMAXmt.dst = MCBSP_getXmtAddr(hMcbsp);          		//  destination addr wird zugewiesen
	configEDMAXmt.cnt =	EDMA_FMKS(CNT, FRMCNT, OF(0)) | EDMA_FMK (CNT, ELECNT, usedbuffer);

	tccXmtPing = EDMA_intAlloc(-1);
	tccXmtPong = EDMA_intAlloc(-1);

	/* configure xmtchannel*/
	configEDMAXmt.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
	configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);			// set TCC
	configEDMAXmt.src = (Uint32)(Buffer_out_ping);				// set destination address
	EDMA_config(hEdmaXmt, &configEDMAXmt);						// write config to channel
	EDMA_config(hEdmaReloadXmtPing, &configEDMAXmt);			/*configure reloadrcvping*/

	/* configure reloadxmtpong*/
	configEDMAXmt.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
	configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPong);			// set TCC
	configEDMAXmt.src = (Uint32)(Buffer_out_pong);				// set destination address
	EDMA_config(hEdmaReloadXmtPong, &configEDMAXmt);			// write config to reload

	EDMA_link(hEdmaXmt,hEdmaReloadXmtPong);
	EDMA_link(hEdmaReloadXmtPing,hEdmaReloadXmtPong);
	EDMA_link(hEdmaReloadXmtPong,hEdmaReloadXmtPing);

	/* enable EDMA TCC */
	EDMA_intClear(tccRcvPing);
	EDMA_intEnable(tccRcvPing);
	/* some more? p added*/
	EDMA_intClear(tccXmtPing);
	EDMA_intEnable(tccXmtPing);
	/* p added*/
	EDMA_intClear(tccRcvPong);
	EDMA_intEnable(tccRcvPong);
//	/* p added*/
	EDMA_intClear(tccXmtPong);
	EDMA_intEnable(tccXmtPong);

	EDMA_intClear(tccRcvPung);
    EDMA_intEnable(tccRcvPung);

	/* which EDMAs do we have to enable? */
	EDMA_enableChannel(hEdmaRcv);
	//added:
	EDMA_enableChannel(hEdmaXmt);
}

void config_interrupts(void)
{
	IRQ_clear(IRQ_EVT_EDMAINT);									//first clear and then enable the interrupts
	IRQ_enable(IRQ_EVT_EDMAINT);
	IRQ_globalEnable();
}


void EDMA_interrupt_service(void)
{
	static enum_process eprocessin = processNone;
	static enum_process eprocessout = processNone;
	static short failcounter = 0;

	if(EDMA_intTest(tccRcvPing)) {
		EDMA_intClear(tccRcvPing); /* clear is mandatory p loescht gesetztes Bit (tccRcvPing)*/
		eprocessin=processPing;
	} /* p EDMA_intTest testet, ob der Wert von tccRcvPing (Interrupt number) im CIPR gesetz ist.
		(z.B. tcc 15 (Bei uns eig immer 15) CIRR Bit 15 gesetzt oder nicht)  Rueckgabewert:1=flag was set */

	else if(EDMA_intTest(tccRcvPong)) {
		EDMA_intClear(tccRcvPong);
		eprocessin=processPong;
	}
	else if(EDMA_intTest(tccRcvPung)) {
		EDMA_intClear(tccRcvPung);
		eprocessin=processPung;
	}

	if(EDMA_intTest(tccXmtPing)) {     //transmit ping is finished  p added
		EDMA_intClear(tccXmtPing);
		eprocessout=processPing;
	}
	else if(EDMA_intTest(tccXmtPong)) {     //transmit pong is finished  p added
		EDMA_intClear(tccXmtPong);
		eprocessout=processPong;
	}

	if(eprocessin && eprocessout) {
		failcounter = 0; /* reset counter, operation works */
		switch(eprocessin){
		case processNone:
			break;
		case processPing:
			psprocessin1 = Buffer_in_ping;
			psprocessin2 = Buffer_in_pung;
			break;
		case processPong:
			psprocessin1 = Buffer_in_pong;
			psprocessin2 = Buffer_in_ping;
			break;
		case processPung:
			psprocessin1 = Buffer_in_pung;
			psprocessin2 = Buffer_in_pong;
		}
		switch(eprocessout){
		case processNone:
			break;
		case processPing:
			psprocessout = Buffer_out_ping;
			break;
		case processPong:
			psprocessout = Buffer_out_pong;
		}
		eprocessin = processNone;
		eprocessout = processNone;
		// processing in SWI
		SWI_post(&SWI_process_ping);
	}
	else{ /* if only one channel finishes multiple times, rewrite EDMA */
		failcounter++;
	}
	if((usedbuffer_gel != usedbuffer) || (failcounter > 10)){
		failcounter = 0;
		SEM_postBinary(&SEM_updateEDMA);
	}
}

void process_ping_SWI(void)					//Golden wire
{
	int i;
	static float gain_old=0;
	static int ledon=0;
	double power=0,gain=0;

	for(i=0; i<usedbuffer; i++){
		power+=psprocessin1[i]*psprocessin1[i];
	}
	power=sqrt(power/(double)usedbuffer);
	if((power<minimal_power)){
		power=desired_power;
		if(ledon!=0){
		SEM_postBinary(&SEM_speechoff);
		ledon=0;
		}
	}
	else if(ledon==0){
		SEM_postBinary(&SEM_speechon);
		ledon=!0;
	}

	gain=desired_power/power;

	float gaindiff=gain-gain_old;
	if((improvement)&&(gaindiff>0.01*usedbuffer/96)){
		gaindiff=0.01*usedbuffer/96;
		gain=gain_old + gaindiff;
	}
	if(++gain_cnt >= GAINHIST)
		gain_cnt = 0;
	gain_his[gain_cnt] = gain;
	int j=0; /* is used for indexing processout */
	for(i=usedbuffer/2; i<usedbuffer; i++,j++){ /* 1st half */
		psprocessout[j]=psprocessin2[i]*(gain_old+((gaindiff*j)/usedbuffer));
	};

	for(i=0; i<usedbuffer/2; i++,j++){ /*2nd half */
		psprocessout[j]=psprocessin1[i]*(gain_old+((gaindiff*j)/usedbuffer));
	};

	gain_old=gain;
	DSK6713_LED_toggle(1);
}

void SWI_LEDToggle(void)
{
	SEM_postBinary(&SEM_LEDToggle);			//create a Semaphore and a task(insert a function: LED toggle)
}

void tsk_led_toggle(void)
{
	/* initializatoin of the task */
	/* nothing to do */

	/* process */
	while(1) {
		SEM_pendBinary(&SEM_LEDToggle, SYS_FOREVER);

//		DSK6713_LED_toggle(1);				//LED 1
	}
}

void tsk_ledon_speechpause(void)
{
	while(1) {
		SEM_pendBinary(&SEM_speechon, SYS_FOREVER);

		DSK6713_LED_on(3);				//LED 3 an
	}
}

void tsk_ledoff_speechpause(void)
{
	while(1) {
		SEM_pendBinary(&SEM_speechoff, SYS_FOREVER);

		DSK6713_LED_off(3);				//LED 3 aus
	}
}

void tsk_updateEDMA_f(void)
{
	while(1) {
		SEM_pendBinary(&SEM_updateEDMA, SYS_FOREVER);
		usedbuffer = usedbuffer_gel;
		EDMA_disableChannel(hEdmaRcv);
		EDMA_disableChannel(hEdmaXmt);
		configEDMARcv.cnt =	EDMA_FMKS(CNT, FRMCNT, OF(0)) | EDMA_FMK (CNT, ELECNT, usedbuffer);

		/* configure rcvchannel*/
		configEDMARcv.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);			// set TCC
		configEDMARcv.dst = (Uint32)(Buffer_in_ping);				// set destination address
		EDMA_config(hEdmaRcv, &configEDMARcv);						// write config to channel
		EDMA_config(hEdmaReloadRcvPing, &configEDMARcv);			/*configure reloadrcvping*/

		/* configure reloadrcvpong*/
		configEDMARcv.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPong);			// set TCC
		configEDMARcv.dst = (Uint32)(Buffer_in_pong);				// set destination address
		EDMA_config(hEdmaReloadRcvPong, &configEDMARcv);			// write config to reload

		/* configure reloadrcvpung*/
		configEDMARcv.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPung);			// set TCC
		configEDMARcv.dst = (Uint32)(Buffer_in_pung);				// set destination address
		EDMA_config(hEdmaReloadRcvPung, &configEDMARcv);			// write config to reload

		/* link transfers ping -> pong -> pung */
		EDMA_link(hEdmaRcv,hEdmaReloadRcvPong);  			/* is that all? EDMA_linking */
		EDMA_link(hEdmaReloadRcvPing,hEdmaReloadRcvPong);
		EDMA_link(hEdmaReloadRcvPong,hEdmaReloadRcvPung);
		EDMA_link(hEdmaReloadRcvPung,hEdmaReloadRcvPing);

		configEDMAXmt.dst = MCBSP_getXmtAddr(hMcbsp);          		//  destination addr wird zugewiesen
		configEDMAXmt.cnt =	EDMA_FMKS(CNT, FRMCNT, OF(0)) | EDMA_FMK (CNT, ELECNT, usedbuffer);

		/* configure xmtchannel*/
		configEDMAXmt.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
		configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);			// set TCC
		configEDMAXmt.src = (Uint32)(Buffer_out_ping);				// set destination address
		EDMA_config(hEdmaXmt, &configEDMAXmt);						// write config to channel
		EDMA_config(hEdmaReloadXmtPing, &configEDMAXmt);			/*configure reloadrcvping*/

		/* configure reloadxmtpong*/
		configEDMAXmt.opt &= 0xFFF0FFFF;							//Reset TCC for new TCC
		configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPong);			// set TCC
		configEDMAXmt.src = (Uint32)(Buffer_out_pong);				// set destination address
		EDMA_config(hEdmaReloadXmtPong, &configEDMAXmt);			// write config to reload

		EDMA_link(hEdmaXmt,hEdmaReloadXmtPong);
		EDMA_link(hEdmaReloadXmtPing,hEdmaReloadXmtPong);
		EDMA_link(hEdmaReloadXmtPong,hEdmaReloadXmtPing);
		EDMA_intClear(tccRcvPing);
		EDMA_intEnable(tccRcvPing);
		EDMA_intClear(tccXmtPing);
		EDMA_intEnable(tccXmtPing);
		EDMA_intClear(tccRcvPong);
		EDMA_intEnable(tccRcvPong);
		EDMA_intClear(tccXmtPong);
		EDMA_intEnable(tccXmtPong);
		EDMA_intClear(tccRcvPung);
	    EDMA_intEnable(tccRcvPung);
		EDMA_enableChannel(hEdmaRcv);
		EDMA_enableChannel(hEdmaXmt);
//		EDMA_close(hEdmaRcv);
//		EDMA_close(hEdmaXmt);
//		EDMA_freeTable(hEdmaReloadRcvPing);
//		EDMA_freeTable(hEdmaReloadRcvPong);
//		EDMA_freeTable(hEdmaReloadRcvPung);
//		EDMA_freeTable(hEdmaReloadXmtPing);
//		EDMA_freeTable(hEdmaReloadXmtPong);
//		config_EDMA();
	}
}
