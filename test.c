#include <stdio.h>

static int
lowbit(const unsigned char *id){
	int i, j;
	for(i=19;i>=0;i--){
		if(id[i]!=0){
			break;
		}
	}

	if(i<0){
		return -1;
	}

	for(j=7;j>=0;j--){
		if((id[i] & (0x80 >> j)) != 0){
			break;
		}
	}

	return 8 * i + j;
}

int main(){
	unsigned char *a = "1";
	printf("%x", a[0]);
//	printf("%x", a[1]);
//1	printf("%x\n", a[2]);
	printf("lowbit = %d\n", lowbit(a));
	return 0;
}
