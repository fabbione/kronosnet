#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include "utils.h"
#include "vty.h"

int knet_vty_init_listener(const char *ip_addr, unsigned short port)
{
	int sockfd = -1, sockopt = 1;
	int socktype = SOCK_STREAM | SOCK_CLOEXEC;
	int af_family = AF_INET6;
	int salen = 0, err = 0;
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;

	/* handle sigpipe if we decide to use KEEPALIVE */

	/*
	 * I REALLY HATE MYSELF FOR WRITING THIS PIECE OF CRAP
	 * but it gets the job done
	 */

	if ((ip_addr) &&
	    (strlen(ip_addr)) &&
	    (!strchr(ip_addr, ':'))) {
		af_family = AF_INET;
	}

	sockfd = socket(af_family, socktype, 0);
	if ((sockfd < 0) &&
	    (errno == EAFNOSUPPORT) &&
	    (af_family = AF_INET6)) {
		af_family = AF_INET;
		sockfd = socket(af_family, socktype, 0);
	}
	if (sockfd < 0)
		return sockfd;

	err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
			 (void *)&sockopt, sizeof(sockopt));
	if (err)
		goto out_clean;

	memset(&sa, 0, sizeof(struct sockaddr));
	if (af_family == AF_INET) {
		salen = sizeof(struct sockaddr_in);
		memset(&sin, 0, sizeof(struct sockaddr_in));
		sin.sin_family = af_family;
		if ((!ip_addr) || (!strlen(ip_addr)))
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		else
			if (inet_pton(af_family, ip_addr, &sin.sin_addr) <= 0) {
				err = -1;
				goto out_clean;
			}
		sin.sin_port = htons(port);
		memcpy(&sa, &sin, sizeof(struct sockaddr_in));
	} else {
		salen = sizeof(struct sockaddr_in6);
		memset(&sin6, 0, sizeof(struct sockaddr_in6));
		sin6.sin6_family = af_family;
		sin6.sin6_port = htons(port);
		if ((!ip_addr) || (!strlen(ip_addr)))
			memcpy(&sin6.sin6_addr, &in6addr_any, sizeof(struct in6_addr));
		else
			if (inet_pton(af_family, ip_addr, &sin6.sin6_addr) <= 0) {
				err = -1;
				goto out_clean;
			}
		memcpy(&sa, &sin6, sizeof(struct sockaddr_in6));
	}

	err = bind(sockfd, &sa, salen);
	if (err)
		goto out_clean;

	err = listen(sockfd, 0);
	if (err)
		goto out_clean;

	return sockfd;

out_clean:
	if (sockfd >= 0)
		close(sockfd);

	return err;
}