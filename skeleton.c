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

#define BUFFER_LEN 500
/* Ping-Pong buffers. Place them in the compiler section .datenpuffer */
/* How do you place the compiler section in the memory? p Seite 45 EDMA    */
#pragma DATA_SECTION(Buffer_in_ping, ".datenpuffer");
short Buffer_in_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_in_pong, ".datenpuffer");
short Buffer_in_pong[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_ping, ".datenpuffer");
short Buffer_out_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_pong, ".datenpuffer");
short Buffer_out_pong[BUFFER_LEN];
void config_interrupts(void);

//Configuration for McBSP1 (data-interface)
MCBSP_Config datainterface_config = {
		/* McBSP Control Register */
        MCBSP_FMKS(SPCR, FREE, NO)              |	// Freilauf 
        MCBSP_FMKS(SPCR, SOFT, YES)          	|	// p Clock stoppt nicht beim debuggen, aber ist glaub egal, weil wir den Clock nicht vorgeben
        MCBSP_FMKS(SPCR, FRST, YES)             |	// Framesync ist ein
        MCBSP_FMKS(SPCR, GRST, YES)             |	// Reset aus, damit läuft der Samplerate- Generator
        MCBSP_FMKS(SPCR, XINTM, XRDY)           |	// p Sender Interrupt wird durch "XRDY-Bit" ausgelöst. Interrupt können wir vielleicht für EDMA nutzen.
        MCBSP_FMKS(SPCR, XSYNCERR, NO)          |	// empfängerseitig keine Überwachung der Synchronisation
        MCBSP_FMKS(SPCR, XRST, YES)             |	// Sender läuft (kein Reset- Status)	
        MCBSP_FMKS(SPCR, DLB, OFF)              |	// Loopback (Kurschluss) nicht aktiv
        MCBSP_FMKS(SPCR, RJUST, RZF)            |	// rechtsbündige Ausrichtung der Daten im Puffer
        MCBSP_FMKS(SPCR, CLKSTP, DISABLE)       |	// Clock startet ohne Verzögerung auf fallenden Flanke (siehe auch PCR-Register)
        MCBSP_FMKS(SPCR, DXENA, OFF)            |	// DX- Enabler wird nicht verwendet
        MCBSP_FMKS(SPCR, RINTM, RRDY)           |	// Sender Interrupt wird durch "RRDY-Bit" ausgelöst
        MCBSP_FMKS(SPCR, RSYNCERR, NO)          |	// senderseitig keine Überwachung der Synchronisation
        MCBSP_FMKS(SPCR, RRST, YES),			// Empfänger läuft (kein Reset- Status)
		/* Empfangs-Control Register */
        MCBSP_FMKS(RCR, RPHASE, SINGLE)         |	// Nur eine Phase pro Frame
        MCBSP_FMKS(RCR, RFRLEN2, DEFAULT)       |	// Länge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RWDLEN2, DEFAULT)       |	// Wortlänge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(RCR, RFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die Übertragung neu
        MCBSP_FMKS(RCR, RDATDLY, 0BIT)          |	// keine Verzögerung (delay) der Daten
        MCBSP_FMKS(RCR, RFRLEN1, OF(1))         |	// Länge der Phase 1 --> 1 Wort
        MCBSP_FMKS(RCR, RWDLEN1, 16BIT)         |	// p AIC erwartet 16 Bit
        MCBSP_FMKS(RCR, RWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt
		/* Sende-Control Register */
        MCBSP_FMKS(XCR, XPHASE, SINGLE)         |	//
        MCBSP_FMKS(XCR, XFRLEN2, DEFAULT)       |	// Länge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XWDLEN2, DEFAULT)       |	// Wortlänge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(XCR, XFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die Übertragung neu
        MCBSP_FMKS(XCR, XDATDLY, 0BIT)          |	// keine Verzögerung (delay) der Daten
        MCBSP_FMKS(XCR, XFRLEN1, OF(1))         |	// Länge der Phase 1 --> 1 Wort
        MCBSP_FMKS(XCR, XWDLEN1, 16BIT)         |	// Wortlänge in Phase 1 --> 16 bit
        MCBSP_FMKS(XCR, XWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt
		/* Sample Rate Generator Register */
        MCBSP_FMKS(SRGR, GSYNC, DEFAULT)        |	// Einstellungen nicht relevant da
        MCBSP_FMKS(SRGR, CLKSP, DEFAULT)        |	// der McBSP1 als Slave läuft
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
        MCBSP_FMKS(PCR, XIOEN, SP)              |	// Pin wird für serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, RIOEN, SP)              |	// Pin wird für serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, FSXM, EXTERNAL)         |	// Framesync- Signal für Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, FSRM, EXTERNAL)         |	// Framesync- Signal für Empfänger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKXM, INPUT)           |	// Takt für Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKRM, INPUT)           |	// Takt für Empfänger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKSSTAT, DEFAULT)      |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, DXSTAT, DEFAULT)        |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, FSXP, ACTIVEHIGH)       |	// Framesync senderseitig ist "activehigh"
        MCBSP_FMKS(PCR, FSRP, ACTIVEHIGH)       |	// Framesync empfängerseitig ist "activehigh"
        MCBSP_FMKS(PCR, CLKXP, FALLING)         |	// Datum wird bei fallender Flanke gesendet
        MCBSP_FMKS(PCR, CLKRP, RISING)			// Datum wird bei steigender Flanke übernommen
};

/* template for a EDMA configuration */
EDMA_Config configEDMARcv = {
    EDMA_FMKS(OPT, PRI, LOW)          |   // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode p Increment
    EDMA_FMKS(OPT, TCINT,YES)         |   // EDMA interrupt erzeugen? p Ja, Interrupt wird nach Sendem von einem Element gesetzt
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

    EDMA_FMK (CNT, FRMCNT, NULL)       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

    (Uint32)Buffer_in_ping,       		  // Ziel-Adresse, p Casten von Short-> Uint32

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

    EDMA_FMK (RLD, ELERLD, NULL)       |  // Reload Element
    EDMA_FMK (RLD, LINK, NULL)            // Reload Link
};

    /* ergänzt */
    EDMA_Config configEDMAXmt = {
        EDMA_FMKS(OPT, PRI, LOW)          |   // auf beide Queues verteilen
        EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size
        EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
        EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
        EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
        EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode p Increment
        EDMA_FMKS(OPT, TCINT,YES)         |   // EDMA interrupt erzeugen? p Ja, Interrupt wird nach Sendem von einem Element gesetzt
        EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
        EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
        EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

        EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

        EDMA_FMK (CNT, FRMCNT, NULL)       |  // Anzahl Frames
        EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

        (Uint32)Buffer_in_ping,       		  // Ziel-Adresse, p Casten von Short-> Uint32

        EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
        EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

        EDMA_FMK (RLD, ELERLD, NULL)       |  // Reload Element
        EDMA_FMK (RLD, LINK, NULL)            // Reload Link
};

/* Transfer-Complete-Codes for EDMA-Jobs */
int tccRcvPing;
//ergänzt:
int tccRcvPong;
int tccXmtPing;
int tccXmtPong;

/* EDMA-Handles; are these really all? */
EDMA_Handle hEdmaRcv;  
EDMA_Handle hEdmaReload; 
							/* ergänzt */
EDMA_Handle hEdmaXmt;
EDMA_Handle hEdmaReload1;
MCBSP_Handle hMcbsp=0;
								
main()
{
	
	CSL_init();  
	
	/* Configure McBSP0 and AIC23 */
	Config_DSK6713_AIC23();
	
	/* Configure McBSP1*/
	hMcbsp = MCBSP_open(MCBSP_DEV1, MCBSP_OPEN_RESET); //Bevor ein McBSP Port verwendet werden kann, muss er erst mit dieser Funktion geöffnet werden
    MCBSP_config(hMcbsp, &datainterface_config);	//Einstellungen übergeben
    
	/* configure EDMA */
    config_EDMA();

    /* finally the interrupts p EDMA_INT schaltet immer um zwischen process_ping_SWI und process_pong_SWI bzw. ruft die Funktionen auf */
    config_interrupts();

    MCBSP_start(hMcbsp, MCBSP_RCV_START, 0xffffffff);	/* tell it what you want to start p start to recieve, Golden wire macht die Ausgabe  ??? */
    MCBSP_write(hMcbsp, 0x0); 	/* one shot */
} /* finished*/


void config_EDMA(void)
{
	/* Konfiguration der EDMA zum Lesen*/
	hEdmaRcv = EDMA_open(EDMA_CHA_REVT1, EDMA_OPEN_RESET);  	// EDMA Channel for REVT1 // open a Channel, open EDMA
	hEdmaReload = EDMA_allocTable(-1);               			// Reload-Parameters

	configEDMARcv.src = MCBSP_getRcvAddr(hMcbsp);         	 //  source addr wird zugewiesen

	tccRcvPing = EDMA_intAlloc(-1);                        // next available TCC p Transfer Complete Code indicate which channel (parameter set) is ready
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);     // set it (shift values)

	/* configure */
	EDMA_config(hEdmaRcv, &configEDMARcv);
	EDMA_config(hEdmaReload, &configEDMARcv);

	tccRcvPong = EDMA_intAlloc(-1);
	configEDMARcv.opt &= 0xFFF0FFF;							//Reset TCC for new TCC
	configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPong);
	
	/* configure */
	EDMA_config(hEdmaRcv, &configEDMARcv);
	EDMA_config(hEdmaReload, &configEDMARcv);
	/* could we need also some other EDMA read job?*/ /*p ja brauchen wir für pong*/

	/* link transfers ping -> pong -> ping */
	EDMA_link(hEdmaRcv,hEdmaReload);  /* is that all? p  EDMA_link(CSL p.161) */
	EDMA_link(hEdmaReload,hEdmaRcv); /*ergänzt*/

	/* do you want to hear music? When yes you are need a EDMA writing*/

	/* p ergänzt*/
	/* Konfiguration der EDMA zum Schreiben*/
	hEdmaXmt = EDMA_open(EDMA_CHA_XEVT1, EDMA_OPEN_RESET);  // EDMA Channel for XEVT1 // open a Channel, open EDMA
	hEdmaReload1 = EDMA_allocTable(-1);               // Reload-Parameters


	configEDMAXmt.src = MCBSP_getXmtAddr(hMcbsp);          //  source addr wird zugewiesen

	tccRcvPing = EDMA_intAlloc(-1);                        // next available TCC p Transfer Complete Code indicate which channel (parameter set) is ready
	configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);     // set it (shift values)

	/* configure */
	EDMA_config(hEdmaXmt, &configEDMAXmt);
	EDMA_config(hEdmaReload1, &configEDMAXmt);

	EDMA_link(hEdmaXmt,hEdmaReload1);
	EDMA_link(hEdmaReload1,hEdmaXmt); /*ergänzt*/
	/* p ergänzt*/

	/* enable EDMA TCC */
	EDMA_intClear(tccRcvPing);
	EDMA_intEnable(tccRcvPing);
	/* some more? p ergänzt*/
	EDMA_intClear(tccXmtPing);
	EDMA_intEnable(tccXmtPing);
	/* p ergänzt*/
	EDMA_intClear(tccRcvPong);
	EDMA_intEnable(tccRcvPong);
	/* p ergänzt*/
	EDMA_intClear(tccXmtPong);
	EDMA_intEnable(tccXmtPong);

	/* which EDMAs do we have to enable? */
	EDMA_enableChannel(hEdmaRcv);
	//ergänzt:
	EDMA_enableChannel(hEdmaXmt);
}

void config_interrupts(void)
{   //Lücken ergänzt
	IRQ_map(IRQ_EVT_EDMAINT, 12);	//ergänzt Interrupt sources (here EDMAINT) and mapping to CPU interrupts (is chosen by 12)
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
		EDMA_intClear(tccRcvPing); /* clear is mandatory p löscht gesetztes Bit (tccRcvPing)*/
		rcvPingDone=1;
	} /* p EDMA_intTest testet, ob der Wert von tccRcvPing (Interrupt number) im CIPR gesetz ist.
		(z.B. tcc 15 (Bei uns eig immer 15) CIRR Bit 15 gesetzt oder nicht)  Rückgabewert:1=flag was set */

	else if(EDMA_intTest(tccRcvPong)) {
		EDMA_intClear(tccRcvPong);
		rcvPongDone=1;
	}

	else if(EDMA_intTest(tccXmtPing)) {     //transmit ping is finished  p ergänzt
			EDMA_intClear(tccXmtPing);
			xmtPingDone=1;
		}
	else if(EDMA_intTest(tccXmtPong)) {     //transmit pong is finished  p ergänzt
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
		
		DSK6713_LED_toggle(1);				//LED 1
	}
}
