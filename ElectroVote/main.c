/* Yavuz Selim YESILYURT 2259166
 * Alper KOCAMAN 2169589
 */

/* Throughout the code, some descriptive comments are present on specific blocks, which describes underlying details about the statement.
 *
 * STRUCTURE AND MAIN RELATIONS
 * 	The structure of our homework is based on enabling 4 interrupt sources and getting action when they gets interrupted.
 * 	In our main routine we have looped with checking if any of the interrupt sources interrupted and if so we have called the corresponding
 *      handler function. In these handler functions we simply acknowledged our internal flag and done what is needed to do when that kind of
 *      interrupt comes. Along with these our basic task was checking "if adcDone is set then acknowledge it and call changeCandidate()" 
 *
 * 	In our ISR whenever we have waited for PORTB Change Interrupt, TMR0 Overflow Interrupt, TMR1 Overflow Interrupt and ADC Module Interrupt.
 *      Whenever we get an interrupt we set its corresponding flag.
 *
 * 	Initially we have initialized and configured our Timers, ADC module, LCD and buttons
 *      (Details of how we configured Timers, ADC and others are described in their related init functions)
 *
 * 	Then we have waited for RE1 button push and release, as soon as it's done we have waited for 3 seconds and changed our state to "EnterVoters"
 *
 * 	In "EnterVoters" phase we have modified our LCD and set our Voter Number with PORTB Change interrupts
 *
 * 	After "EnterVoters" phase we have switched to "VotingPhase", where we have waited for correspnding interrupts to happen and change our LCD and
 *      7 Segment Displays accordingly as long as there is some remaining time and remaining voters.
 *
 * 	After voting phase we have closed timer1 and started to loop for Timer0 and ADC interrupts and update LCD accordingly.
 *
 * FOR SUBROUTINE DESCRIPTIONS
 *  All subroutines have their descriptions in themselves as comments on first couple of lines in their block.
 *  Below describes all subroutines with their relations to each other with brief descriptions:
 *     -> enterVoters -> function for entering voters phase
 *     -> votingPhase -> function for entering voting phase
 *     -> RBPressed -> PORTB Change interrupt function, called by both enterVoters and votingPhase functions
 *     -> TMR1Interrupted -> TMR1 interrupt function, called by both enterVoters and votingPhase functions
 *     -> TMR0Interrupted -> TMR0 interrupt function, called by both enterVoters, votingPhase and in main
 *     -> ADCInterrupted -> ADC interrupt function, called by both enterVoters, votingPhase and in main
 *     -> changeCandidate -> Function for changing current candidate shown on LCD, called in main whenever adcDone is set
 *     -> incrementVoterNum -> Increments initial voter number on EnterVoters phase
 *     -> startVoting -> Function for handling the some minor changes before switching to VotingPhase
 *     -> vote -> Function for voting current candidate shown on LCD
 *     -> blinkCursor -> Function that blinks the cursor with 250ms intervals that is altered with TMR0
 *     -> startADC -> Function for starting ADC Conversion, which enables AD Interrupts and starts conversion (with calling delay() for waiting acquisition time interval of AD Module)
 *     -> readADC -> Function for reading converted AD value into "adcValue" variable from ADRES registers
 *     -> updateLCDTime -> Decrements time on LCD with 1s intervals that is provided with TMR1
 *     -> resolveCandidateId -> Provides a lookup table for learning CandidateId with corresponding AD value
 *     -> delay -> Provides a delay for acquisition time intervak of AD Module (Delay is provided with TMR0)
 *     -> delay3sec -> Provides a busy wait delay for 3 seconds at initial wait
 *     -> initLCD -> inits LCD
 *     -> initTimer -> inits Timers (details are written in function)
 *     -> initADC -> inits AD Module (details are written in function)
 *     -> init -> inits buttons (details are written in function)
 *     -> refresh7Segment -> Function for periodic switch between digits of 7 Segment Displays (Called in a number of places including LCD functions for handling flickering on displays)
 *
 * SOME IMPORTANT GLOBAL VARIABLES
 * 	adcDone -> set by ADCInterrupted function after calling readADC and cleared by votingPhase/main in loop
 				, to ensure that candidate gets scrolled up/down in LCD in accordance with newly updated adcValue
 *  adcValue -> set by readADC function after ADC conversion and checked with resolveCandidateId function
 */


