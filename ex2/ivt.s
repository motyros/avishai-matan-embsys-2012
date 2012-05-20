.seg "INTERRUPT_TABLE", text
.global InterruptTable
        
InterruptTable:
	FLAG 0				; IRQ 0
	b       _start
	jal 	MEM_ERROR_ISR		; IRQ 1
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 2
	jal 	timer0ISR	        ; IRQ 3
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 4	
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 5	
	jal 	BAD_INSTRUCTION_ISR 	; IRQ 6	
	jal 	timer1ISR          	; IRQ 7
	jal     BAD_INSTRUCTION_ISR     ; IRQ 8
	jal     buttonPressedIsr     ; IRQ 9     

MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
	flag 1
	nop
	nop
	nop