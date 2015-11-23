
typedef void dht_callback(void *closure, int event, const unsigned char *info_hash, const void *data, size_t data_len);

#define DHT_EVENT_NONE          0
#define DHT_EVENT_VALUES        1
#define DHT_EVENT_VALUES6       2
#define DHT_EVENT_SEARCH_DONE   3
#define DHT_EVENT_SEARCH_DONE6  4

int dht_init(int s, int s5, const unsigned char *id, const unsigned char *v);
int dht_uninit(void);
int dht_nodes(int af, int *good_return, int *dubious_return, int * cached_return, int *incoming_return);
int dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
int dht_ping_node(struct sockaddr *sa, int salen);

int dht_get_nodes(struct sockaddr_in *sin, int *num, struct sockaddr_in6, *sin6, int *num6);

int dht_search(const unsigned char *id, int port, int af, dht_callback *callback, void *closure);

int dht_dump_tables(FILE *f);
int dht_periodic(const void *buf, size_t buflen, 
                 const struct sockaddr *from, int fromlen,
                 time_t *tosleep, dht_callback *callback, void *closure);
