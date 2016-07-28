#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <stomp/list.h>
#include <pthread.h>

#define QUEUELEN (1<<16)
#define CONNECTION_BUFFER_SIZE (1<<22)

typedef struct connection_t {
  int sock;
  int buffer_index;
  char *buffer;
} connection_t;

int conn_send(connection_t *, char *, int);
connection_t *conn_init(char *, int);
void conn_cleanup(connection_t *);

#endif
