#include <sys/socket.h>
// AF_UNIX, AF_NS, AF_IMPLINK, ...
struct sockaddr
{
	u_short	sa_family;		/* address family */
	char	sa_date[14];	/* protocol specific information */
};

#include <netline/in.h>
struct in_addr
{
	u_long	s_addr;			/* 32-bit address, network byte order */
};

struct sockaddr_in
{
	short	sin_family;			/* AF_INET family */
	u_short	sin_port;			/* 16 bit port number */
	struct	in_addr sin_addr;	/* 32 bit host address */
	char	sin_zero[8];		/* set to zero, not used */
};
