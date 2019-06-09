LIST    P=18F8722

#INCLUDE <p18f8722.inc> 
    
CONFIG OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

 last_led       udata 0X20
 last_led
 delay_variable1       udata 0X22
 delay_variable1
 delay_variable2       udata 0X24
 delay_variable2
 delay_variable3       udata 0X26
 delay_variable3
 ra4_pressed	    udata 0x28
 ra4_pressed	       

ORG     0x00
goto    main

init
    movlw 0
    movwf last_led 
    movwf ra4_pressed
    movlw h'F0' 
    movwf TRISA 
    clrf  LATA 
    clrf PORTA;
    movlw h'0F' ; Configure A/D
    movwf ADCON1 ; for digital inputs

    movlw h'00' 
    movwf TRISB 
    clrf  LATB 

    movlw h'00' 
    movwf TRISC 
    clrf  LATC 

    movlw h'00' 
    movwf TRISD 
    clrf  LATD 
    
    clrf PORTE;

    movlw h'0F'
    movwf LATA 
    movwf LATB 
    movwf LATC 
    movwf LATD 
    call delay ; delay 1s
    call delay ; delay 1s
    movlw h'00'
    movwf LATA 
    movwf LATB 
    movwf LATC 
    movwf LATD 
    call delay ; delay 1s after closing too

    return

delay ; procedure for delaying 1 second
    movlw h'35' 
    movwf delay_variable1
    movwf delay_variable2
    movwf delay_variable3
    loop1:
	decfsz delay_variable1
	goto loop2
	goto delayexit1
	loop2:
	    decfsz delay_variable2
	    goto loop3
	    goto loop1
	    loop3:
		decfsz delay_variable3
		goto loop3
		goto loop2
    delayexit1:
	clrf delay_variable1
	clrf delay_variable2
	clrf delay_variable3
	return
	
delay2 ; procedure for delaying 0.5 second (533 ms)
    movlw h'08' 
    movwf delay_variable1
    movwf delay_variable2
    movwf delay_variable3
    loop4:
        movlw 1
	cpfseq ra4_pressed 
	goto noper1
	btfsc PORTA,4 ; if flag is true, then check if RA4 is released (skip next inst if released)
	goto dtask1
	movlw 0
	movwf ra4_pressed 
	goto state2
	noper1:
	    nop
	    nop
	    nop
	dtask1:
	    btfsc PORTA,4 ; Check if RA4 is pressed (skip whenever it is not pressed)
	    movwf ra4_pressed ; if pressed update flag
	    decfsz delay_variable1 ; else carry on w/task
	    goto loop5
	    goto delayexit2
	    loop5:
	        movlw 1
		cpfseq ra4_pressed 
		goto noper2
		btfsc PORTA,4 ; if flag is true, then check if RA4 is released (skip next inst if released)
		goto dtask2
		movlw 0
		movwf ra4_pressed 
		goto state2
		noper2:
		    nop
		    nop
		    nop
		dtask2:
		    btfsc PORTA,4 ; Check if RA4 is pressed (skip whenever it is not pressed)
		    movwf ra4_pressed ; if pressed update flag
		    decfsz delay_variable2 ; else carry on w/task
		    goto loop6
		    goto loop4
		    loop6:
			movlw 1
			cpfseq ra4_pressed 
			goto noper3
			btfsc PORTA,4 ; if flag is true, then check if RA4 is released (skip next inst if released)
			goto dtask3
			movlw 0
			movwf ra4_pressed 
			goto state2
			noper3:
			    nop
			    nop
			    nop
			dtask3:
			    btfsc PORTA,4 ; Check if RA4 is pressed (skip whenever it is not pressed)
			    movwf ra4_pressed ; if pressed update flag
			    decfsz delay_variable3 ; else carry on w/task
			    goto loop6
			    goto loop5
    delayexit2:
	clrf delay_variable1
	clrf delay_variable2
	clrf delay_variable3
	return
    
state1:
    ; inf loop in switch table for state1
    ; go to state2 whenever RA4 release or all leds are turned on
    movlw 1
    cpfseq ra4_pressed 
    goto s1task
    btfsc PORTA,4 ; if flag is true, then check if RA4 is released (skip next inst if released)
    goto s1task
    movlw 0
    movwf ra4_pressed 
    goto state2
    s1task:
	btfsc PORTA,4 ; Check if RA4 is pressed (skip whenever it is not pressed)
	movwf ra4_pressed ; if pressed update flag
	movlw 0 ; else carry on w/task
	cpfseq last_led 
	goto skip0
	goto case0_1
