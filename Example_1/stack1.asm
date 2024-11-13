;****************************************************************************************
; file = stack1.asm
; Quick examples of how to use the stack and subroutine calls.
; Dr. Karl Gugel, Jan 2019
;
; To be assembled using Code Composer Studio which requires a linker command 
; file to tell CCR where to place code & data into DSP SRAM. 
; The command file used = KG_2837x_RAM_lnk_cpu1.cmd
; Important Code locations:
;		.data   RAMLS0	 (internal DSP memory) starting address = 08000 Hex, 2K Words
;		.text	RAMLS2   (internal DSP memory) starting address = 09000 Hex, 2K Words
;		.ebss	RAMLS4	 (internal DSP memory) starting address = 0A000 Hex, 2K Words
;		.stack	RAMM1	 (internal DSP memory) starting address = 0400  Hex, 0x400 Words
;
		.global		_c_int00	;This assembler directive allows _c_int00 to be a
						;global variable. This tells the linker where your
						;program (.text code) begins and where to boot from. 
;								
;****************************************************************************************
;***************************** Program Constants ****************************************
vector_len	.set		0x7		;length of test vector
stack_addr	.set		0x400		;the first address of stack memory
WDCR		.set		0x7029		;Watchdog Register is at address 0x7029
;****************************************************************************************
;******************* DATA ALLOCATION SECTION - Variables/Data ***************************
		.sect		.data		;data section, see the command linker file, starting at 0x8000.
vector_addr 	.word 	  0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777 ;test vector
						;.global directive lets you to see addresses in map file.
		.global vector_addr,vector_len
;****************************************************************************************

;************************ Stack & Subroutine Program Example ****************************
		.text		;Program section, see command linker file, program code starts at 0x9000
_c_int00:			;This label tells the linker where the entry (starting) point for
				;the first instruction in your program.	
									
		EALLOW		;Enable Protected resiter write. Needed to change control registers.

				;HIGHLY RECOMMENDED WHEN DEBUGGING AND USING AR7:0 AS PTRs
		MOVL	XAR0,#0x0	;clear AR0H, since AR0 will be used as a ptr
		MOVL	XAR1,#0x0	;clear AR1H, since AR1 will be used as a ptr
		MOVL	XAR2,#0x0	;clear AR2H, since AR2 will be used as a ptr

		MOV		AL,#0x0068	;Disable Watchdog Timer in case we want to run this code on our board
		MOV		AR0,#WDCR
		MOV		*AR0,AL

		MOV		SP,#stack_addr	;Initialize the SP to point to Stack Memory (internal DSP RAM)
	
		MOV		AL,#0xffff	;set stack mem to FFFF so that we can see changes in mem 0x400-0x404
		MOV		AR1,#0x5	;number of mem locations to set to FFFF
		MOV		AR0,#0x400	;starting addr of stack memory
LOOP1:		MOV		*AR0,AL
		INC		AR0
		DEC		AR1
		B		LOOP1,NEQ	

		MOV	    	AR0,#vector_len		;save vector len on the stack
		PUSH		AR0
		
		MOV		AR0,#vector_addr	;save vector starting address on the stack
		PUSH		AR0

		LC		Reverse		;call subroutine Reverse, this routine automatically pushes
                            			;the two word return addr onto the stack
								
END1:		B		END1,UNC 	;loop to prevent un-initialized code being executed

;**************************** Subroutine Section ***************************************
Reverse:
				;function: reverses the elements in a vector
				;inputs: vector length & vector start address
				;input passing method: inputs are pulled (popped) from the stack
				;output: vector (memory) is modified
				;special Note: all subs should be placed before or after the main program.
		NOP		;these NOPs were put in to purge the pipeline so that SP would stablize
		NOP		;and stack memory would be constant by the time the last NOP is executed.
		NOP
		NOP		;I'm not sure if these are now needed with the new CCS v8 debugger....
		NOP
		
		POP		XAR7		;get the return address off the stack
		POP		AR1		;get the vector start address off the stack
		POP		AL		;get the vector length from the stack

		MOV		AR2,AR1		;set up a ptr to the last element in the array
		ADD		AR2,AL		;last addr = first addr + length - 1
		DEC		AR2		;now AR1 = vector start addr, AR2 = vector end addr

		LSR		AL,1		;loop counter = length/2
		MOV		AR0,AL		;free up AL and use AR0 as our counter
								
LOOP2:		MOV		AL,*AR1		;get first value
		MOV		AH,*AR2		;get last value
		MOV		*AR2,AL		;move first value to last position
		MOV		*AR1,AH		;move last value to first position
		INC		AR1		;inc ptr pointing to beginning of vector
		DEC		AR2		;dec prt pointing to end of vector
		DEC		AR0		;dec counter and test if zero, if != 0 then repeat
		B		LOOP2,NEQ
								
		PUSH		XAR7		;push return addr back onto stack
		LRET				;end of subroutine, this instruction pops off the two word return addr
;******* Notes for Stepping through this Code ******	;  1. 	Watch the SP register before/after a Push, Pop, Long Call (LC) and Long Return (LRET);  2. 	Observe how the stack memory changes (0x400 thru 0x403 internal SRAM) after a Push and/or LC.;  3. 	Try to predict the SP changes and stack memory changes once you understand the;	SP and stack operation.
	
		
	
