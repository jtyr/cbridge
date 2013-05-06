/*
 * Name: cbridge.c
 * Version: 0.4
 * Description: UDP bridge for multicast/unicast networks
 * Programmer: Jiri Tyr
 * Last update: 29.10.2009
 */


#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>


/* size of the UDP video packet */
#define MSGBUFSIZE 2000
#define DSTARRAYSIZE 256
#define DEFTTL 16


/* global variable */
unsigned int DEBUG = 0;


/* make connection */
void udp_connect(int *fd, struct sockaddr_in *addr, char *host, int port, char *type) {
	u_int yes = 1;
	u_char ttl = DEFTTL;
	int ip[4], i, j, n = 0, d = 0;
	char token[3];
	struct ip_mreq mreq;
	struct sockaddr_in addr_local;

	if (DEBUG) fprintf(stderr, "I: Preparing connection to %s:%d...\n", host, port);

	/* create what looks like an ordinary UDP socket */
	if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("E: Socket error");
		exit(1);
	}

	/* allow multiple sockets to use the same ADDRESS */
	if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("E: Set reuse address error");
		exit(1);
	}

	/* set up destination address */
	if (strcmp(type, "server") == 0) {
		/* local address and specific port */
		memset(addr, 0, sizeof(*addr));
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = inet_addr(host);
		addr->sin_port = htons(port);

		/* bind local address and specific port */
		if (bind(*fd, (struct sockaddr *) addr, sizeof(*addr)) < 0) {
			perror("E: Bind error");
			exit(1);
		}
	} else {
		/* server connection = remote address and specific port */
		memset(addr, 0, sizeof(*addr));
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = inet_addr(host);
		addr->sin_port = htons(port);

		/* local address and port 0*/
		memset(&addr_local, 0, sizeof(addr_local));
		addr_local.sin_family = AF_INET;
		addr_local.sin_addr.s_addr = htonl(INADDR_ANY);
		addr_local.sin_port = htons(0);

		/* bind local address */
		if (bind(*fd, (struct sockaddr *) &addr_local, sizeof(addr_local)) < 0) {
			perror("E: Bind error");
			exit(1);
		}
	}

	/* split host IP */
	for (i=0; i<=strlen(host); i++) {
		if (host[i] == '.' || host[i] == '\0') {
			ip[d] = atoi(token);
			for (j=0;j<sizeof(token); j++) {
				token[j] = ' ';
			}
			n = 0;
			d++;
		} else {
			token[n] = host[i];
			n++;
		}
	}

	/* use setsockopt() to request that the kernel join a multicast group */
	/* only for multicast addresses 224.0.0.0-239.255.255.255 */
	if (ip[0] >= 223 && ip[0] <= 239) {
		if (DEBUG) fprintf(stderr, "    * IS multicast\n");

		mreq.imr_multiaddr.s_addr = inet_addr(host);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(*fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
			perror("E: Add IP source membership error");
			exit(1);
		}

		if (setsockopt(*fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
			perror("E: Multicast TTL error");
			exit(1);
		}
	} else {
		if (DEBUG) fprintf(stderr, "    * is NOT multicast\n");
	}

	if (DEBUG) fprintf(stderr, "    * connection done\n");
}


/* return ip from hostname */
char* get_ip(char *host) {
	char *ip;
	struct hostent *h;
	struct sockaddr_in server;

	if (DEBUG) fprintf(stderr, "I: Search IP of %s\n", host);

	if ((h = gethostbyname(host)) == NULL) {
		perror("E: Resolving hostname error");
		exit(1);
	}

	memcpy(&server.sin_addr, h->h_addr_list[0], h->h_length);
	ip = inet_ntoa(server.sin_addr);

	if (DEBUG) fprintf(stderr, "   * found %s\n", ip);

	return ip;
}


/* help */
void help(void) {
	puts("Usage: cbridge [OPTIONS]\n");
	puts("OPTIONS:");
	puts("  -s, --src=VAL          source address");
	puts("  -d, --dst=VAL          destination address");
	puts("  -p, --sport=VAL        source port");
	puts("  -r, --dport=VAL        destination port");
	puts("  -e, --debug            debug mode");
	puts("  -h, --help             display this help and exit\n");
	puts("Examples:");
	puts("cbridge --src=239.194.1.1 --sport=1234 --dst=137.138.165.238 --dport=1234");
	puts("cbridge --src=239.194.1.1 --sport=1234 --dst=137.138.165.237 --dport=1235 \\");
	puts("                                       --dst=137.138.165.238 --dport=1236 \\");
	puts("                                       --dst=137.138.165.239 --dport=1237\n");
	puts("Report bugs to <jiri(dot)tyr(at)gmail(dot)com>.");

	exit(-1);
}