skip0:
    movlw 1
    cpfseq last_led 
    goto skip1
    goto case1_1
skip1:
    movlw 2
    cpfseq last_led 
    goto skip2
    goto case2_1
skip2:
    movlw 3
    cpfseq last_led 
    goto skip3
    goto case3_1
skip3:
    movlw 4
    cpfseq last_led 
    goto skip4
    goto case4_1
skip4:
    movlw 5
    cpfseq last_led 
    goto skip5
    goto case5_1
skip5:
    movlw 6
    cpfseq last_led 
    goto skip6
    goto case6_1
skip6:
    movlw 7
    cpfseq last_led 
    goto skip7
    goto case7_1
skip7:
    movlw 8
    cpfseq last_led 
    goto skip8
    goto case8_1
skip8:
    movlw 9
    cpfseq last_led 
    goto skip9
    goto case9_1
skip9:
    movlw 10
    cpfseq last_led 
    goto skip10
    goto case10_1
skip10:
    movlw 11
    cpfseq last_led 
    goto skip11
    goto case11_1
skip11:
    movlw 12
    cpfseq last_led 
    goto skip12
    goto case12_1
skip12:
    movlw 13
    cpfseq last_led 
    goto skip13
    goto case13_1
skip13:
    movlw 14
    cpfseq last_led 
    goto skip14
    goto case14_1
skip14:
    movlw 15
    cpfseq last_led 
    goto skip15
    goto case15_1
skip15:
    movlw 16
    cpfseq last_led 
    goto state1 ; THERE IS AN ERROR IF CONTROL REACHES HERE!!
    goto case16_1

state2:
    ; check for RE3 or RE4 in inf loop
    ; if RE3 goto state1
    ; else if RE4 goto state3 
    btfsc PORTE,3 ; Check if RE3 pressed (skip whenever it is not pressed)
    goto finishPressRE3
    btfsc PORTE,4 ; Check if RE4 pressed (skip whenever it is not pressed)
    goto finishPressRE4
    goto state2

state3:
    ; inf loop in switch table for state3
    ; go to state2 whenever RA4 release or all leds are turned on
    movlw 1
    cpfseq ra4_pressed 
    goto s3task
    btfsc PORTA,4 ; if flag is true, then check if RA4 is released (skip next inst if released)
    goto s3task
    movlw 0
    movwf ra4_pressed 
    goto state2
    s3task:
	btfsc PORTA,4 ; Check if RA4 is pressed (skip whenever it is not pressed)
	movwf ra4_pressed ; if pressed update flag
	movlw 0 ; else carry on w/task
	cpfseq last_led 
	goto skip0_2
	goto case0_2
skip0_2:
    movlw 1
    cpfseq last_led 
    goto skip1_2
    goto case1_2
skip1_2:
    movlw 2
    cpfseq last_led 
    goto skip2_2
    goto case2_2
skip2_2:
    movlw 3
    cpfseq last_led 
    goto skip3_2
    goto case3_2
skip3_2:
    movlw 4
    cpfseq last_led 
    goto skip4_2
    goto case4_2
skip4_2:
    movlw 5
    cpfseq last_led 
    goto skip5_2
    goto case5_2
skip5_2:
    movlw 6
    cpfseq last_led 
    goto skip6_2
    goto case6_2
skip6_2:
    movlw 7
    cpfseq last_led 
    goto skip7_2
    goto case7_2
skip7_2:
    movlw 8
    cpfseq last_led 
    goto skip8_2
    goto case8_2
skip8_2:
    movlw 9
    cpfseq last_led 
    goto skip9_2
    goto case9_2
skip9_2:
    movlw 10
    cpfseq last_led 
    goto skip10_2
    goto case10_2
skip10_2:
    movlw 11
    cpfseq last_led 
    goto skip11_2
    goto case11_2
skip11_2:
    movlw 12
    cpfseq last_led 
    goto skip12_2
    goto case12_2
skip12_2:
    movlw 13
    cpfseq last_led 
    goto skip13_2
    goto case13_2
skip13_2:
    movlw 14
    cpfseq last_led 
    goto skip14_2
    goto case14_2
skip14_2:
    movlw 15
    cpfseq last_led 
    goto skip15_2
    goto case15_2