#include <p18cxxx.h>
#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#define _XTAL_FREQ   40000000

#include "INCLUDE.h"
#include "LCD.h"

/* Global Variables */ 
unsigned int adcDone = 0, adcValue = 0, remainingVoters = 0, onesRemainingVoters = 0, remainingTime = 90 , portBval = 0, tens = 1, votingStarted = 0,
			 tmr0Count = 0, portBpressed = 0, tmr0Interrupted = 0,tmr1Interrupted = 0, adcInterrupted = 0, tmr1Count = 0, blink = 1;

unsigned int poeVote = 0, galibVote = 0, selimiVote = 0, nesimiVote = 0, hatayiVote = 0, zweigVote = 0, nabiVote = 0, austenVote = 0,
			 currCandidateId = 0, mostVotedId = 0, mostVoted = 0, votingFinished = 0 ;
unsigned int disp0 = 0, disp1 = 0, disp2 = 0, disp3 = 0;  
unsigned char ch;

void initTimer(){
	/* Configuration for Timers */

	/* Configure Timer0 as b'01000111', which means:
	 * Initially disabled, 8-bit mode, Prescaler assigned and set to 1:256 
	 * Furthermore, to ensure that it overflows for every 1ms preload it with 220 
	 * (It was 217 but we altered it a bit for providing exactness other code segments' delays)
	 */
	T0CON = 71 ; 
	TMR0 = 220 ; 

	/* Open Timer0 Interrupt Enable, to get its interrupt after timer start */
	TMR0IE = 1 ; 

	/* Configure Timer1 as b'11100000', which means:
	 * Initially disabled, 16-bit mode, Prescaler assigned and set to 1:4 
	 * Furthermore, to ensure that it overflows for every 20ms preload it with 15535 
	 */
	T1CON = 224 ; 
	TMR1 = 15535 ;

	/* Open Timer0 Interrupt Enable, to get its interrupt after timer start */
	TMR1IE = 1 ; 
}

void initADC(){
	/* Configuration for ADC Module */

	/* ADCON0 -> b'00110000', which means, we have used 12th channel and GODONE, ADON is disabled for now
	 * ADCON1 -> b'00000000', which means, we have set all channels as analog input channels
	 * ADCON2 -> b'10101010', which means, we have used 12Tad for Acquisition Time and Fosc/32 for Conversion Time
	 */
	TRISHbits.TRISH4 = 1 ;
	ADCON0 = 48 ; 
	ADCON1 = 0 ;   
	ADCON2 = 170 ;   

	/* Enable ADON bit for Usage of ADC Module in future */    
	ADON = 1 ; 	  
}

void init(){
	/* Configuration for Buttons */

	/* Required buttons are configured as inputs */
	TRISB6 = 1;   
	TRISB7 = 1;   
	TRISE1 = 1;   

	/* GIE and RBIE Interrupts are enabled at the end of initializations and configurations */
	RBIE = 1 ;
	GIE  = 1 ;
}

void delay3sec(void) { 
	/* Function for Initial 3 Seconds wait after RE1 pressed & released */

	unsigned int i=244, j=100, k=4649;
	
	while(i > 0){
		i--;
		while(j > 0){
			j--;
			while(k > 0){
				k--;
			}
			k = 100;
		}
		j = 100;
	}
}

