#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#define QUEUELEN (1<<10)

typedef struct connection_t {
  int sock;
} connection_t;

connection_t *conn_init(char *, int);

int conn_send(connection_t *, char *, int);
void conn_cleanup(connection_t *);

#endif
