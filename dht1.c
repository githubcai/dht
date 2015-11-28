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

static void
debugf(const char *format, ...){
    va_list args;
    va_start(args, format);
    if(dht_debug){
        vfprintf(dht_debug, format, args);
    }
    va_end(args);
    if(dht_debug){
        fflush(dht_debug);
    }
}

static int
is_martian(const struct sockaddr *sa){
	switch(sa->sa_family){
	case AF_INET:{
		struct sockaddr_in *sin = (struct sockaddr_in*)sa;
		const unsigned char *address = (const unsigned char*)&sin->sin_addr;
		return sin->sin_port == 0 ||
			(address[0] == 0) ||
			(address[0] == 127) ||
			((address[0] & 0xE0) == 0xE0);
	}
	case AF_IENT6:{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)sa;
		const unsigned char *address = (const unsigned char*)&sin6->sin6_addr;
		return sin6->sin6_port == 0 || 
			(address[0] == 0xFE) ||
			(address[0] == 0xFE && (address[1] & oxC0) == 0x80) ||
			(memcmp(address, zeroes, 15) == 0) &&
			(address[15] == 0 || address[15] == 1) ||
			(memcmp(address, v4prefix, 12) == 0);
	}
	default:
		return 0;
	}
}
