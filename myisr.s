InterruptTick:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	push es
	push ds
	sti
	call InterruptTicker
	cli
	mov al, 0x20
	out 0x20, al
	pop ds
	pop es
	pop bp
	pop di
	pop si
	pop dx
	pop cx 
	pop bx
	pop ax
	iret
InterruptKeyboard:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	push es
	push ds
	sti
	call InterruptKeyboarder
	cli
	mov al, 0x20
	out 0x20, al
	pop ds
	pop es
	pop bp
	pop di
	pop si
	pop dx
	pop cx 
	pop bx
	pop ax
	iret
InterruptReset:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	push es
	push ds
	sti
	call InterruptReseter
	cli
	mov al, 0x20
	out 0x20, al
	pop ds
	pop es
	pop bp
	pop di
	pop si
	pop dx
	pop cx 
	pop bx
	pop ax
	iret
