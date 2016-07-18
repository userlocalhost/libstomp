#include <stomp/connection.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

static connection_t *alloc_conn() {
  connection_t *c;

  c = (connection_t *)malloc(sizeof(connection_t));
  if(c != NULL) {
    c->sock = 0;
  }

  return c;
}

static void free_conn(connection_t *c) {
  free(c);
}

connection_t *conn_init(char *host, int port) {
  struct sockaddr_in addr;
  connection_t *conn = NULL;
  int sd;
 
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return NULL;
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

int conn_send(connection_t *c, char *data, int len) {
  assert(c != NULL);

  return send(c->sock, data, len, 0);
}

void conn_cleanup(connection_t *conn) {
  if(conn != NULL) {
    if(conn->sock > 0) {
      close(conn->sock);
    }
  }

  free_conn(conn);
}
