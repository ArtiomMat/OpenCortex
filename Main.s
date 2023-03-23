	.file	"Main.cpp"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"%d\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB34:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movl	$69, %esi
	movl	$.LC0, %edi
	xorl	%eax, %eax
	movl	$1, %ebx
	call	printf
	movl	$.LC0, %edi
	xorl	%eax, %eax
#APP
# 11 "Main.cpp" 1
	addb %bl, %sil
# 0 "" 2
#NO_APP
	movsbl	%sil, %esi
	call	printf
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE34:
	.size	main, .-main
	.ident	"GCC: (GNU) 12.2.1 20221121 (Red Hat 12.2.1-4)"
	.section	.note.GNU-stack,"",@progbits
