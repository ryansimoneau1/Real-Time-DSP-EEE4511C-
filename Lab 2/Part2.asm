;****************************************************************************************
; Part2.asm
; Ryan Simoneau
; Last Modified: Jan, 26 2021
;
; Description: This program determines the number of values in memory that are greater than A
;			   and the number of values that are less than or equal to A and then stores this
;			   info into variables X and Y respectively. Works for unsigned Numbers only.
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

vect_addr	.set	0x8000 ; start address of table at the beginning of data memory
N			.set	8 ; arbitrary values to avoid hard coding (# of #s in vector)
A			.set	10 ; the value being compared against


;****************************************************************************************


;******************* DATA ALLOCATION SECTION - Variables/Data ***************************
	.sect	.data

vector		.byte	15 ; a vector of arbitrary values
			.byte	11
			.byte	4
			.byte	16
			.byte	5
			.byte	18
			.byte	10
			.byte	10

AVal		.byte	A ; load the value of A into memory as an 8-bit number

sumX		.usect ".ebss", 1 ; allocate space for the sums to go in memory
sumY		.usect ".ebss", 1



;****************************************************************************************


;********************************** F335 Program ****************************************

		.text

_c_int00:

		; place data addresses into registers for computations and reference
		MOV		AR0,#N ; move the value of the counter into AR0
		MOV		AR1,#vect_addr ; place the starting address of the table into a register
		MOV		AR2,#sumX ; place the address of SumX into a register
		MOV		AR3,#sumY ; place the address of sumY into a register
		MOV		AR4,#AVal ; place address of A into auxillary register

NEWV	; return here once the previous value is compared to load a new value

		; operation to set the carry flag for unsigned algorithm
		MOV		AH,#10
		MOV		AL,#9
		CMP		AH,AL

		MOV		AH,*AR4 ; AH will contain the value A for the entirety of the algorithm
		MOV		AL,*AR1 ; move a value from the table into a register to be compared against

		; check if A is less than or equal to what's in AL (unsigned)
		CMP		AH,AL ; Do AH - AL to compare the 2 values
		B		SUMX,LOS ; branch to SUMX if A <= TableVal
		B		SUMY,UNC ; if A </= TableVal, A > TableVal

SUMX
		INC 	*AR2 ; increment the sum in memory
		B		CNTC,UNC ; branch to where the counter is to be decremented

SUMY
		INC		*AR3 ; increment the sum in memory
		B		CNTC,UNC ; branch to where the counter is to be decremented

CNTC	; increments the table pointer and decrements the counter
		INC		AR1 ; increment the pointer used to gather new data
		DEC		AR0 ; decrement the counter
		B		NEWV,NEQ ; check to see if the counter is 0. if not, load a new value. If it is, the algorithm is finished


END1	B		END1,UNC