skip15_2:
    movlw 16
    cpfseq last_led 
    goto state3 ; THERE IS AN ERROR IF CONTROL REACHES HERE!!
    goto case16_2
    
; CASE STATEMENTS FOR STATE1
case0_1:
    movlw 1
    movwf last_led 
    bsf LATA, 0
    call delay2
    goto state1
case1_1:
    movlw 2
    movwf last_led
    bsf LATA, 1
    call delay2
    goto state1
case2_1:
    movlw 3
    movwf last_led 
    bsf LATA, 2
    call delay2
    goto state1
case3_1:
    movlw 4
    movwf last_led
    bsf LATA, 3
    call delay2
    goto state1
case4_1:
    movlw 5
    movwf last_led 
    bsf LATB, 0
    call delay2
    goto state1
case5_1:
    movlw 6
    movwf last_led
    bsf LATB, 1
    call delay2
    goto state1
case6_1:
    movlw 7
    movwf last_led 
    bsf LATB, 2
    call delay2
    goto state1
case7_1:
    movlw 8
    movwf last_led
    bsf LATB, 3
    call delay2
    goto state1
case8_1:
    movlw 9
    movwf last_led 
    bsf LATC, 0
    call delay2
    goto state1
case9_1:
    movlw 10
    movwf last_led
    bsf LATC, 1
    call delay2
    goto state1    
case10_1:
    movlw 11
    movwf last_led 
    bsf LATC, 2
    call delay2
    goto state1
case11_1:
    movlw 12
    movwf last_led
    bsf LATC, 3
    call delay2
    goto state1
case12_1:
    movlw 13
    movwf last_led 
    bsf LATD, 0
    call delay2
    goto state1
case13_1:
    movlw 14
    movwf last_led
    bsf LATD, 1
    call delay2
    goto state1
case14_1:
    movlw 15
    movwf last_led 
    bsf LATD, 2
    call delay2
    goto state1
case15_1:
    movlw 16
    movwf last_led
    bsf LATD, 3
    call delay2
    goto state1
case16_1:
    goto state2 ; if all leds are turned on, directly go to state2
    
; CASE STATEMENTS FOR STATE3
case0_2:
    goto state2 ; if all leds are turned off, directly go to state2
case1_2:
    movlw 0
    movwf last_led
    bcf LATA, 0
    call delay2
    goto state3
case2_2:
    movlw 1
    movwf last_led 
    bcf LATA, 1
    call delay2
    goto state3
case3_2:
    movlw 2
    movwf last_led
    bcf LATA, 2
    call delay2
    goto state3
case4_2:
    movlw 3
    movwf last_led 
    bcf LATA, 3
    call delay2
    goto state3
case5_2:
    movlw 4
    movwf last_led
    bcf LATB, 0
    call delay2
    goto state3
case6_2:
    movlw 5
    movwf last_led 
    bcf LATB, 1
    call delay2
    goto state3
case7_2:
    movlw 6
    movwf last_led
    bcf LATB, 2
    call delay2
    goto state3
case8_2:
    movlw 7
    movwf last_led 
    bcf LATB, 3
    call delay2
    goto state3
case9_2:
    movlw 8
    movwf last_led
    bcf LATC, 0
    call delay2
    goto state3    
case10_2:
    movlw 9
    movwf last_led 
    bcf LATC, 1
    call delay2
    goto state3
case11_2:
    movlw 10
    movwf last_led
    bcf LATC, 2
    call delay2
    goto state3
case12_2:
    movlw 11
    movwf last_led 
    bcf LATC, 3
    call delay2
    goto state3
case13_2:
    movlw 12
    movwf last_led
    bcf LATD, 0
    call delay2
    goto state3
case14_2:
    movlw 13
    movwf last_led 
    bcf LATD, 1
    call delay2
    goto state3
case15_2:
    movlw 14
    movwf last_led
    bcf LATD, 2
    call delay2
    goto state3
case16_2:
    movlw 15
    movwf last_led
    bcf LATD, 3
    call delay2
    goto state3 
        
finishPressRE3:
    btfsc PORTE,3 ; Check if RE3 is released
    goto finishPressRE3
    goto state1 ; if yes, then go to state1
    
finishPressRE4:
    btfsc PORTE,4 ; Check if RE4 is released
    goto finishPressRE4
    goto state3 ; if yes, then go to state3

main:
    call init
    goto state1
    end