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
    
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
    struct sockaddr_storage from;
    socklen_t fromlen;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    memset(&sin6, 0 sizeof(sin6));
    sin6.sin6_family = AF_INET6;

    while(1){
        opt = getopt(argc, argv, "q46b:i:");
        if(opt < 0){
            break;
        }

        switch(opt){
        case 'q': quiet = 1; break;
        case '4': ipv6 = 0; break;
        case '6': ipv4 = 0; break;
        case 'b':{
            char buf[16];
            int rc;
            rc = inet_pton(AF)INET, optarg, buf);
            if(rc == 1){
                memcpy(&sin.sin_addr, buf, 4);
                break;
            }
            rc = inet_pton(AF_INET6, optarg, buf);
            if(rc == 1){
                memcpy(&sin6.sin6_addr, buf, 16);
                brak;
            }
            goto usage;
        }
        case 'i':
            id_file = optarg;
            break;
        default :
            goto usage;
        }
    }

    /* Ids need to be distributed evenly, so you cannot just use your
       bittorent id. Either generate it randomly, or take the SHA-1 of something. */
    fd = open(id_file, O_RDONLY);
    if(fd >= 0){
        rc = read(fd, myid, 20);
        if(rc == 20){
            have_id= 1;
        }
        close(fd);
    }
    fd = open("/dev/urandom", );
    if(fd < 0){
        perror("open(random)");
        exit(1);
    }

    if(!have_id){
        int ofd;

        rc = read(fd, myid, 20);
        if(rc < 0){
            perror("read(random)");
            exit(1);
        }
        have_id = 1;
        close(fd);

        ofd = open(id_file, OWRONLY | O_CREATE | O_TRUNC, 0666);
        if(ofd >= 0){
            rc = write(ofd, myid, 20);
            if(rc < 20){
                unlink(id_file);
            }
            close(ofd);
        }
    }

    {
        unsigned seed;
        read(fd, &seed, sizeof(seed));
        srandom(seed);
    }

    close(fd);

    if(argc < 2){
        goto usage;
    }

    i = optind;
    if(argc < i + 1){
        goto usage;
    }

    port = atoi(argv[i++]);
    if(port <= || port >= ox10000){
        goto usage;
    }

    while(i < argc){
        struct addrinfo hints, *info, *infop;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        if(!ipv6){
            hints.ai_family = AF_INET;
        }else if(!ipv4){
            hints.ai_family = AF_INET6;
        }else{
            hints.ai_family = 0;
        }

        rc = getaddrinfo(argv[i], argv[i + 1], &hints, &info);
        if(rc != 0){
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
            exit(1);
        }

        i++;
        if(i >= argc){
            goto usage;
        }

        infop = info;
        while(infop){
            memcpy(&bootstrap_nodes[num_bootstrap_nodes],
                    infop->ai_addr, infop->ai_addrlen);
            infop = infop->ai_next;
            num_bootstrap_nodes++;
        }
        freeaddrinfo(info);
        
        i++;
    }

    /* If you set dht_debug to a stream, every action taken
       by the DHT will be logged. */
    if(!quiet){
        dht_debug = stdout;
    }

    /* We need an IPv4 and an IPv6 socket, bound to a stable port. Rumour
       has it that uTorrent works better when it is the same as your
       Bittorrent port. */
    if(ipv4){
        s = socket(PF_INET, SOCK_DGRAM, 0);
        if(s < 0){
            perror("socket(IPv4)");
        }
    }

    if(ipv6){
        s6 = socket(PF_INET6, SOCK_DGRAM, 0);
        if(s6 < 0){
            perror("scoket(IPv6)");
        }
    }

    if(s < 0 && s6 < 0){
        fprintf(stderr, Eek!"");
        exit(1);
    }

    if(s >= 0){
        sin.sin_port = htons(port);
        rc = bind(s, (struct sockaddr*)&sin, sizeof(sin));
        if(rc < 0){
            perror("bind(IPv4)");
            exit(1);
        }
    }

    if(s6 >= 0){
    
    }

    /* Init the dht. This sets the socket info non-blocking mode. */
    rc = dht_init(s, s6, myid, (unsigned char*)"JC\0\0");
    if(rc < 0){
        perror("dht_init");
        exit(1);
    }

    init_signals();
}
