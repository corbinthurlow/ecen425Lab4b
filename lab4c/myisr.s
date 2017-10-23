	%macro save_context_ISR 0
		push ax
		push bx
		push cx
		push dx
		push si
		push di
		push bp
		push es
		push ds
	%endmacro

	%macro restore_context_ISR 0
		pop ds
		pop es
		pop bp
		pop di
		pop si
		pop dx
		pop cx
		pop bx
		pop ax
	%endmacro

	%macro EOI_command 0
		mov al, 0x20
		out 0x20, al
	%endmacro

InterruptTick:
	save_context_ISR
	call YKEnterISR			;call enter isr at beginning of isr
	sti
	call YKTickHandler		; call tick handler every time in the tick isr
	call InterruptTicker
	cli
	EOI_command
	call YKExitISR			; call YKExitISR before iret command 
	restore_context_ISR
	iret

InterruptKeyboard:
	save_context_ISR
	call YKEnterISR			;call enter isr at beginning of isr - shawn
	sti
	call InterruptKeyboarder
	cli
	EOI_command
	call YKExitISR			; call YKExitISR before iret command - shawn
	restore_context_ISR
	iret

InterruptReset: ; since this will terminate program, we don't need to save context and all that. just call InterruptReseter - shawn
	save_context_ISR
	sti
	call InterruptReseter
	cli
	EOI_command
	restore_context_ISR
	iret