void delay() { 
	/* Function providing a delay which is controlled by TMR0
	 * when in startADC, there should be a delay in order to take the digitized values correctly
	 * this delay is 125 clock cycle for the acquisition time  
	 */

	unsigned int temp ; // in order to take a current TMR0 value , a local variable is created 

	if(TMR0 < 130) {   // If value is less than 130 , there will be no overflow 
		temp = TMR0 ;   // take TMR0 value exactly
		while(temp+126 > TMR0) ; // if TMR0 value exceeds the temp+125 , break the while loop  
		}

	else { 
		temp = TMR0 -126 ; // If value is bigger than 130, there will be overflow
		while( TMR0 > temp ) ; // firstly, TMR value is bigger than temp
		while( TMR0 < temp ) ; // when TMR0 overflows, TMR0 should pass the value of temp in order to accomplish delay
	}	
}

void startADC() {
	/* Function for starting ADC module 
	 * AD Interrupt gets enabled and AD Conversion gets started
	 * delay() Function called for waiting Acquisition time interval before conversion
	 */
	ADIE = 1;
	GODONE = 1;
	delay();
}

void readADC() {
	/* Function for reading ADC value from ADRESH and ADRESL registers */
	adcValue = ((ADRESH & 2) / 2) * 512 + (ADRESH & 1) * 256 + ADRESL;
}

void refresh7Segment(){ 
	/* Function for periodic switch between digits of 7 Segment Displays */

	if(LATH0) {

		LATH0 = 0 ;
		LATH3 = 1 ;

		LATJ = disp0 ;

	}

	else if(LATH3) {

		LATH3 = 0 ;
		LATH2 = 1 ;

		LATJ = disp1 ;
	}

	else if(LATH2) {

		LATH2 = 0 ;
		LATH1 = 1 ;

		LATJ = disp2 ;
	}

	else if(LATH1) {

		LATH1 = 0 ;
		LATH0 = 1 ;

		LATJ = disp3 ;
	}
}

