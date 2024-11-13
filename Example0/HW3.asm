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

N		.set	3 ; Counter equal to the length of the tables
WDCR	.set	0x7029 ; the location of the watchdog timmer

;****************************************************************************************


;******************* DATA ALLOCATION SECTION - Variables/Data ***************************

	.sect	.data

count	.word	N ; set the counter as the length of DegF
TST		.float	273.15

DegC	.float	20, 30, 15
DegKOnes	.usect ".ebss",2*N ; will need 2 words per floating point number. (2*N)
DegKTenths	,usect ".ebss",2*N


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

		; load auxiliary registers with addresses of data memory
		MOVL	XAR0,#count
		MOVL	XAR1,#DegC
		MOVL	XAR2,#DegKOnes
		MOVL	XAR3,#DegKTenths
		MOVL	XAR4,#TST

FVL:	LC		CtoK

		DEC		*XAR0
		B		FVL,NEQ

END:	B		END,UNC ; if the counter is zero, the algorithm is complete

;********************************** Subroutines *****************************************

CtoK:

		; floating point math operations
		MOV32	R0H,*XAR1 ; R0 is DegC

		MOV32	R2H,*XAR4 ; R2 gets 273.15

		; add 273.15 to each value
		ADDF32	R1H,R1H,R2H
		NOP

		; copy whats in R1H to another register to preserve the decimal
		MOV32	R3H,R1H

		; convert number to integer and then back to a float to remove the decimal
		F32TOUI16	R1H,R1H
		NOP
		NOP
		I16TOF32	R1H,R1H

		; subtract the integer value and multiply by ten and convert to an integer to get the tenths place
		SUBF32		R3H,R3H,R1H
		NOP
		MPYF32		R3H,#10.0,R3H
		NOP
		; store the ones place into memory
		MOV32	*XAR2,R1H

		; move tenths place into memory
		MOV32	*XAR3,R3H


		; increment the address pointers
		MOV		AH,AR1
		ADD		AH,#2
		MOV		AR1,AH

		MOV		AH,AR2
		ADD		AH,#2
		MOV		AR2,AH


		MOV		AH,AR3
		ADD		AH,#2
		MOV		AR3,AH

		LRET
