IFDEF RAX

getcontext PROTO STDCALL :QWORD
setcontext PROTO STDCALL :QWORD

; ToDo: replace magic numbers with CONTEXT structure field offset calculations
offset_rip equ 252
offset_rsp equ 156

.code

swapcontext PROC
	push [rsp + 8]
	call getcontext

	; correct rip
	lea rax, [rsp + 8]
	add rax, offset_rip
	lea rdx, done
	mov [rax], rdx

	; correct rsp
	lea rax, [rsp + 8]
	add rax, offset_rsp
	mov [rax], rsp

	push [rsp + 16]
	call setcontext
done:
	add rsp, 16
	ret
swapcontext ENDP

ENDIF

end