void changeCandidate() {
	/* Function for changing candidates whenever ADC conversion ends 
	 * (Whenever User scrolls up/down the potentiometer)
	 * (Updates second line of LCD during voting period)
	 */

	currCandidateId = resolveCandidateId() ;

	switch(currCandidateId){
		case 18:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 18 Poe    :");
            refresh7Segment();
    		ch = (char)(((int)'0') + poeVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + poeVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
		case 64:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 64 Galib  :");
            refresh7Segment();
    		ch = (char)(((int)'0') + galibVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + galibVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
		case 34:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 34 Selimi :");
            refresh7Segment();
    		ch = (char)(((int)'0') + selimiVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + selimiVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
		case 23:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 23 Nesimi :");
            refresh7Segment();
    		ch = (char)(((int)'0') + nesimiVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + nesimiVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
		case 33:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 33 Hatayi :");
    		ch = (char)(((int)'0') + hatayiVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + hatayiVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
		case 67:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 67 Zweig  :");
            refresh7Segment();
    		ch = (char)(((int)'0') + zweigVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + zweigVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
            
    		break ;
		case 63:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 63 Nabi   :");
            refresh7Segment();
    		ch = (char)(((int)'0') + nabiVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + nabiVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
		case 99:
			WriteCommandToLCD(0xC2); // Goto to the 3rd place of the second line
            refresh7Segment();
    		WriteStringToLCD(" 99 Austen :");
            refresh7Segment();
     		ch = (char)(((int)'0') + austenVote/10);
    		WriteCommandToLCD(0xCD); // Goto to the 14th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		ch = (char)(((int)'0') + austenVote%10);
    		WriteCommandToLCD(0xCE); // Goto to the 15th place of the second line
    		WriteDataToLCD(ch);
            refresh7Segment();
    		break ;
  		default :
			/* if execution reaches here then there is a bug */
   			break ; 
	}
}

void updateLCDTime() {
	/* Function for decrementing remaining time for voting period on LCD 
	 * (Gets called whenever 1 Second period ends)
	 */
	unsigned int ones = remainingTime % 10;
	unsigned int tens = remainingTime / 10;

	WriteCommandToLCD(0x8E); // Goto to the beginning of the ones digit of time
	switch (ones) {
		case 0:
			WriteDataToLCD('0');
			break;
		case 1:
			WriteDataToLCD('1');
			break;
		case 2:
			WriteDataToLCD('2');
			break;
		case 3:
			WriteDataToLCD('3');
			break;
		case 4:
			WriteDataToLCD('4');
			break;
		case 5:
			WriteDataToLCD('5');
			break;
		case 6:
			WriteDataToLCD('6');
			break;
		case 7:
			WriteDataToLCD('7');
			break;
		case 8:
			WriteDataToLCD('8');
			break;
		case 9:
			WriteDataToLCD('9');
			break;
		default:
			/* if execution reaches here then there is a bug */
			break;
	}

	WriteCommandToLCD(0x8D); // Goto to the beginning of the tens digit of time
	switch (tens) {
		case 0:
			WriteDataToLCD('0');
			break;
		case 1:
			WriteDataToLCD('1');
			break;
		case 2:
			WriteDataToLCD('2');
			break;
		case 3:
			WriteDataToLCD('3');
			break;
		case 4:
			WriteDataToLCD('4');
			break;
		case 5:
			WriteDataToLCD('5');
			break;
		case 6:
			WriteDataToLCD('6');
			break;
		case 7:
			WriteDataToLCD('7');
			break;
		case 8:
			WriteDataToLCD('8');
			break;
		case 9:
			WriteDataToLCD('9');
			break;
		default:
			/* if execution reaches here then there is a bug */
			break;
	}
}

void blinkCursor() {
	/* Function for blinking cursor with 250ms intervals
	 * If C1 cell has '>' make it ' ' and vice versa 
	 * (Gets called whenever 250ms period ends)
	 */

	if (blink) {
		WriteCommandToLCD(0xC1); // Goto to '>' character
		WriteDataToLCD(' ');
		blink = 0;
	}
	else {
		WriteCommandToLCD(0xC1); // Goto to ' ' character
		WriteDataToLCD('>');
		blink = 1;
	}
}

int resolveCandidateId() {
	/* Helper Function for providing a lookup table for resolution of candidate ids  */
	if (adcValue <= 127) 
		return 18;
	else if (adcValue <= 255) 
		return 64;
	else if (adcValue <= 383) 
		return 34;
	else if (adcValue <= 511) 
		return 23;
	else if (adcValue <= 639) 
		return 33;
	else if (adcValue <= 767) 
		return 67;
	else if (adcValue <= 895) 
		return 63;
	else if (adcValue <= 1023) 
		return 99;
}

void vote() {
	/* Function for voting current candidate
	 * (Gets called whenever RB7 interrupt comes and an increment 
	 *  in vote of current candidate needs to happen)
	 */
	int update7Seg = 0;
	switch (currCandidateId) {
		case 18:
			if (poeVote == 99) // break if 99 vote is reached
				break;
			poeVote++;
			if (poeVote > mostVoted) {
				update7Seg = 1;
				mostVoted = poeVote;
				mostVotedId = 18;
			}
			break;
		case 64:
			if (galibVote == 99) // break if 99 vote is reached
				break;
			galibVote++;
			if (galibVote > mostVoted) {
				update7Seg = 1;
				mostVoted = galibVote;
				mostVotedId = 64;
			}
			break;
		case 34:
			if (selimiVote == 99) // break if 99 vote is reached
				break;
			selimiVote++;
			if (selimiVote > mostVoted) {
				update7Seg = 1;
				mostVoted = selimiVote;
				mostVotedId = 34;
			}
			break;
		case 23:
			if (nesimiVote == 99) // break if 99 vote is reached
				break;
			nesimiVote++;
			if (nesimiVote > mostVoted) {
				update7Seg = 1;
				mostVoted = nesimiVote;
				mostVotedId = 23;
			}
			break;
		case 33:
			if (hatayiVote == 99) // break if 99 vote is reached
				break;
			hatayiVote++;
			if (hatayiVote > mostVoted) {
				update7Seg = 1;
				mostVoted = hatayiVote;
				mostVotedId = 33;
			}
			break;
		case 67:
			if (zweigVote == 99) // break if 99 vote is reached
				break;
			zweigVote++;
			if (zweigVote > mostVoted) {
				update7Seg = 1;
				mostVoted = zweigVote;
				mostVotedId = 67;
			}
			break;
		case 63:
			if (nabiVote == 99) // break if 99 vote is reached
				break;
			nabiVote++;
			if (nabiVote > mostVoted) {
				update7Seg = 1;
				mostVoted = nabiVote;
				mostVotedId = 63;
			}
			break;
		case 99:
			if (austenVote == 99) // break if 99 vote is reached
				break;
			austenVote++;
			if (austenVote > mostVoted) {
				update7Seg = 1;
				mostVoted = austenVote;
				mostVotedId = 99;
			}
			break;
		default:
			/* if execution reaches here then there is a bug */
			break;
	}

	if (update7Seg) {
		/* Most voted candidate has been changed -- Update 7 Segment Display */

		disp3 = mostVotedId / 10;
		disp2 = mostVotedId % 10;
		disp1 = mostVoted / 10;
		disp0 = mostVoted % 10;

		switch(disp3){
			case 0: 
				disp3 = 63  ;// b'00111111'
				break ;
			case 1: 
				disp3 = 6   ;// b'00000110'
				break ;
			case 2: 
				disp3 = 91  ;// b'01011011'
				break ;
			case 3: 
				disp3 = 79  ;// b'01001111'
				break ;
			case 4: 
				disp3 = 102 ;// b'01100110'
				break ;
			case 5: 
				disp3 = 109 ;// b'01101101'
				break ;
			case 6: 
				disp3 = 125 ;// b'01111101'
				break ;
			case 7: 
				disp3 = 7   ;// b'00000111'
				break ;
			case 8: 
				disp3 = 127 ;// b'01111111'
				break ;
			case 9: 
				disp3 = 111 ;// b'01101111'
				break ;
			default:
				/* if execution reaches here then there is a bug */
				break ; 

		}

		switch(disp2){
			case 0: 
				disp2 = 63  ;// b'00111111'
				break ;
			case 1: 
				disp2 = 6   ;// b'00000110'
				break ;
			case 2: 
				disp2 = 91  ;// b'01011011'
				break ;
			case 3: 
				disp2 = 79  ;// b'01001111'
				break ;
			case 4: 
				disp2 = 102 ;// b'01100110'
				break ;
			case 5: 
				disp2 = 109 ;// b'01101101'
				break ;
			case 6: 
				disp2 = 125 ;// b'01111101'
				break ;
			case 7: 
				disp2 = 7   ;// b'00000111'
				break ;
			case 8: 
				disp2 = 127 ;// b'01111111'
				break ;
			case 9: 
				disp2 = 111 ;// b'01101111'
				break ;
			default:
				/* if execution reaches here then there is a bug */
				break ; 
		}

		switch(disp1){
			case 0: 
				disp1 = 63  ;// b'00111111'
				break ;
			case 1: 
				disp1 = 6   ;// b'00000110'
				break ;
			case 2: 
				disp1 = 91  ;// b'01011011'
				break ;
			case 3: 
				disp1 = 79  ;// b'01001111'
				break ;
			case 4: 
				disp1 = 102 ;// b'01100110'
				break ;
			case 5: 
				disp1 = 109 ;// b'01101101'
				break ;
			case 6: 
				disp1 = 125 ;// b'01111101'
				break ;
			case 7: 
				disp1 = 7   ;// b'00000111'
				break ;
			case 8: 
				disp1 = 127 ;// b'01111111'
				break ;
			case 9: 
				disp1 = 111 ;// b'01101111'
				break ;
			default:
				/* if execution reaches here then there is a bug */
				break ; 

		}

		switch(disp0){
			case 0: 
				disp0 = 63  ;// b'00111111'
				break ;
			case 1: 
				disp0 = 6   ;// b'00000110'
				break ;
			case 2: 
				disp0 = 91  ;// b'01011011'
				break ;
			case 3: 
				disp0 = 79  ;// b'01001111'
				break ;
			case 4: 
				disp0 = 102 ;// b'01100110'
				break ;
			case 5: 
				disp0 = 109 ;// b'01101101'
				break ;
			case 6: 
				disp0 = 125 ;// b'01111101'
				break ;
			case 7: 
				disp0 = 7   ;// b'00000111'
				break ;
			case 8: 
				disp0 = 127 ;// b'01111111'
				break ;
			case 9: 
				disp0 = 111 ;// b'01101111'
				break ;
			default:
				/* if execution reaches here then there is a bug */
				break ; 

		}

	}
	// Do not forget to decrement remainingVoters
	remainingVoters--;
}

void startVoting() {
	/* Function for handling the initial configuration when the voting starts */

	ClearLCDScreen();

	votingStarted = 1; 

	// Determine VoterNumber (remainingVoters is tens and onesRemainingVoters is ones coefficient of VoterNumber)
	remainingVoters += onesRemainingVoters;

	// Set Initial Configuration for 7 Segment Display 
	// (make all Displays " - - - - ")
	LATJ = 64 ; // b'01000000' 
	disp0 = disp1 = disp2 = disp3 = 64 ; 
	LATH = 15 ; // open 4 displays

	// Read Initial value of ADC for showing first candidate on LCD
	readADC();
	currCandidateId = resolveCandidateId();

	// Open Timers, since Voting starts from now on
	TMR0ON = 1;
	TMR1ON = 1;

	WriteCommandToLCD(0x80); // Goto to the beginning of the first line
	WriteStringToLCD("  Time left :90 ");
	changeCandidate();
}

void incrementVoterNum() {
	/* Function for incrementing for the initial altering of voter number before start of voting */

	if (tens) {
		/* increment tens digit of voter number */
		switch (remainingVoters) {
			case 0:
				remainingVoters = 10;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 10             ");
				break;
			case 10:
				remainingVoters = 20;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 20             ");
				break;
			case 20:
				remainingVoters = 30;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 30             ");
				break;
			case 30:
				remainingVoters = 40;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 40             ");
				break;
			case 40:
				remainingVoters = 50;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 50             ");
				break;
			case 50:
				remainingVoters = 60;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 60             ");
				break;
			case 60:
				remainingVoters = 70;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 70             ");
				break;
			case 70:
				remainingVoters = 80;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 80             ");
				break;
			case 80:
				remainingVoters = 90;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 90             ");
				break;
			case 90:
				remainingVoters = 0;
				WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
				WriteStringToLCD(" 00             ");
				break;
			default:
				/* if execution reaches here then there is a bug */
				break;
		}
	}
	else {
		/* increment ones digit of voter number */
		switch (onesRemainingVoters) {
			case 0:
				onesRemainingVoters = 1;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('1');
				break;
			case 1:
				onesRemainingVoters = 2;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('2');
				break;
			case 2:
				onesRemainingVoters = 3;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('3');
				break;
			case 3:
				onesRemainingVoters = 4;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('4');
				break;
			case 4:
				onesRemainingVoters = 5;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('5');
				break;
			case 5:
				onesRemainingVoters = 6;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('6');
				break;
			case 6:
				onesRemainingVoters = 7;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('7');
				break;
			case 7:
				onesRemainingVoters = 8;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('8');
				break;
			case 8:
				onesRemainingVoters = 9;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('9');
				break;
			case 9:
				onesRemainingVoters = 0;
				WriteCommandToLCD(0xC2); // Goto to the second digit
				WriteDataToLCD('0');
				break;
			default:
				/* if execution reaches here then there is a bug */
				break;
		}
	}
}

void RBPressed() {
	/* Function for handling RB Interrupts whenever an RB interrupt happens
	 * "portBpressed" flag is used for catching the interrupt.
	 */

	portBpressed = 0;
    
	if(((portBval & 0x40) == 0x40) && !votingStarted) {
		/* Check if RB6 pressed */
        
        while (PORTBbits.RB6) // Wait until RB6 is released and during this waiting refresh 7 Segment
            refresh7Segment();

		incrementVoterNum();
	}
	else if((portBval & 0x80) == 0x80) {
		/* Check if RB7 pressed */
        while (PORTBbits.RB7) // Wait until RB6 is released and during this waiting refresh 7 Segment
            refresh7Segment();
            
		if(!tens){
			if(!votingStarted) {
				/* Very second press to RB7, start voting */
				startVoting();
			}
			else {
				/* Vote current candidate */
				vote();
			}
		}
		else {
			/* Very first press to RB7, switch to ones digit */
			tens = 0;
		}
	}
}

void TMR0Interrupted() {
	/* Function for handling TMR0 Interrupts whenever TMR0 overflows and interrupts
	 * "tmr0Interrupted" flag is used for catching the interrupt.
	 */

	// Dont forget to preload TMR0 again
    TMR0 = 220 ;
    
	tmr0Interrupted = 0 ;
    
    // We keep count of interrupts to get the desired delay after a number of overflows
	tmr0Count++ ;
    
    /* Call blinkCursor() whenever 250 ms elapses
     * Actually we were testing tmr0Count in here with 250 but delays seemed 
     * to have more accurate values when we count up to 86 
     */
	if ((tmr0Count % 86 == 0) && !votingFinished) {
		/* 250 ms elapsed */
		blinkCursor();
	}
	
    /* Start ADC conversion whenever 100 ms elapses
     * Actually we were testing tmr0Count in here with 100 but delays seemed 
     * to have more accurate values when we count up to 32 
     */
	if(tmr0Count % 32 == 0) {
		/* 100 ms elapsed */
		startADC() ;
	}

    // Reset tmr0Count on lcm of 86 and 32
	if (tmr0Count == 1376) {
		/* Reset tmr0Count to avoid overflows on tmr0Count */
		tmr0Count = 0 ;
	}

	/* Refresh 7 Segment Display for every 1ms */
	refresh7Segment();
}

void TMR1Interrupted() {
	/* Function for handling TMR1 Interrupts whenever TMR1 overflows and interrupts
	 * "tmr1Interrupted" flag is used for catching the interrupt.
	 */

	// Dont forget to preload TMR1 again
    TMR1 = 15535;

	tmr1Interrupted = 0 ;
    
    // We keep count of interrupts to get the desired delay after a number of overflows
	tmr1Count++ ;

    /* Update Timer on LCD Screen whenever 1s elapses
     * Actually we were testing tmr1Count in here with 50 but delays seemed 
     * to have more accurate values when we count up to 21 
     */
	if (tmr1Count == 21) {
		/* 1 Second elapsed */
		remainingTime--;
		updateLCDTime();
        // reset count at every 1 second
		tmr1Count = 0;
	}
}

void ADCInterrupted() {
	/* Function for handling ADC Interrupts whenever ADC finishes AD conversion and interrupts
	 * "adcInterrupted" flag is used for catching the interrupt.
	 */

	adcInterrupted = 0 ;

	/* Read AdcValue from ADRES Registers by calling readADC() */
	readADC() ;

	/* Set adcDone flag to ensure that candidate gets scrolled up/down in LCD */
	adcDone = 1 ;
}

void __interrupt() isr(void) {
	/* In ISR we checked 4 interrupt flags consecutively, 
	 * set our own internal interrupt flags accordingly and acknowledged the interrupt flags 
	 * "portBpressed" flag is used for catching  PORTB change interrupt.
	 * "tmr0Interrupted" flag is used for catching Timer0 interrupt.
	 * "tmr1Interrupted" flag is used for catching Timer1 interrupt.
	 * "adcInterrupted" flag is used for catching ADC interrupt.
	 */ 
    
	if(RBIF == 1 ){ 
		/* For PORTB change interrupt */

        portBpressed = 1 ;
		RBIF = 0 ;
        /* PORTB value is stored to later determine which button created the interrupt */
        portBval = PORTB ;
	}

	if(TMR0IF == 1) {
		/* For Timer0 interrupt */
		tmr0Interrupted = 1 ;
		TMR0IF = 0 ;
	}

	if(TMR1IF == 1) {
		/* For Timer1 interrupt */
		tmr1Interrupted = 1 ;
		TMR1IF = 0 ;
	}

	if(ADIF == 1) {
		/* For ADC interrupt */
		adcInterrupted = 1 ;
		ADIF = 0 ;

		/* AD Interrupt enable bit is cleared to avoid 
		 * further interrupts from ADC Module before 100ms 
		 */
		ADIE = 0 ;
	}
}

void enterVoters() {
	/* Function for initial entering voters phase after 3 seconds waiting period */

	/* Change LCD Screen for Alterings of Voter Number */	
	WriteCommandToLCD(0x80); // Goto to the beginning of the first line
	WriteStringToLCD(" #Enter Voters# ");
	WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
	WriteStringToLCD(" 00             ");
	    
    /* Until Voting phase starts, handle all voter number alterations with PORTB Change Interrupts */
	while(!votingStarted) {
		if(portBpressed) 
			RBPressed();
	} 
}

void votingPhase() {
	/* Function for Voting phase after "EnterVoters" phase */

    TMR0ON = 1;
    TMR1ON = 1;
    
	/* While there is remainingVoters and remainingTime handle Voting Phase */
	while(remainingVoters && remainingTime) { 

		/* Check for voting current candidate */
		if(portBpressed) 
			RBPressed();

		/* Check for 250ms and 100ms time Intervals (Blinking of Cursor and starting ADC) */
		if(tmr0Interrupted)
			TMR0Interrupted();

		/* Check for 1s time Interval (Decrementing LCD Time) */
		if(tmr1Interrupted)
			TMR1Interrupted();

		/* Check for Detecting the end of ADC Conversion */
		if(adcInterrupted)
			ADCInterrupted();

		/* Update LCD according to newly converted value of ADC and Change Candidate in LCD if needed */
		if(adcDone) {
			changeCandidate();
			adcDone = 0;
		}
	}

	/* Disable TMR1 since we dont need it anymore and set finish flag */
	TMR1ON = 0;
	votingFinished = 1;
}

void main (void){

	/* Clear interrupt registers initially to avoid Interrupts during initializations
	 * (Interrupts are reenabled at the end of the initializations, at init() function) 
	 */
	INTCON = 0;
	RCON = 0;

	ClearLCDScreen();

	/* Initialize important components consecutively */
	InitLCD();
	initTimer();
	initADC();
	init();
	
	/* Wait for RE1 button push and release */
	while(!PORTEbits.RE1);
	while(PORTEbits.RE1);
	
	/* Wait for 3 seconds */
	delay3sec();

	/* Go to Enter Voters Phase */
	enterVoters();

	/* Go to Voting Phase */
	votingPhase();

	/* After Voting phase allow user to scroll up/down showing information about
	 * candidates, stop LCD timer and show the winner on 7 Segment Displays 
	 */
	while (1) {

		/* Check for 100ms time Intervals (starting ADC) */
		if(tmr0Interrupted)
			TMR0Interrupted();
		
		/* Check for Detecting the end of ADC Conversion */
		if(adcInterrupted)
			ADCInterrupted();
		
		/* Update LCD according to newly converted value of ADC and Change Candidate in LCD if needed */
		if(adcDone) {
			changeCandidate();
			adcDone = 0;
		}
	}
}