/* main function */
int main(int argc, char *argv[]) {
	int c, datalen;
	unsigned int i, src_addrlen;
	char databuf[MSGBUFSIZE];
	int bool_send = 1, bool_read = 1;
	int dst_param = 0;

	/* socket variables */
	int src_fd, *dst_fd;
	struct sockaddr_in src_addr, *dst_addr;
	char *src_host = "", **dst_host;
	int src_port = 0, *dst_port;

	/* allocate memory for all dst arrays */
	dst_host = (char **) calloc(DSTARRAYSIZE, sizeof(char *));
	dst_port = (int *) calloc(DSTARRAYSIZE, sizeof(int));
	dst_fd = (int *) calloc(DSTARRAYSIZE, sizeof(int));
	dst_addr = (struct sockaddr_in *) calloc(DSTARRAYSIZE, sizeof(struct sockaddr_in));

	/* reset all dst_addr and dst_port */

	/* parse command line options */
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "src",
				required_argument,
				NULL,
				's'
			},
			{ "dst",
				required_argument,
				NULL,
				'd'
			},
			{ "sport",
				required_argument,
				NULL,
				'p'
			},
			{ "dport",
				required_argument,
				NULL,
				'r'
			},
			{ "debug",
				optional_argument,
				NULL,
				'e'
			},
			{ "help",
				no_argument,
				NULL,
				'h'
			}
		};

		c = getopt_long(argc, argv, "s:d:p:r:eh", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 's': {
					char *ip;
					ip = (char *) get_ip(optarg);
					//free(src_host);
					src_host = calloc((strlen(ip)+1), sizeof(char));
					strcpy(src_host, ip);
				}
				break;

			case 'd': {
					char *ip;
					ip = (char *) get_ip(optarg);
					dst_host[dst_param] = (char*) calloc((strlen(ip)+1), sizeof(char));
					strcpy(dst_host[dst_param], ip);
				}
				break;

			case 'p':
				src_port = atoi(optarg);
				break;

			case 'r': {
					dst_port[dst_param] = atoi(optarg);
					dst_param++;
				}
				break;

			case 'e':
				DEBUG = 1;
				break;

			case 'h':
				help();
				break;

			default:
				fprintf(stderr, "E: getopt returned character code 0%o\n", c);
				exit(-1);
				break;
		}
	}

	/* if no options or wrong values of the options, show help */
	if (optind == 1 || strlen(src_host) == 0) {
		help();
	}

	/* if wrong values of the options, show help */
	for (i=0; i<dst_param; i++) {
		if (strlen(dst_host[i]) == 0 || src_port < 0 || src_port > 65536 || dst_port[i] < 0 || dst_port[i] > 65536) {
			help();
		}
	}

	/* identify non-option elements */
	if (DEBUG && optind < argc) {
		fprintf(stderr, "W: Non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "'%s' ", argv[optind++]);
		fprintf(stderr, "\n");
	}

	/* connect to SRC and DST IP */
	udp_connect(&src_fd, &src_addr, src_host, src_port, "server");
	for (i=0; i<dst_param; i++) {
		udp_connect(&dst_fd[i], &dst_addr[i], dst_host[i], dst_port[i], "client");
	}

	/* now just enter a read-print loop */
	while (1) {
		if (DEBUG && bool_read) {
			fprintf(stderr, "I: Reading data...\n");
		}

		src_addrlen = sizeof(src_addr);
		if ((datalen = recvfrom(src_fd, databuf, MSGBUFSIZE, 0, (struct sockaddr *) &src_addr, &src_addrlen)) < 0) {
			perror("E: SRC read error");
			exit(1);
		}

		/* if there are some data, send them */
		if (datalen > 0) {
			if (DEBUG && bool_read) {
				fprintf(stderr, "    * got data len=%d\n", datalen);
				bool_read = 0;
			}

			if (DEBUG && bool_send) {
				fprintf(stderr, "I: Sending data (datalen=%d)...\n", datalen);
			}

			for (i=0; i<dst_param; i++) {
				if (sendto(dst_fd[i], databuf, datalen, 0, (struct sockaddr *) &dst_addr[i], sizeof(dst_addr[i])) < 0) {
					perror("E: Sending datagram message error");
				}
			}

			if (DEBUG && bool_send) {
				fprintf(stderr, "    * data sent\n");
				bool_send = 0;
			}
		}
	}

	return EXIT_SUCCESS;
}
