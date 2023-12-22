#ifndef __NETSTACK_DEF_H__
#define __NETSTACK_DEF_H__

#include <stdint.h>
#include <types.h>

/* <sys/socket.h>
 *
 * Types
 */
#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */

/* <sys/socket.h>
 *
 * Address families.
 */
#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes) */
#define AF_LOCAL        AF_UNIX         /* backward compatibility */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */

typedef	uint32_t	      socklen_t;
typedef uint8_t         sa_family_t;

struct sockaddr {
    uint8_t         sa_len;         /* total length */
    sa_family_t     sa_family;      /* [XSI] address family */
    char            sa_data[14];    /* [XSI] addr value (actually smaller or larger) */
};

struct msghdr {
    void		      *msg_name;		  /* optional address */
    socklen_t	     msg_namelen;		/* size of address */
    struct iovec	*msg_iov;		    /* scatter/gather array */
    int		         msg_iovlen;		/* # elements in msg_iov */
    void		      *msg_control;		/* ancillary data, see below */
    socklen_t	     msg_controllen;/* ancillary data buffer len */
    int		         msg_flags;		  /* flags on received message */
};


#endif // __NETSTACK_DEF_H__
