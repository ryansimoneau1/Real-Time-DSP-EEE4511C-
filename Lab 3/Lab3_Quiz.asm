;****************************************************************************************
; Part2.asm
; Ryan Simoneau
; Last Modified: Feb, 02 2021
;
; Description: This program converts a table of fahrenheit temperature values of arbitrary length in data memory to celcius
;			   and stores them into a seperate table also located in data memory. This program assumes signed numbers.
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

N			.set	10 ; Counter equal to the length of the tables
WDCR		.set	0x7029 ; the location of the watchdog timmer
stack_addr	.set	0x400 ; location of the stack

;****************************************************************************************


;******************* DATA ALLOCATION SECTION - Variables/Data ***************************

	.sect	.data

count	.word	N ; set the counter as the length of DegF

LINE	.float	0,2,3,4,5,6,7,8,9,10
CUBE	.usect ".ebss",2*N ; will need 2 words per floating point number. (2*N)


;****************************************************************************************


;********************************** F335 Program ****************************************
		.text

_c_int00:
		; clear auxiliary registers for use later
		MOVL	XAR0,#0
		MOVL	XAR1,#0
		MOVL	XAR2,#0
		MOVL	XAR3,#0
		MOVL	XAR4,#0

		MOV		SP,#stack_addr	;Initialize the SP

		; load auxiliary registers with addresses of data memory
		MOVL	XAR0,#count
		MOVL	XAR3,#LINE
		MOVL	XAR4,#CUBE

FVL:	LC		CUBES

		DEC		*XAR0
		B		FVL,NEQ

END:	B		END,UNC ; if the counter is zero, the algorithm is complete

;********************************** Subroutines *****************************************

CUBES:

		; floating point math operations
		MOV32	R0H,*XAR3 ; put LINE value into 2 seperate registers for future computation
		NOP		; purge the pipeline
		NOP
		NOP
		MOV32	R1H,*XAR3
		NOP
		NOP
		NOP

		; multiply the number by itself
		MPYF32	R1H,R1H,R0H
		NOP
		NOP
		NOP

		; multiply the number by itself again to get the cube
		MPYF32	R1H,R1H,R0H
		NOP


		; move value into data memory at the table where XAR4 points
		MOV32	*XAR4,R1H
		NOP
		NOP
		NOP

		; decrement the counter and increment the address pointers
		MOV		AH,AR3
		ADD		AH,#2
		MOV		AR3,AH


		MOV		AH,AR4
		ADD		AH,#2
		MOV		AR4,AH

		LRET

;****************************************************************************************
