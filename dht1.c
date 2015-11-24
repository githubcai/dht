static int
id_cmp(const unsigned char *restrict id1,
	   const unsigned char *restrict id2){
	/* Memcmp is guaranteed to perform an unsigned comparison. */
	return memcmp(id1, id2, 20);
}

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

/* Find the lowest 1 bit in an id */
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
		if((id[j] & (0x80>>j)) != 0){
			break;
		}
	}
	return 8 * i + j;
}
