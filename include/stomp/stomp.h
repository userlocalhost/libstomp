#ifndef __STOMP_H__
#define __STOMP_H__

#include <stomp/list.h>
#include <stomp/frame.h>
#include <stomp/connection.h>

#include <pthread.h>

typedef struct stomp_session_t {
  connection_t *conn;
  pthread_t tid_receiving;
  int receiving_worker_status;

  struct list_head h_frames;
  pthread_mutex_t mutex_frames;
} stomp_session_t;

stomp_session_t *stomp_init();
void stomp_cleanup(stomp_session_t *);

int stomp_connect(stomp_session_t *, char *, int, char *, char *);
int stomp_send(stomp_session_t *, char *, char *, int);
int stomp_subscribe(stomp_session_t *, char *);
int stomp_disconnect(stomp_session_t *);
frame_t *stomp_recv(stomp_session_t *);

#endif
