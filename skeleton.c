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

#define BUFFER_LEN 500
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

//Configuration for McBSP1 (data-interface)
MCBSP_Config datainterface_config = {
		/* McBSP Control Register */
        MCBSP_FMKS(SPCR, FREE, NO)              |	// Freilauf 
        MCBSP_FMKS(SPCR, SOFT, YES)          	|	// p Clock stoppt nicht beim debuggen, aber ist glaub egal, weil wir den Clock nicht vorgeben
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
    EDMA_FMKS(OPT, PRI, LOW)          |   // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode p Increment
    EDMA_FMKS(OPT, TCINT,YES)         |   // EDMA interrupt erzeugen? p Ja, Interrupt wird nach Sendem von einem Element gesetzt
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

    EDMA_FMKS(CNT, FRMCNT, OF(0))      |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

    (Uint32)Buffer_in_ping,       		  // Ziel-Adresse, p Casten von Short-> Uint32

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

    EDMA_FMK (RLD, ELERLD, 0)       |  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};

/* added */
EDMA_Config configEDMAXmt = {
	EDMA_FMKS(OPT, PRI, LOW)          |   // auf beide Queues verteilen
	EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size
	EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
	EDMA_FMKS(OPT, SUM, INC)           |  // Quell
	EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
	EDMA_FMKS(OPT, DUM, NONE)          |  // Ziel-update mode -> FEST (McBSP)!!!
	EDMA_FMKS(OPT, TCINT,YES)         |   // EDMA interrupt erzeugen? p Ja, Interrupt wird nach Sendem von einem Element gesetzt
	EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
	EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
	EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

	EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

	EDMA_FMKS(CNT, FRMCNT, OF(0))       |  // Anzahl Frames
	EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

	(Uint32)Buffer_in_ping,       		  // Ziel-Adresse, p Casten von Short-> Uint32

	EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
	EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

	EDMA_FMK (RLD, ELERLD, 0)       |  // Reload Element
	EDMA_FMK (RLD, LINK, 0)            // Reload Link
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
    
	/* configure EDMA */
    config_EDMA();

    /* finally the interrupts p EDMA_INT schaltet immer um zwischen process_ping_SWI und process_pong_SWI bzw. ruft die Funktionen auf */
    config_interrupts();

    MCBSP_start(hMcbsp, MCBSP_RCV_START, 0xffffffff);	/* tell it what you want to start p start to recieve, Golden wire macht die Ausgabe  ??? */
    MCBSP_write(hMcbsp, 0x0); 	/* one shot */
} /* finished*/


void config_EDMA(void)
{
/* ################ Config receive ################ */
	hEdmaRcv = EDMA_open(EDMA_CHA_REVT1, EDMA_OPEN_RESET);  	// EDMA Channel for REVT1 // open a Channel, open EDMA
	hEdmaReloadRcvPing = EDMA_allocTable(-1);               			// Reload-Parameters
	hEdmaReloadRcvPong = EDMA_allocTable(-1);               			// Reload-Parameters
	hEdmaReloadRcvPung = EDMA_allocTable(-1);               			// Reload-Parameters

	configEDMARcv.src = MCBSP_getRcvAddr(hMcbsp);         	//  source addr , constant


	tccRcvPing = EDMA_intAlloc(-1);                         // next available TCC Transfer Complete Code
	tccRcvPong = EDMA_intAlloc(-1);							//   indicate which channel (parameter set) is ready
	tccRcvPung = EDMA_intAlloc(-1);							//

	/* configure rcvchannel*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);		// set TCC
	configEDMARcv.dst = (Uint32)(&Buffer_in_ping);			// set destination address
	EDMA_config(hEdmaRcv, &configEDMARcv);					// write config to channel

	/* configure reloadrcvping*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);		// set TCC
	configEDMARcv.dst = (Uint32)(&Buffer_in_ping);			// set destination address
	EDMA_config(hEdmaReloadRcvPing, &configEDMARcv);		// write config to reload

	/* configure reloadrcvpong*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPong);		// set TCC
	configEDMARcv.dst = (Uint32)(&Buffer_in_pong);			// set destination address
	EDMA_config(hEdmaReloadRcvPong, &configEDMARcv);		// write config to reload
	
	/* configure reloadrcvpung*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPung);		// set TCC
	configEDMARcv.dst = (Uint32)(&Buffer_in_pung);			// set destination address
	EDMA_config(hEdmaReloadRcvPung, &configEDMARcv);		// write config to reload
	/* could we need also some other EDMA read job?*/ /*p ja brauchen wir fuer pong receive und pingpong transmit*/

	/* link transfers ping -> pong -> ping */
	EDMA_link(hEdmaRcv,hEdmaReloadRcvPong);  /* is that all? p  EDMA_link(CSL p.161) */
	EDMA_link(hEdmaReloadRcvPing,hEdmaReloadRcvPong);
	EDMA_link(hEdmaReloadRcvPong,hEdmaReloadRcvPing); //to include pung change ping
	EDMA_link(hEdmaReloadRcvPung,hEdmaReloadRcvPing);

	/* do you want to hear music? When yes you need a EDMA writing */

/* ################ Config transmit ################ */
	hEdmaXmt = EDMA_open(EDMA_CHA_XEVT1, EDMA_OPEN_RESET);  // EDMA Channel for XEVT1 // open a Channel, open EDMA
	hEdmaReloadXmtPing = EDMA_allocTable(-1);              // Reload-Parameters
	hEdmaReloadXmtPong = EDMA_allocTable(-1);              // Reload-Parameters


	configEDMAXmt.dst = MCBSP_getXmtAddr(hMcbsp);          //  source addr wird zugewiesen

	tccXmtPing = EDMA_intAlloc(-1);
	tccXmtPong = EDMA_intAlloc(-1);

	/* configure xmtchannel*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);		// set TCC
	configEDMARcv.src = (Uint32)(&Buffer_out_ping);			// set destination address
	EDMA_config(hEdmaXmt, &configEDMAXmt);					// write config to channel

	/* configure reloadxmtping*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);		// set TCC
	configEDMARcv.src = (Uint32)(&Buffer_out_ping);			// set destination address
	EDMA_config(hEdmaReloadXmtPing, &configEDMAXmt);		// write config to reload

	/* configure reloadxmtpong*/
	configEDMARcv.opt &= 0xFFF0FFFF;						//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccXmtPong);		// set TCC
	configEDMARcv.src = (Uint32)(&Buffer_out_pong);			// set destination address
	EDMA_config(hEdmaReloadXmtPong, &configEDMAXmt);		// write config to reload

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
	/* p added*/
	EDMA_intClear(tccXmtPong);
	EDMA_intEnable(tccXmtPong);
	/* p added*/
	EDMA_intClear(tccRcvPung);
	EDMA_intEnable(tccRcvPung);

	/* which EDMAs do we have to enable? */
	EDMA_enableChannel(hEdmaRcv);
	//added:
	EDMA_enableChannel(hEdmaXmt);
}

