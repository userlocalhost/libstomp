#ifndef __STOMP_H__
#define __STOMP_H__

#include <stomp/list.h>
#include <stomp/frame.h>
#include <stomp/connection.h>

#include <pthread.h>

typedef struct session_t {
  connection_t *conn;
  pthread_t tid_receiving;
  int receiving_worker_status;

  struct list_head h_frames;
  pthread_mutex_t mutex_frames;
} session_t;

session_t *stomp_init();
void stomp_cleanup(session_t *);

int stomp_connect(session_t *, char *, int, char *, char *);
int stomp_disconnect(session_t *);
frame_t *stomp_recv(session_t *);

#endif
