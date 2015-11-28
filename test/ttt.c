#include <stdio.h>

void main(){
	unsigned char c[4] = {23, 24, 123, 56};
	unsigned char c1[4] = {42, 52, 12, 190};
	unsigned char result;
	int i;

	for(i=0;i<4;i++){
		printf("%02x", c[i]^c1[i]);
	}
}
