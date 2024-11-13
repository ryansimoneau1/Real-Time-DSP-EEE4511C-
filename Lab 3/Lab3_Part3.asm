;****************************************************************************************
; Lab3_Part3.asm
; Ryan Simoneau
; Last Modified: Feb, 07 2021
;
; Description: This program writes my name to an LCD screen
;
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


; GPIO register locations in memory
GPDMUX1L	.set	0x7CC6
GPDMUX1H	.set	0x7CC7
GPDGMUX1L	.set	0x7CE0
GPDGMUX1H	.set	0x7CE1

GPDDATL		.set	0x7F18
GPDDIRL		.set	0x7CCA
GPDSETL		.set	0x7F1A
GPDCLEARL	.set	0x7F1C
GPDTOGGLEL	.set	0x7F1E
GPDPUDL		.set	0x7CCC
GPDPUDH		.set	0x7CCD


; Values needed to interface with the LCD
LCDADDR		.set	0x4E
FBITTLIN	.set	0x28
DISPCUR		.set	0x0F
CLEARDISP	.set	0x01

;****************************************************************************************


;******************* DATA ALLOCATION SECTION - Variables/Data ***************************
		.sect	.data

NameVector .word	"Ryan Simoneau"


;****************************************************************************************


;********************************** F335 Program ****************************************
		.text

_c_int00:

		EALLOW ; allow GPIO CTRL registers to be modified

		; clear auxiliary registers for use later
		MOVL	XAR0,#0
		MOVL	XAR1,#0
		MOVL	XAR2,#0
		MOVL	XAR3,#0
		MOVL	XAR4,#0

		; dissable the watchdog timer
		MOV		AR0,#WDCR
		MOV		AL,#0x68
		MOV		*AR0,AL

		MOV		SP,#stack_addr	;Initialize the SP

		LC		GPIOINIT

		LC		Initialize_LCD
		LC		Write_Char_String

END:	B		END,UNC ; if the counter is zero, the algorithm is complete

;********************************** Subroutines *****************************************

Initialize_LCD: ; this subroutine initializes the LCD by sending it a series of commands

		MOV		AR0,#0x33
		LC		Write_Command_Reg

		MOV		AR0,#0x32
		LC		Write_Command_Reg

		; set the LCD into 4-bit mode
		MOV		AR0,#FBITTLIN
		LC		Write_Command_Reg

		; display the cursor
		MOV		AR0,#DISPCUR
		LC		Write_Command_Reg

		; clear the display
		MOV		AR0,#CLEARDISP
		LC		Write_Command_Reg

		LRET

Write_Char_String: ; this subroutine writes a string of characters to the LCD
		; subroutine uses AR0 for Table values

		PUSH	AR1
		PUSH	AL

		MOV		AR1,#NameVector

NCHAR	MOV		AL,*AR1
		MOV		AR0,AL
		LC		Write_Data_Reg
		INC		AR1
		MOV		AL,*AR1
		CMP		AL,#0
		B		NCHAR,NEQ

		POP		AL
		POP		AR1
		LRET



Write_Data_Reg: ; this subroutine loads a character to the LCD
		; assume data is passed via AR0

		PUSH	AH
		PUSH	AL
		PUSH	AR1

		; load AR0 value into both acc's to seperate into nibbles
		MOV		AH,AR0
		MOV		AL,AR0
		LSL		AL,#4
		AND		AH,#0x00F0
		AND		AL,#0x00F0
		PUSH	AL ; save these values onto the stack for future use
		PUSH	AH


		; OR AL and AH with 0b1101 to create the enable signal
		OR		AH,#0x0D
		OR		AL,#0x0D

		; send this info via I2C
		MOV		AR0,AH
		LC		I2C_SND
		MOV		AR1,#0x0FFF
LOOPA	LC		WAIT
		DEC		AR1
		B		LOOPA,NEQ

		POP		AH
		OR		AH,#0x09
		MOV		AR0,AH
		LC		I2C_SND
		MOV		AR1,#0x0FFF
LOOPB	LC		WAIT
		DEC		AR1
		B		LOOPB,NEQ


		MOV		AR0,AL
		LC		I2C_SND
		MOV		AR1,#0x0FFF
LOOPC	LC		WAIT
		DEC		AR1
		B		LOOPC,NEQ


		POP		AL
		OR		AL,#0x09
		MOV		AR0,AL
		LC		I2C_SND
		MOV		AR1,#0x0FFF
