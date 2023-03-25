#include <stdio.h>
#include <stdlib.h>

#define CheckOverflow \
"jno ._End;"\
\
"js ._HaveSign;"\
\
"movb $-128, %0;" /* If we don't have a sign we underflowed*/\
"jmp ._End;"\
\
"._HaveSign: movb $127, %0;" /* If we have sign we overflowed */\
\
"._End:"

char _OAI_N = 4;

inline void _OAI_F8_Add(char& A, char B) {
	asm (
		"addb %1, %0;"
		CheckOverflow
		: "+g" (A) 
		: "g" (B)
	);
}

inline void _OAI_F8_Sub(char& A, char B) {
	asm (
		"subb %1, %0;"
		CheckOverflow
		: "+g" (A) 
		: "g" (B)
	);
}

inline void _OAI_F8_Mul(char& A, char B) {
	asm (
		"mov %0, %%al;"
		"mov %1, %%bl;"
		"imul %%bl;" // Stored in AX
		"sar %2, %%ax;" // TODO: I am 69% sure we can check for overflow before we even shift and increase performance
		
		"cmp $127, %%ax;"
		"jle ._OtherCheck;"
		"mov $127, %%al;"
		"jmp ._End;"

		"._OtherCheck:"
		"cmp $-128, %%ax;"
		"jge ._End;"
		"mov $-128, %%al;"

		"._End:"
		"mov %%al, %0"
		: "+r" (A) 
		: "r" (B), "g" (_OAI_N)
	);
}

inline void _OAI_F8_Div(char& A, char B) {
	short D = A;
	D <<= _OAI_N;
	D /= B;
	
	if (D > 127)
		D = 127;
	else if (D < -128)
		D = -128;
	
	A = D;
}

int main() {
	char A = 7<<_OAI_N;
	char B = 1<<(_OAI_N-3);
	char X = A;
	// char X = ((short)(A)<<_OAI_N) / B;

	_OAI_F8_Add(X, B);

	printf("(%i # %i) = %i\n", A, B, X);

	printf("(%f # %f) = %f\n", ((float)A)/(1<<_OAI_N), ((float)B)/(1<<_OAI_N), ((float)X)/(1<<_OAI_N));

	return 0;
}
