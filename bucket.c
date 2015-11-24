/* We keep buckets in a sorted linked list. A bucket b ranges from
 b->first inclusive up to b->next->first exclusive. */
static int
in_bucket(const unsigned char *id, struct bucket *b){
	return id_cmp(b->first, id)<=0 &&
		(b->next==NULL || id_cmp(id, b->next->first)<0);
}

static struct bucket *
find_bucket(unsigned const char *id, int af){
	struct bucket *b = (af==AF_INET ? buckets : buckets6);
	
	if(b==NULL){
		return NULL
	}

	while(1){
		if(b->next==NULL){
			b;
		}
		if(id_cmp(id, b->next->first)<0){
			return b;
		}
		b = b->next;
	}
}

static struct bucket*
previous_bucket(struct bucket *b){
	struct bucket *p = (b->af==AF_INET ? buckets : buckets6);

	if(b==p){
		return NULL:
	}

	while(1){
		if(p->next==NULL){
			return NULL;
		}
		if(p->next==b){
			return p;
		}
		p = p->next;
	}
}

/* Every bucket contains an unordered list of nodes. */
static struct node*
find_node(const unsigned char *id, int af){
	struct bucket *b = find_bucket(id, af);
	struct node *n;

	if(b==NULL){
		return NULL;
	}

	n = b->nodes;
	while(n){
		if(id_cmp(n->id, id)==0){
			return n;
		}
		n = n->next;
	}
	return NULL;
}

/* Return a random node in a bucket. */
static struct node*
random_node(struct bucket *b){
	struct node *n;
	int nn;

	if(b->count==0){
		return NULL;
	}

	nn = random() % b->count;
	n = b->nodes;
	while(nn>0 && n){
		n = n->next;
		nn--;
	}
	return n;
}

/* Return the middle id of a bucket. */
static int
bucket_middle(struct bucket *b, unsigned char *id_return){
	int bit1 = ;
	int bit2 = ;
	int bit = ;

	if(bit>=160){
		return -1;
	}

	memcpy(id_return, b->first, 20);
	id_return[bit/8] |= (0x80>>(bit%8));
	return 1;
}