LOOPD	LC		WAIT
		DEC		AR1
		B		LOOPD,NEQ


		POP		AR1
		POP		AL
		POP		AH
		LRET


Write_Command_Reg: ; this program write to the control registers of the LCD
		; assumes data is passed via AR0

		PUSH	AH
		PUSH	AL

		; load AR0 value into both acc's to seperate into nibbles
		MOV		AH,AR0
		MOV		AL,AR0
		LSL		AL,#4
		AND		AH,#0x00F0
		AND		AL,#0x00F0
		PUSH	AL ; save these values onto the stack for future use
		PUSH	AH


		; OR AL and AH with 0b100 to create the enable signal
		OR		AH,#0x0C
		OR		AL,#0x0C

		; send this info via I2C
		MOV		AR0,AH
		LC		I2C_SND
		LC		WAIT

		POP		AH
		OR		AL,#0x08
		MOV		AR0,AH
		LC		I2C_SND
		LC		WAIT ; wait for LCD timing diagram to end
		LC		WAIT

		MOV		AR0,AL
		LC		I2C_SND
		LC		WAIT

		POP		AL
		OR		AL,#0x08
		MOV		AR0,AL
		LC		I2C_SND
		LC		WAIT ; wait for LCD timing diagram to end
		LC		WAIT

		POP		AL
		POP		AH
		LRET

I2C_SND: ; This subroutine allows for quick sending of any information to the LCD
		; subroutine assumes AR0 contains valid transmissible data

		LC		I2C_INIT
		LC		I2C_INFO
		LC		I2C_END

		LRET


WAIT: ; causes a software delay of 1 microsecond

		PUSH	AL

		MOV		AL,#0x0001
LOOP	DEC		AL
		B		LOOP,NEQ

		POP		AL
		LRET

GPIOINIT: ; this subroutine initializes the GPIO port D pins and prepares them for communication with the LCD

		PUSH	AL
		PUSH	AH
		PUSH	AR0

		MOV		AL,#0x0000

		; make sure GPIOD is set for GPIO
		MOV		AR0,#GPDMUX1H ; change GPDMUX to zero before modifying GPDGMUX
		MOV		*AR0,AL
		MOV		AR0,#GPDGMUX1H
		MOV		*AR0,AL

		; enable pull up resistors for both pin 104 and 105
		MOV		AR0,#GPDPUDL
		MOV		AL,#0xFCFF
		MOV		*AR0,AL

		; set the outputs of both pins 104 and 105 as 0
		MOV		AR0,#GPDCLEARL
		MOV		AL,#0x0300
		MOV		*AR0,AL

		; set both pins to be inputs initially (so that they are high to start)
		MOV		AH,#0xFCFF
		MOV		AR0,#GPDDIRL
		MOV		AL,*AR0
		AND		AH,AL
		MOV		*AR0,AH



		; This portion was to test the WAIT subroutine.

		; set GPIO 104 as an output with an initial value of 0 at pin 104 (bit 8 in DIR)
		;MOV		AR0,#GPDCLEARL
		;MOV		AL,#0x0100 ; set the initial value of pin 104 as 0
		;MOV		*AR0,AL

		;MOV		AR0,#GPDDIRL
		;MOV		AH,*AR0
		;OR		AL,AH ; bit mask so that only 104 value changes
		;MOV		*AR0,AL ; sets pin 104 as an output



		POP		AR0
		POP		AH
		POP		AL
		LRET

I2C_INIT: ; initialize the I2C module to send info to the LCD (sets the address)
; 		AR1 = counter starting at 8
;		AR2 = pointer for the DIR register
;		AL  = used to do ALU opperations such as NOT, OR, AND, etc.
;		AH  = used to do ALU opperations such as NOT, OR, AND, etc.

		PUSH	AR1
		PUSH	AR2
		PUSH	AL
		PUSH	AH

		; load AR2 with DIR register address
		MOV		AR2,#GPDDIRL

		; send 104 low and then 105 low to initiate start condition
		MOV		AH,*AR2
		OR		AH,#0x0100
		MOV		*AR2,AH
		LC		WAIT ; ****************************************
		OR		AH,#0x0200 ; send clock low
		MOV		*AR2,AH

		; load AR1 with counter
		MOV		AR1,#8

		; load LCD addr into AR0
		MOV		AH,#LCDADDR
		NOT		AH ; flip the bits since input is 1 and output is 0

		; align AR0 value with pin 104
		LSL		AH,#1

		; save this value for later use
