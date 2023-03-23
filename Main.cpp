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
		"shr %2, %%ax;" // TODO: I am 69% sure we can check for overflow before we even shift and increase performance

		// In multiplication and division there is a special overflow check since we don't use 8 bit registers.
		"cmp $0, %%ah;"
		"jz ._End;"
		
		// 2's complement means we are negative.
		"and $0b10000000, %%ah;"
		"cmp $0, %%ah;"
		"jnz ._HaveSign;"
		
		"movb $-128, %%al;" /* If we don't have a sign we underflowed*/
		"jmp ._End;"
		
		"._HaveSign: movb $127, %%al;" /* If we have sign we overflowed */

		"._End:"
		"mov %%al, %0"
		: "+r" (A) 
		: "r" (B), "g" (_OAI_N)
	);
}


int main() {
	char X = 3<<_OAI_N;
	
	_OAI_F8_Mul(X, 3<<_OAI_N);

	printf("%f\n", ((float)X)/(1<<_OAI_N));

	return 0;
}
