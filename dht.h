


int dht_init(int s, int s5, const unsigned char *id, const unsigned char *v);
int dht_uninit(void);
int dht_nodes(int af, int *good_return, int *dubious_return, int * cached_return, int *incoming_return);
int dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
int dht_ping_node(struct sockaddr *sa, int salen);
