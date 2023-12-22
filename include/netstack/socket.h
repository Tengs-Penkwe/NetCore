#ifndef __NETSTACK_SOCKET_H__
#define __NETSTACK_SOCKET_H__

#include <stdint.h>
#include <stddef.h>
#include <netstack/def.h>

typedef struct socket {
    // sa_family_t             family;        // Address family (AF_INET, AF_INET6, etc.)
    int                     type;          // Socket type (SOCK_STREAM, SOCK_DGRAM, etc.)
    int                     protocol;      // Protocol (IPPROTO_TCP, IPPROTO_UDP, etc.)
    void*                   server;
    // struct sockaddr_storage addr;          // Socket address
    // socklen_t               addr_len;      // Length of the address
    // struct socket_ops      *ops;           // Function pointers for socket operations
    // void                   *private_data;  // Protocol-specific data
    // ... other fields for managing state, buffers, etc.
} Socket;

// struct socket_ops {
//     int (*bind)(struct socket *sock, struct sockaddr *addr, int addr_len);
//     int (*connect)(struct socket *sock, struct sockaddr *addr, int addr_len, int flags);
//     ssize_t (*send)(struct socket *sock, const void *buf, size_t len, int flags);
//     ssize_t (*recv)(struct socket *sock, void *buf, size_t len, int flags);
//     int (*listen)(struct socket *sock, int backlog);
//     int (*accept)(struct socket *sock, struct socket **new_sock, int flags);
//     int (*close)(struct socket *sock);
//     // ... other operations like setsockopt, getsockopt, etc.
// };
//

__BEGIN_DECLS

int     accept(int socket_fd, struct sockaddr *address, socklen_t *address_len);
int     bind(int socket_fd, const struct sockaddr *address, socklen_t address_len);
int     connect(int socket_fd, const struct sockaddr *server_address, socklen_t address_len);
int     getpeername(int socket_fd, struct sockaddr *peer_address, socklen_t *address_len);
int     getsockname(int socket_fd, struct sockaddr *local_address, socklen_t *address_len);
int     getsockopt(int socket_fd, int level, int option_name, void *option_value, socklen_t *option_len);
int     listen(int socket_fd, int backlog);
ssize_t recv(int socket_fd, void *buffer, size_t length, int flags);
ssize_t recvfrom(int socket_fd, void *buffer, size_t length, int flags, struct sockaddr *src_address, socklen_t *address_len);
ssize_t recvmsg(int socket_fd, struct msghdr *message, int flags);
ssize_t send(int socket_fd, const void *message, size_t length, int flags);
ssize_t sendmsg(int socket_fd, const struct msghdr *message, int flags);
ssize_t sendto(int socket_fd, const void *message, size_t length, int flags, const struct sockaddr *dest_address, socklen_t address_len);
int     setsockopt(int socket_fd, int level, int option_name, const void *option_value, socklen_t option_len);
int     shutdown(int socket_fd, int how);
int     sockatmark(int socket_fd);
int     socket(int domain, int type, int protocol);
int     socketpair(int domain, int type, int protocol, int socket_vector[2]);

__END_DECLS


#endif //__NETSTACK_SOCKET_H__
