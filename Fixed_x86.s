#define Global

.globl _OAI_F8_Add
.globl _OAI_F8_Sub
.globl _OAI_F8_Mul
.globl _OAI_F8_Div

.globl _OAI_F8_N

_OAI_F8_N: .byte 4

_OAI_F8_Add:
	addb %sil, (%rdi)
	
_OAI_F8_Sub:
	subb %sil, (%rdi)
	ret

_OAI_F8_Mul:
	movzbl (%rdi), %eax
	imull %esi, %eax
	movb %al, (%rdi)

_OAI_F8_Div:

// We assume that bl contains the value of 
_OverflowCheck:
	
	ret
