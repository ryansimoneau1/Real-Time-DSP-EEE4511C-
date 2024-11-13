;****************************************************************************************
; Part4.asm
; Ryan Simoneau
; Last Modified: Jan, 27 2021
;
; Description: This program sets values to the LEDs based on the inputs from the DIP switches and push buttons
;
; Command file used = KG_2837x_RAM_lnk_cpu1.cmd
; Important Code locations:
;		.data   RAMLS0	 (internal DSP memory) starting address = 08000 Hex, 2K Words
;		.text	RAMLS2   (internal DSP memory) starting address = 09000 Hex, 2K Words
;		.ebss	RAMLS4	 (internal DSP memory) starting address = 0A000 Hex, 2K Words
;
;****************************************************************************************
		.global		_c_int00


;***************************** Program Constants ****************************************

WDCP		.set	0x7029 ; address of each register in memory

GPAMUX1L	.set	0x7C06
GPAMUX1H	.set	0x7C06
GPAGMUX1L	.set	0x7C20
GPAGMUX1H	.set	0x7C21

GPAMUX2L	.set	0x7C08
GPAMUX2H	.set	0x7C09
GPAGMUX2L	.set	0x7C22
GPAGMUX2H	.set	0x7C23

GPADATL		.set	0x7F00
GPADIRL		.set	0x7C0A
GPASETL		.set	0x7F02
GPACLEARL	.set	0x7F04
GPAPUDL		.set	0x7C0C
GPAPUDH		.set	0x7C0D

;****************************************************************************************

;********************************** F335 Program ****************************************

		.text

_c_int00:

		EALLOW ; allow GPIO CTRL registers to be modified

		; clear some aux registers
		MOVL	XAR0,#0
		MOVL	XAR1,#0
		MOVL	XAR2,#0

		; dissable the watchdog timer
		MOV		AR0,#WDCP
		MOV		AL,#0x68
		MOV		*AR0,AL

		; enable pull up resistors for GPIO[16:14] and [3:0] (where the switches and buttons are)
		MOV		AH,#0xFFFE ; GPIO16
		MOV		AL,#0x3FF0 ; GPIO[15:14] GPIO[3:0]
		MOV		AR0,#GPAPUDH
		MOV		AR1,#GPAPUDL
		MOV		*AR0,AH
		MOV		*AR1,AL

		; set all the GPIO pins to GPIO
		MOV		AL,#0x0000

		MOV		AR0,#GPAMUX1L ; change GPAMUX to zero before modifying GPAGMUX
		MOV		*AR0,AL
		MOV		AR0,#GPAGMUX1L
		MOV		*AR0,AL

		MOV		AR0,#GPAMUX1H
		MOV		*AR0,AL
		MOV		AR0,#GPAGMUX1H
		MOV		*AR0,AL

		MOV		AR0,#GPAMUX2L
		MOV		*AR0,AL
		MOV		AR0,#GPAGMUX2L
		MOV		*AR0,AL

		MOV		AR0,#GPAMUX2H
		MOV		*AR0,AL
		MOV		AR0,#GPAGMUX2H
		MOV		*AR0,AL

		; LED[7:2] => GPIO[11:6]. LED [1:0] => GPIO[5:4]
		MOV		AR0,#GPASETL
		MOV		AL,#0x00 ; set GPIO[15:0] to start since LEDs active low
		LSL		AL,#4
		MOV		*AR0,AL
		MOV		AL,#0xFF
		LSL		AL,#4
		MOV		*AR0,AL

		; set the direction of GPIO[11:4] as outputs
		MOV		AR0,#GPADIRL
		MOV		AL,#0xFF ; set LED pins as outputs from the uProcessor
		LSL		AL,#4 ; shift left so that the LEDs are set
		MOV		*AR0,AL

		; set the direction of push buttons and switches as inputs to the uProcessor


LOOP	; continuously update the LEDs

		; Push Buttons
		MOV		AR1,#GPADATL+1 ; GPIO16
		MOV		AH,*AR1
		LSL		AH,#2 ; move it up 2 spaces
		AND		AH,#4 ; bitwise AND with 0b00000100 to mask
		MOV		AR1,#GPADATL ; GPIO[15:14]
		MOV		AL,*AR1
		LSR		AL,#14 ; move to least 2 significant bits
		AND		AL,#3 ; bitwise AND with 0b00000011 to mask
		OR		AH,AL ; combine so AH contains GPIO[16:14] in lowest 3 bits
		MOV		AR0,AH ; move value out of accumulator register

		; Switches
		MOV		AR1,#GPADATL ; GPIO[3:0]
		MOV		AH,*AR1
		LSL		AH,#3 ; make room for the push buttons
		AND		AH,#0x78 ; bitwise and with 0b01111000 to mask

		; Combine
		MOV		AL,AR0 ; move the push button values into AL
		OR		AH,AL ; combine the two so that the lower 7 bits are the switches and push buttons respectively
		OR		AH,#0x80 ; make sure theres a 1 in the MSb at all times so that the MSb of the LEDs is off

		; send to LEDs
		MOV		AR1,#GPADATL
		LSL		AH,#4 ; LEDs at GPIO[11:4]
		MOV		*AR1,AH

		; Branch back to the top of the loop and begin again
		B		LOOP,UNC





