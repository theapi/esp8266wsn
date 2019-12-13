#define _GNU_SOURCE /* For struct ip_mreq */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define SERVER_PORT 12345
#define SERVER_GROUP "239.0.0.58"
#define MSGBUFSIZE 64

static void printArray(const uint8_t *string, int size) {
  while (size) {
    printf("%02x ", *string++);
    size--;
  }
}

int main(int argc, char *argv[]) {
  struct sockaddr_in addr;
  int fd, nbytes;
  socklen_t addrlen;
  struct ip_mreq mreq;
  uint8_t msgbuf[MSGBUFSIZE];

  /* create what looks like an ordinary UDP socket */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  /* allow multiple sockets to use the same PORT number */
  int yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("Reusing ADDR failed");
    exit(1);
  }

  /* set up destination address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); /* N.B.: differs from sender */
  addr.sin_port = htons(SERVER_PORT);

  /* Bind to receive address */
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }

  /* Use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr = inet_addr(SERVER_GROUP);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    perror("setsockopt");
    exit(1);
  }

  /* Keep listening */
  while (1) {
    addrlen = sizeof(addr);
    if ((nbytes = recvfrom(fd, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *)&addr,
                           &addrlen)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    printf("\nReceived %d bytes: ", nbytes);
    printArray(msgbuf, nbytes);
  }
}
