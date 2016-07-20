#ifndef __FRAME_H__
#define __FRAME_H__

#include <stomp/connection.h>
#include <stomp/list.h>
#include <pthread.h>

#define LD_MAX (1024)
#define MAX_DATA_CHUNK (1<<22)

typedef struct frame_t {
  char *cmd;
  int cmd_len;
  int status;
  struct list_head h_headers;
  struct list_head h_body;
  pthread_mutex_t mutex_headers;
  pthread_mutex_t mutex_body;

  // some frames (mainly received frame) are contained in a bucket.
  struct list_head list;
} frame_t;

struct data_entry {
  char *data;
  int len;
  struct list_head list;
};

frame_t *frame_init();
int frame_set_cmd(frame_t *, char *, int);
int frame_set_header(frame_t *, char *, int);
int frame_set_body(frame_t *, char *, int);
int frame_send(frame_t *, connection_t *);
void frame_free(frame_t *);

#endif
