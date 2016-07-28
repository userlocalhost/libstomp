#include <stomp/connection.h>
#include <stomp/common.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static connection_t *alloc_conn() {
  connection_t *c;

  c = (connection_t *)malloc(sizeof(connection_t));
  if(c == NULL) {
    return NULL;
  }

  c->sock = 0;
  c->buffer = (char *)malloc(CONNECTION_BUFFER_SIZE);
  if(c->buffer == NULL) {
    free(c);
    return NULL;
  }
  c->buffer_index = 0;

  memset(c->buffer, 0, CONNECTION_BUFFER_SIZE);

  return c;
}

static void free_conn(connection_t *c) {
  free(c->buffer);
  free(c);
}

// Input data to connection buffer, this returns
//   RET_SUCCESS : stored data to connection buffer safely
//   RET_ERROR   : couldn't store data because connection buffer is full
static int input_buffer(connection_t *c, char *data, int len) {
  int ret = RET_ERROR;

  if(c->buffer_index + len < CONNECTION_BUFFER_SIZE) {
    strncpy(c->buffer + c->buffer_index, data, len);
    c->buffer_index += len;
    ret = RET_SUCCESS;
  }

  return ret;
}
static void clear_buffer(connection_t *c) {
  memset(c->buffer, 0, c->buffer_index);
  c->buffer_index = 0;
}
static int send_buffer(connection_t *c) {
  int ret = 0;

  if(c->buffer_index > 0) {
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(c->sock, &fds);
    if(select(c->sock + 1, NULL, &fds, NULL, NULL) < 0) {
      printf("[error] socket is not ready to send\n");
    }

    ret = send(c->sock, c->buffer, c->buffer_index, MSG_DONTWAIT);
    if(ret == -1) {
      switch(errno) {
        case EAGAIN:
          printf("[warning] errno == EAGAIN (retring)\n");
          ret = send(c->sock, c->buffer, c->buffer_index, 0);
          break;
        default:
          printf("send failed: %s\n", strerror(errno));
      }
    }
  }
  clear_buffer(c);
  return ret;
}
static int is_terminated(connection_t *c) {
  return (c->buffer_index > 0 && c->buffer[c->buffer_index] == '\0');
}

int conn_send(connection_t *c, char *data, int len) {
  assert(c != NULL);

  if(input_buffer(c, data, len) == RET_ERROR) {
    send_buffer(c);

    input_buffer(c, data, len);
  }

  if(is_terminated(c)) {
    send_buffer(c);
  }

  return len;
}
connection_t *conn_init(char *host, int port) {
  struct sockaddr_in addr;
  connection_t *conn = NULL;
  int sd, sock_opt;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return NULL;
  }

  sock_opt = 1;
  if(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&sock_opt, sizeof(sock_opt)) < 0) {
    printf("[warning] Couldn't setsockopt(TCP_NODELAY)\n");
  }

  sock_opt = 1 << 20;
  if(setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_opt, sizeof(sock_opt)) < 0) {
    printf("[warning] Couldn't customize(SO_SNDBUF)\n");
  }
  if(setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt, sizeof(sock_opt)) < 0) {
    printf("[warning] Couldn't customize(SO_SNDBUF)\n");
  }
 
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  if(connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return NULL;
  }

  conn = alloc_conn();
  if(conn != NULL) {
    conn->sock = sd;
  }

  return conn;
}
void conn_cleanup(connection_t *conn) {
  if(conn != NULL) {
    if(conn->sock > 0) {
      close(conn->sock);
    }
  }

  free_conn(conn);
}