void config_interrupts(void)
{   //Luecken added
	IRQ_map(IRQ_EVT_EDMAINT, 12);	//added Interrupt sources (here EDMAINT) and mapping to CPU interrupts (is chosen by 12)
	IRQ_clear(IRQ_EVT_EDMAINT);				//first clear and then enable the interrupts
	IRQ_enable(IRQ_EVT_EDMAINT);
	IRQ_globalEnable();
}


void EDMA_interrupt_service(void)
{
	static int rcvPingDone=0; //static variables, to keep their values
	static int rcvPongDone=0;
	static int xmtPingDone=0;
	static int xmtPongDone=0;
	
	if(EDMA_intTest(tccRcvPing)) {
		EDMA_intClear(tccRcvPing); /* clear is mandatory p loescht gesetztes Bit (tccRcvPing)*/
		rcvPingDone=1;
	} /* p EDMA_intTest testet, ob der Wert von tccRcvPing (Interrupt number) im CIPR gesetz ist.
		(z.B. tcc 15 (Bei uns eig immer 15) CIRR Bit 15 gesetzt oder nicht)  Rueckgabewert:1=flag was set */

	else if(EDMA_intTest(tccRcvPong)) {
		EDMA_intClear(tccRcvPong);
		rcvPongDone=1;
	}

	else if(EDMA_intTest(tccXmtPing)) {     //transmit ping is finished  p added
			EDMA_intClear(tccXmtPing);
			xmtPingDone=1;
		}
	else if(EDMA_intTest(tccXmtPong)) {     //transmit pong is finished  p added
				EDMA_intClear(tccXmtPong);
				xmtPongDone=1;
			}
	
	if(rcvPingDone && xmtPingDone) {
		rcvPingDone=0;
		xmtPingDone=0;
		// processing in SWI
		SWI_post(&SWI_process_ping);
	}
	else if(rcvPongDone && xmtPongDone) {
		rcvPongDone=0;
		xmtPongDone=0;
		// processing in SWI
		SWI_post(&SWI_process_pong);
	}
}

void process_ping_SWI(void)					//Golden wire
{
	int i;
	for(i=0; i<BUFFER_LEN; i++)
		*(Buffer_out_ping+i) = *(Buffer_in_ping+i);
}

void process_pong_SWI(void)
{
	int i;
	for(i=0; i<BUFFER_LEN; i++)
		*(Buffer_out_pong+i) = *(Buffer_in_pong+i);
}

void LEDToggle_SWI(void)
{
	SEM_postBinary(&SEM_LEDToggle);			//create a Semaphore and a task(insert a function: LED toggle)
}

void led_toggle_tsk(void)
{
	/* initializatoin of the task */
	/* nothing to do */

	/* process */
	while(1) {
		SEM_pendBinary(&SEM_LEDToggle, SYS_FOREVER);

		DSK6713_LED_toggle(1);				//LED 1
	}
}