LOOP1	PUSH	AH

		; isolate MSb of address
		AND		AH,#0x0100 ; bit mask MSb of address

		; shift a value into pin 104
		MOV		AL,*AR2 ; move DIR register value into AL
		AND		AL,#0xFEFF ; ensure bit changes value by and with 0xFEFF and then OR with AL
		OR		AH,AL ; makes sure that any bit change will occure without affecting other bits in DIR register
		MOV		*AR2,AH

		; toggle the clock to latch in value
		AND		AH,#0xFDFF ; send clock high
		MOV		*AR2,AH
		LC		WAIT ; wait half a period
		LC		WAIT ; ********************************************
		OR		AH,#0x0200 ; send clock low
		MOV		*AR2,AH
		LC		WAIT ; wait half a period

		POP		AH ; get back the value of the address

		LSL		AH,#1 ; shift up to get the next bit of the address to align with pin 104
		DEC		AR1
		B		LOOP1,NEQ

		;send pin 104 high and then 105 high to check the acknowledge bit
		MOV		AL,*AR2
		AND		AL,#0xFEFF
		MOV		*AR2,AL

		MOV		AL,*AR2
		AND		AL,#0xFDFF
		MOV		*AR2,AL

		LC		WAIT
		LC		WAIT


		MOV		AL,*AR2
		OR		AL,#0x0200
		MOV		*AR2,AL

		POP		AH
		POP		AL
		POP		AR2
		POP		AR1

		LRET










I2C_INFO: ; subroutine used to send general info through the I2C device
;		subroutine assumes AR0 will be used to pass data
;		subroutine also assumes I2C_INIT was executed prior

		PUSH	AR1
		PUSH	AR2
		PUSH	AL
		PUSH	AH

		; load AR2 with DIR register address
		MOV		AR2,#GPDDIRL

		; load AR1 with counter
		MOV		AR1,#8

		; load value into AH
		MOV		AH,AR0
		NOT		AH ; flip the bits since input is 1 and output is 0

		; align AR0 value with pin 104
		LSL		AH,#1

		; save this value for later use
LOOP2	PUSH	AH

		; isolate MSb of address
		AND		AH,#0x0100 ; bit mask MSb of address

		; shift a value into pin 104
		MOV		AL,*AR2 ; move DIR register value into AL
		AND		AL,#0xFEFF ; ensure bit changes value by and with 0xFEFF and then OR with AL
		OR		AH,AL ; makes sure that any bit change will occure without affecting other bits in DIR register
		MOV		*AR2,AH

		; toggle the clock to latch in value
		AND		AH,#0xFDFF ; send clock high
		MOV		*AR2,AH
		LC		WAIT ; wait half a period
		LC		WAIT ; ********************************************
		OR		AH,#0x0200 ; send clock low
		MOV		*AR2,AH
		LC		WAIT ; wait half a period

		POP		AH ; get back the value of the address

		LSL		AH,#1 ; shift up to get the next bit of the address to align with pin 104
		DEC		AR1
		B		LOOP2,NEQ

		;send pin 104 high and then 105 high to check the acknowledge bit
		MOV		AL,*AR2
		AND		AL,#0xFEFF
		MOV		*AR2,AL

		MOV		AL,*AR2
		AND		AL,#0xFDFF
		MOV		*AR2,AL

		LC		WAIT
		LC		WAIT


		MOV		AL,*AR2
		OR		AL,#0x0200
		MOV		*AR2,AL

		POP		AH
		POP		AL
		POP		AR2
		POP		AR1

		LRET




I2C_END: ; end I2C communication with the condition SCL = 1 before SDA = 1

		PUSH	AH
		PUSH	AR2

		MOV		AR2,#GPDDIRL
		MOV		AH,*AR2

		; send both the clock and SDA low and then send the clock high before SDA
		OR		AH,#0x0200 ; send clock low
		MOV		*AR2,AH

		OR		AH,#0x0100 ; send SDA low
		MOV		*AR2,AH

		; send the clock high
		AND		AH,#0xFDFF ; send clock high
		MOV		*AR2,AH

		LC		WAIT

		; send SDA high
		AND		AH,#0xFEFF
		MOV		*AR2,AH

		POP		AR2
		POP		AH
		LC		WAIT

		LRET



;****************************************************************************************
