.seg "INTERRUPT_TABLE", text
.global InterruptTable
        
InterruptTable:
	FLAG 0				; IRQ 0
	b       _start
	jal 	MEM_ERROR_ISR		; IRQ 1
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 2
	jal 	timer0ISR	        ; IRQ 3
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 4	
	jal 	flashISR		; IRQ 5
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 6	
	jal 	timer1ISR          	; IRQ 7
	jal     BAD_INSTRUCTION_ISR 	; IRQ 8
	jal     buttonPressedISR     	; IRQ 9
	jal     BAD_INSTRUCTION_ISR     ; IRQ 10
	jal     BAD_INSTRUCTION_ISR     ; IRQ 11
	jal     BAD_INSTRUCTION_ISR     ; IRQ 12
	jal     BAD_INSTRUCTION_ISR     ; IRQ 13
	jal     networkISR		; IRQ 14
	jal     lcdDisplayISR  		; IRQ 15
	  

MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
	flag 1
	nop
	nop
	nop
