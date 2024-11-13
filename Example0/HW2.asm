;****************************************************************************************
; Part4.asm
; Ryan Simoneau
; Last Modified: Jan, 27 2021
;
; Description: This program serves as a platform to complete HW2. Problems are numbered at the top of their respective sections in code
;
; Command file used = KG_2837x_RAM_lnk_cpu1.cmd
; Important Code locations:
;		.data   RAMLS0	 (internal DSP memory) starting address = 08000 Hex, 2K Words
;		.text	RAMLS2   (internal DSP memory) starting address = 09000 Hex, 2K Words
;		.ebss	RAMLS4	 (internal DSP memory) starting address = 0A000 Hex, 2K Words
;
;****************************************************************************************
		.global		_c_int00

;******************* DATA ALLOCATION SECTION - Variables/Data ***************************
	.sect	.data

vector		.word	0xF321, 0xF111, 0x2222, 0x1234, 0x0 ; a vector of arbitrary values

Val1		.word	0xFFFF
Val2		.word	0x7FFF

sum			.usect ".ebss", 1 ; allocate space for the sums to go in memory


;****************************************************************************************

;********************************** F335 Program ****************************************

		.text

_c_int00:
		; ** Queston 5 ** ;
		; clear registers that will be used as address pointers
		MOVL		XAR0,#0
		MOVL		XAR1,#0

		MOV		AR0,#0x8000
		MOV		AR1,#0xA000

LOOP	MOV		AH,*AR0

		B		SUM,GT ; N = 0, Z = 0
		B		NEGG,LT
		B		END,EQ ; Z = 1

NEGG	INC		AR0 ; go to the next value in memory
		B		LOOP,UNC

SUM		INC		*AR1 ; increment the sum in memory
		INC		AR0 ; go to the next value in memory table
		B		LOOP,UNC ; go back to the top of the loop

END		; reset used registers and continue with the rest of the code for HW2
		MOV		AR0,#0
		MOV		AR1,#0
		MOV		AH,#0
		MOV		AL,#0

		; ** Question 8: check if less than (signed) **
		MOV		AR0,#Val1 ; Val1 = 0xFFFF
		MOV		AR1,#Val2 ; Val2 = 0x7FFF
		MOV		AH,*AR0
		MOV		AL,*AR1

		CMP		AH,AL
		B		Label1,LO
		B		Label2,UNC

Label1	NOP

Label2	NOP





		; $8000 + $8000
		MOV		AH,#0x8000
		MOV		AL,#0x8000
		ADD		AH,AL

		; $FFFF + $4000
		MOV		AH,#0xFFFF
		MOV		AL,#0x4000
		ADD		AH,AL

		; $8000 + $F000
		MOV		AH,#0x8000
		MOV		AL,#0xF000
		ADD		AH,AL

		; $7F00 + $3EEE
		MOV		AH,#0x7F00
		MOV		AL,#0x3EEE
		ADD		AH,AL

		; $8000 - $8000
		MOV		AH,#0x8000
		MOV		AL,#0x8000
		SUB		AH,AL

		; $FFFF - $4000
		MOV		AH,#0xFFFF
		MOV		AL,#0x4000
		SUB		AH,AL

		; $8000 - $F000
		MOV		AH,#0x8000
		MOV		AL,#0xF000
		SUB		AH,AL

		; $7F00 - $3EEE
		MOV		AH,#0x7F00
		MOV		AL,#0x3EEE
		SUB		AH,AL

END1	B		END1,UNC






