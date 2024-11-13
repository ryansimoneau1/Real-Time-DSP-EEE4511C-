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

N			.set	26 ; Counter equal to the length of the tables
WDCR		.set	0x7029 ; the location of the watchdog timmer
stack_addr	.set	0x400 ; location of the stack

;****************************************************************************************


;******************* DATA ALLOCATION SECTION - Variables/Data ***************************

	.sect	.data

count	.word	N ; set the counter as the length of DegF
Fnine	.float	0.555556 ; 5/9 as a decimal
Ttwo	.float	32

DegF	.float	0, 8.8, 17.6, 26.4, 35.2, 44, 52.8, 61.6, 70.4, 79.2, 88, 96.8, 105.6, 114.4, 123.2, 132, 140.8, 149.6, 158.4, 167.2, 176, 184.8, 193.6, 202.4, 211.2, 220
DegC	.usect ".ebss",2*N ; will need 2 words per floating point number. (2*N)


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
		MOVL	XAR1,#Fnine
		MOVL	XAR2,#Ttwo
		MOVL	XAR3,#DegF
		MOVL	XAR4,#DegC

		PUSH	AR0
		PUSH	AR1
		PUSH	AR2
		PUSH	AR3
		PUSH	AR4

FVL:	LC		FtoC

		DEC		*XAR0
		B		FVL,NEQ

END:	B		END,UNC ; if the counter is zero, the algorithm is complete

;********************************** Subroutines *****************************************

FtoC:
		POP		AR7
		POP		AR6
		POP		AR4
		POP		AR3
		POP		AR2
		POP		AR1
		POP		AR0

		; floating point math operations
		MOV32	R0H,*XAR1 ; R0 is 5/9
		NOP		; purge the pipeline
		NOP
		NOP

		MOV32	R1H,*XAR3 ; R1 gets each value of DegF
		NOP
		NOP
		NOP

		MOV32	R2H,*XAR2 ; R2 gets 32
		NOP
		NOP
		NOP

		; subtract 32 from the number
		SUBF32	R1H,R1H,R2H
		NOP
		NOP
		NOP

		; multiply the number by 5/9
		MPYF32	R1H,R1H,R0H
		NOP
		NOP
		NOP

		; move value into data memory at the table where xar3 points
		MOV32	*XAR4,R1H
		NOP
		NOP
		NOP

		; decrement the counter amd increment the address pointers
		MOV		AH,AR3
		ADD		AH,#2
		MOV		AR3,AH


		MOV		AH,AR4
		ADD		AH,#2
		MOV		AR4,AH

		PUSH	AR0
		PUSH	AR1
		PUSH	AR2
		PUSH	AR3
		PUSH	AR4
		PUSH	AR6
		PUSH	AR7
		LRET

;****************************************************************************************



