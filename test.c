#include <stdio.h>

/* Determine whether id1 or id2 is closer to ref */
static int 
xorcmp(const unsigned char *id1,
	   const unsigned char *id2,
	   const unsigned char *ref){
	int i;
	for(i=0;i<20;i++){
		unsigned char xor1, xor2;
		if(id1[i]==id2[i]){
			continue;
		}
		xor1 = id1[i] ^ ref[i];
		xor2 = id2[i] ^ ref[i];
		if(xor1 < xor2){
			return -1;
		}else{
			return 1;
		}
	}
	return 0;
}

int main(){
	char *id1="abce", *id2="abcd", *ref="efgh";
	printf("resutl = %d\n", xorcmp(id1, id2, ref));

	return 0;
}
