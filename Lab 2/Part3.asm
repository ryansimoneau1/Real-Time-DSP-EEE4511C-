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
GPAGMUX1L	.set	0x7C20
GPADIRL		.set	0x7C0A
GPASETL		.set	0x7F02
GPACLEARL	.set	0x7F04

;****************************************************************************************

;********************************** F335 Program ****************************************

		.text

_c_int00:

		EALLOW ; allow GPIO CTRL registers to be modified

		; clear aux registers
		MOVL	XAR0,#0
		MOVL	XAR1,#0
		MOVL	XAR2,#0

		; dissable the watchdog timer
		MOV		AR0,#WDCP
		MOV		AL,#0x68
		MOV		*AR0,AL

		; set all the GPIO pins to GPIO
		MOV		AR0,#GPAMUX1L ; change GPAMUX to zero before modifying GPAGMUX
		MOV		AL,#0x0000
		MOV		*AR0,AL

		MOV		AR0,#GPAGMUX1L
		MOV		*AR0,AL

		; LED[7:2] => GPIO[11:6]. LED [1:0] => GPIO[5:4]
		MOV		AR0,#GPASETL
		MOV		AL,#0xFF ; set GPIO[15:0] to start since LEDs active low
		LSL		AL,#4
		MOV		*AR0,AL

		MOV		AL,#0

		; set the direction of GPIO[11:4] as outputs
		MOV		AR0,#GPADIRL
		MOV		AL,#0xFF ; set LED pins as outputs from the uProcessor
		LSL		AL,#4 ; shift left so that the LEDs are set
		MOV		*AR0,AL

		; set some combination of bits (0xAC)
		MOV		AR0,#GPACLEARL
		MOV		AL,#0xAC
		LSL		AL,#4 ; shift left so that the LEDs are set
		MOV		*AR0,AL

END		B		END,UNC


