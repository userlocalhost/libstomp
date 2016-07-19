#include <stomp/connection.h>
#include <stomp/common.h>
#include <stomp/frame.h>
#include <stomp/list.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

static frame_t *alloc_frame() {
  frame_t *f;

  f = (frame_t *)malloc(sizeof(frame_t));
  if(f != NULL) {
    f->cmd = NULL;

    // mainly used by reciever in parsing processing
    f->status = 0;

    INIT_LIST_HEAD(&f->h_headers);
    INIT_LIST_HEAD(&f->h_body);
    INIT_LIST_HEAD(&f->list);

    pthread_mutex_init(&f->mutex_headers, NULL);
    pthread_mutex_init(&f->mutex_body, NULL);
  }

  return f;
}

static void free_list_entry(struct list_head *head) {
  struct data_entry *entry, *e;

  list_for_each_entry_safe(entry, e, head, list) {
    list_del(&entry->list);
    free(entry->data);
  }
}

static int set_frame_data(struct list_head *head, pthread_mutex_t *mutex, char *data, int len) {
  struct data_entry *entry;

  entry = (struct data_entry *)malloc(sizeof(struct data_entry));
  if(entry == NULL) {
    return RET_ERROR;
  }
  INIT_LIST_HEAD(&entry->list);

  strncpy(entry->data, data, len);
  entry->len = len;

  pthread_mutex_lock(mutex);
  {
    list_add_tail(&entry->list, head);
  }
  pthread_mutex_unlock(mutex);

  return RET_SUCCESS;
}

static int do_send_data(connection_t *conn, struct list_head *head, pthread_mutex_t *mutex) {
  struct data_entry *entry;
  int sent_bytes = 0;

  pthread_mutex_lock(mutex);
  list_for_each_entry(entry, head, list) {
    sent_bytes += conn_send(conn, entry->data, entry->len);
  }
  pthread_mutex_unlock(mutex);

  return sent_bytes;
}

frame_t *frame_init() {
  return alloc_frame();
}

int frame_set_cmd(frame_t *frame, char *data, int len) {
  assert(frame != NULL);
  assert(data != NULL);

  if(len > LD_MAX) {
    return RET_ERROR;
  }

  if(frame->cmd == NULL) {
    frame->cmd = (char *)malloc(LD_MAX);
    if(frame->cmd == NULL) {
      return RET_ERROR;
    }
    memset(frame->cmd, 0, LD_MAX);
  }

  strncpy(frame->cmd, data, len);
  frame->cmd_len = len;

  return RET_SUCCESS;
}

int frame_set_header(frame_t *frame, char *data, int len) {
  assert(frame != NULL);
  assert(data != NULL);

  if(len > LD_MAX) {
    return RET_ERROR;
  }

  return set_frame_data(&frame->h_headers, &frame->mutex_headers, data, len);
}

int frame_set_body(frame_t *frame, char *data, int len) {
  int remained_size = len;
  int offset = 0;

  assert(frame != NULL);
  assert(data != NULL);

  while(remained_size > 0) {
    int data_size;
    
    data_size = remained_size;
    if(remained_size > LD_MAX) {
      data_size = LD_MAX;
    }

    if(set_frame_data(&frame->h_body, &frame->mutex_body, data + offset, data_size) == RET_ERROR) {
      return RET_ERROR;
    }

    remained_size -= data_size;
    offset += data_size;
  }

  return RET_SUCCESS;
}

int frame_send(frame_t *frame, connection_t *conn) {
  int sent_bytes = 0;

  assert(frame != NULL);
  assert(conn != NULL);

  // send command
  sent_bytes += conn_send(conn, frame->cmd, frame->cmd_len);

  // send separation of headers
  sent_bytes += conn_send(conn, "\n", 1);

  // send headers data
  sent_bytes += do_send_data(conn, &frame->h_headers, &frame->mutex_headers);

  // send separation of headers
  sent_bytes += conn_send(conn, "\n", 1);

  // send body data
  sent_bytes += do_send_data(conn, &frame->h_body, &frame->mutex_body);

  // send determination of frame
  sent_bytes += conn_send(conn, "\0", 1);

  return sent_bytes;
}

void frame_free(frame_t *f) {
  free_list_entry(&f->h_headers);
  free_list_entry(&f->h_body);
  free(f->cmd);
  free(f);
}
