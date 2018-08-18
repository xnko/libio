IFDEF RAX

fix_and_swapcontext PROTO STDCALL :QWORD

.code

swapcontext PROC

	push r8
	push r9
	push rdi
	push rsi
	push rbp
	push rdx ; other
	push rcx ; current

	lea r8, gotback
	mov r9, rsp

	; without some stack padding someone overrides our values, no idea why?
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax
	push rax

	call fix_and_swapcontext

gotback:

	pop rcx
	pop rdx
	pop rbp
	pop rsi
	pop rdi
	pop r9
	pop r8
	ret

swapcontext ENDP

ENDIF

end