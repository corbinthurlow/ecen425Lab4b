%macro YKDispatcher1_run 0
	mov bx, [YKRdyList]		;set up the stack pointer
	mov sp, [bx]		;by restoring it from TCB
	pop word [ssTemp]
	pop ds
	pop si
	pop di
	pop es
	pop ax
	pop bx
	pop cx
	pop dx
	pop bp
	iret
%endmacro

%macro save_context 0
	push bp
	push dx
	push cx
	push bx
	push ax
	push es
	push di
	push si
	push ds
	mov [ssTemp], ss
	push word [ssTemp]
						;move current stack pointer on cTask struct
	mov bx, [oldTask]
	mov [bx], sp
%endmacro

YKDispatcherASM:
	cmp word[saveContext], 0
	je YKDispatcher1
	pushf
	push cs
	save_context
YKDispatcher1:
	YKDispatcher1_run

YKEnterMutex:
	cli
	ret

YKExitMutex:
	sti
	ret


YKIdleTask:
	push bp	
	mov bp, sp
	jmp while1

while1:
	cli
	inc word [YKIdleCount]
	sti
	jmp while1




