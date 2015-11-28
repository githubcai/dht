#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "dht.h"

static void
usage(){
    printf("Usage: dht-example [-q] [-b address]...\n"
        "                  port [address port]...\n");
    exit(1);
    
}

static void
print_hex(FILE *f, const unsigned char *buf, int buflen)
{
    int i;
    for(i = 0; i < buflen; i++)
        fprintf(f, "%02x", buf[i]);
    printf("\n");
}

static void
callback(void *closure,
         int event,
         const unsigned char *info_hash,
         const void *data, size_t data_len){
    if(event == DHT_EVENT_SEARCH_DONE){
        printf("Search done.\n");
    }else if(event == DHT_EVENT_VALUES){
        printf("Received %d values.\n", (int)data_len / 6);
    }
}

int
main(int argc, char *argv[]){
    int sfd, fd;
    int rc, i;
    int opt, quiet = 0, port;
    struct sockaddr_in sin;
    unsigned char myid[20];
    
    if(argc < 2){
        usage();
    }

    while(1){
        int opt_result = 1;
        opt = getopt(argc, argv, "qb:");
        if(opt < 0){
            break;
        }

        switch(opt){
        case 'q': quiet = 1; break;
        case 'b':{
            char buf[16];
            int rc;
            rc = inet_pton(AF_INET, optarg, buf);
            if(rc == 1){
                memcpy(&sin.sin_addr, buf, 4);
                break;
            }
            opt_result = 0;
        }
        default:
            opt_result = 0;
        }
        if(opt_result == 0){
            usage();
        }
    }

    i = optind;
    if(argc < i + 1){
        usage();
    }

    port = atoi(argv[i++]);
    if(port <= 0 || port >= 0x10000){
        usage();
    }

    while(i < argc){
        struct addrinfo, hints, *info, *infop;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_INET:
        rc = getaddrinfo(argv[i], argv[i + 1], &hints, &info);
        if(rc != 0){
        
        }
    }

    fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0){
        perror("open(random)");
        exit(1);
    }
    rc = read(fd, myid, 20);
    if(rc < 0){
        perror("read(random)");
        exit(EXIT_FAILURE);
    }
    close(fd);
    print_hex(stdout, myid, sizeof(char) * 20);

    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sfd < -1){
        perror("socket(IPv4)");
    }

    sin.sin_port = htons(port);
    rc = bind(s, (struct sockaddr*)&sin, sizeof(sin));
    if(rc < 0)

    close(sfd);

    return EXIT_SUCCESS;
}

int
dht_blacklisted(const struct sockaddr *sa, int salen){
    return 0;
}

int
dht_random_bytes(void *buf, size_t size){
    int fd, rc, save;

    fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0){
        return -1;
    }

    rc = read(fd, buf, size);

    save = errno;
    close(fd);
    errno = save;

    return rc;
}

void
dht_hash(void *hash_return, int hash_size,
         const void *v1, int len1,
         const void *v2, int len2,
         const void *v3, int len3)
{
    const char *c1 = v1, *c2 = v2, *c3 = v3;
    char key[9];                /* crypt is limited to 8 characters */
    int i;

    memset(key, 0, 9);
#define CRYPT_HAPPY(c) ((c % 0x60) + 0x20)

    for(i = 0; i < 2 && i < len1; i++)
        key[i] = CRYPT_HAPPY(c1[i]);
    for(i = 0; i < 4 && i < len1; i++)
        key[2 + i] = CRYPT_HAPPY(c2[i]);
    for(i = 0; i < 2 && i < len1; i++)
        key[6 + i] = CRYPT_HAPPY(c3[i]);
    strncpy(hash_return, crypt(key, "jc"), hash_size);
}
