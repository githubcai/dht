#define MAX_BOOTSTRAP_NODES 20
static struct sockaddr_storage bootstrap_nodes[MAX_BOOTSTRAP_NODES];
static int num_bootstrap_nodes = 0;

static volatile sig_atomic_t dumping = 0, searching = 0, exitting = 0;

static void
sigdump(int signo){
	dumping = 1;
}

static void
sigtest(int signo){
	searching = 1;
}

static void sigexit(int signo){
	exitting = 1;
}

static void
init_signals(void){
	struct sigaction sa;
	sigset_t ss;

	sigemptyset(&&ss);
	sa.sa_handler = sigdump;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptytest(&ss);
	sa.sa_handler = sigtest;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGUSR2, &sa, NULL);

	sigemptytest(&ss);
	sa.sa_handler = sigexit;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}

const unsigned char hash[20] = {
	0x54, 0x57, 0x87, 0x89, 0xdf, 0xc4, 0x23, 0xee, 0xf6, 0x03,
	0x1f, 0x81, 0x94, 0xa9, 0x3a, 0x16, 0x98, 0x8b, 0x72, 0x7b
};

/* The callback function is called by the DHT whenever something
   interesting happens. Right now, it only happens when we get a new value
   or when a search completes, but this may be extended in future versions */
static void
callback(void *closure,
		 int event,
		 const unsigned char *info_hash,
		 const void *data, size_t data_len){
	if(event == DHT_EVENT_SEARCHDONE){
		printf("Search done.\n");
	}else if(event == DHT_EVENT_VALUES){
		printf("Received %d values.\n", (int)(data_len / 6));
	}
}

static unsigned char buf[4096];

int main(int argc, char **argv){

}
