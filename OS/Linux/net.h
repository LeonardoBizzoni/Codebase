#ifndef BASE_OS_LINUX_NETWORKING_H
#define BASE_OS_LINUX_NETWORKING_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef enum {
  TRANS_INVALID = 0,
  TCP,
  UDP
} TransportProtocol;

typedef enum {
  NET_INVALID = 0,
  IPv4,
  IPv6
} NetworkProtocol;

typedef struct {
  i32 fd;
  TransportProtocol transprot;
  NetworkProtocol netprot;

  union {
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
  } dest;
} Socket;

inline fn Socket net_connect(TransportProtocol transprot, NetworkProtocol netprot,
			     String8 dest_addr, u16 port) {
  i32 domain, type;

  switch (netprot) {
    case IPv4: {
      domain = AF_INET;
    } break;
    case IPv6: {
      domain = AF_INET6;
    } break;
    default: {
      return (Socket) {0};
    }
  }
  switch (transprot) {
    case TCP: {
      type = SOCK_STREAM;
    } break;
    case UDP: {
      type = SOCK_DGRAM;
    } break;
    default: {
      return (Socket) {0};
    }
  }

  Socket res = {
    .fd = socket(domain, type, 0),
    .transprot = transprot,
    .netprot = netprot,
  };

  switch (netprot) {
  case IPv4: {
    res.dest.v4.sin_addr.s_addr = inet_addr((const char *)dest_addr.str);
    res.dest.v4.sin_port = htons(port);
    res.dest.v4.sin_family = domain;

    if (connect(res.fd, (struct sockaddr *)&res.dest.v4,
	       sizeof(res.dest.v4)) < 0) {
      return (Socket) {0};
    }
  } break;
  case IPv6: {
    printf("IPv6 not yet implemented\n");
    return (Socket) {0};
  } break;
  default: {
    return (Socket) {0};
  }
  }

  return res;
}

inline fn void net_send(Socket *sock, usz size, u8 *bytes) {
  switch (sock->netprot) {
  case IPv4: {
    sendto(sock->fd, bytes, size, 0, 0, sizeof(sock->dest.v4));
  } break;
  case IPv6: {
    sendto(sock->fd, bytes, size, 0, 0, sizeof(sock->dest.v6));
  } break;
  default: {
    fprintf(stderr, "Network protocol not supported\n");
  }
  }
}

#endif
