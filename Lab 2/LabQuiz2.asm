;****************************************************************************************
; LabQuiz2.asm.asm
; Ryan Simoneau
; Last Modified: Jan, 26 2021
;
; Description:
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

counter			.set	5 ; arbitrary values to avoid hard coding (# of #s in vector)


;****************************************************************************************

;******************* DATA ALLOCATION SECTION - Variables/Data ***************************
	.sect	.data

vector		.word	15, 35, 100, 0, 18, 37

VectorSort 	.usect ".ebss", 6



;****************************************************************************************


;********************************** F335 Program ****************************************

		.text

_c_int00:

		; operation to set the carry flag for unsigned algorithm
		MOV		AH,#10
		MOV		AL,#9
		CMP		AH,AL

		MOV		AR0,#counter
		MOV		AR1,#vector
		MOV		AR2,AR1
		INC		AR2
		MOV		AR3,#VectorSort

CHECK		MOV		AH,*AR1
			MOV		AL,*AR2
FALSE1C		CMP		AH,AL
			B		TRUE1,LO
			B		FALSE1,UNC



TRUE1

		DEC		AR0
		B		TRUE2,EQ
		INC		AR2 ; get a new value to compare against AR1 value if were not at the end of the table
		B		CHECK,UNC

FALSE1
		; if false, val2 must be greater than val1, so swap them
		MOV		*AR1,AL
		MOV		*AR2,AH
		INC		AR2
		B		CHECK,UNC

TRUE2
		MOV		*AR3,AH
		INC		AR3
		INC		AR1
		MOV		AR0,#counter
		DEC		AR0
		MOV		AL,AR1
		MOV		AR2,AL


END		B		END,UNC
