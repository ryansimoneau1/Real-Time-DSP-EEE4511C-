
;****************************************************************************************
; Part3.asm
; Ryan Simoneau
; Last Modified: Jan, 27 2021
;
; Description: This program sets various LED states to test the LEDs and GPIO
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

		MOV		AR0,#GPADATL
		MOV		AL,#0xFF
		LSL		AL,#4
		MOV		*AR0,AL

		MOV		AR1,#GPADATL+1 ; push button 2
		MOV		AR2,#GPADATL ; push button 1 or 0
		MOV		AH,*AR1

		; if any of the push buttons are pressed, the value in data will be zero for that pin

		LSL		AH,#15 ; get PB2 into sign bit position
		B		PB2,GEQ ; if the button is pressed, the sign bit will be zero

		MOV		AL,*AR2
		B		PB1,GEQ

		LSL		AL,#1 ; get to PB0
		B		PB0,GEQ
		B		LOOP,UNC


PB2
		MOV		AR0,#GPADATL
		MOV		AL,#0
		NOT		AL
		LSL		AL,#4 ; shift left so that the LEDs are set
		MOV		*AR0,AL
		B		LOOP,UNC

PB1
		MOV		AR0,#GPADATL
		MOV		AL,#0
		LSL		AL,#4 ; shift left so that the LEDs are set
		MOV		*AR0,AL
		B		LOOP,UNC

PB0

		MOV		AR0,#GPADATL
		MOV		AL,#100
		NOT		AL
		LSL		AL,#4 ; shift left so that the LEDs are set
		MOV		*AR0,AL
		B		LOOP,UNC

		; Branch back to the top of the loop and begin again
		B		LOOP,UNC
