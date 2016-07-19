#include <stomp/list.h>
#include <stomp/stomp.h>
#include <stomp/frame.h>
#include <stomp/common.h>
#include <stomp/connection.h>
#include <stomp/receiving_worker.h>

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define DEFAULT_TIMEOUT_SEC 3

static stomp_session_t *alloc_session() {
  stomp_session_t *session;

  session = (stomp_session_t *)malloc(sizeof(stomp_session_t));
  if(session != NULL) {
    session->conn = NULL;
    session->receiving_worker_status = 0;

    INIT_LIST_HEAD(&session->h_frames);
    pthread_mutex_init(&session->mutex_frames, NULL);
  }

  return session;
}

stomp_session_t *stomp_init() {
  stomp_session_t *session;

  session = alloc_session();

  pthread_create(&session->tid_receiving, NULL, receiving_worker, session);

  return session;
}

void stomp_cleanup(stomp_session_t *session) {
  conn_cleanup(session->conn);

  session->receiving_worker_status = WORKER_STATUS_STOP;

  pthread_cancel(session->tid_receiving);
  pthread_join(session->tid_receiving, NULL);

  free(session);
}

int stomp_connect(stomp_session_t *session, char *host, int port, char *userid, char *passwd) {
  frame_t *frame;
  char hdr_userid[LD_MAX] = {0};
  char hdr_passwd[LD_MAX] = {0};
  int len_userid, len_passwd;

  assert(session != NULL);

  session->conn = conn_init(host, port);
  if(session->conn == NULL) {
    return RET_ERROR;
  }

  frame = frame_init();
  if(frame == NULL) {
    return RET_ERROR;
  }

  frame_set_cmd(frame, "CONNECT", 7);
  frame_set_header(frame, "content-length:0\n", 17);
  frame_set_header(frame, "content-type:text/plain; charset=UTF-8\n", 39);

  // set userid and password to authenticate with STOMP server
  len_userid = sprintf(hdr_userid, "login: %s\n", userid);
  len_passwd = sprintf(hdr_passwd, "passcode: %s\n", passwd);
  frame_set_header(frame, hdr_userid, len_userid);
  frame_set_header(frame, hdr_passwd, len_passwd);

  frame_send(frame, session->conn);

  frame_free(frame);

  return RET_SUCCESS;
}

int stomp_send(stomp_session_t *session, char *dest, char *data, int datalen) {
  frame_t *frame;
  char hdr_context[LD_MAX] = {0};
  int hdr_len;

  if(session->conn == NULL) {
    return RET_ERROR;
  }

  frame = frame_init();
  if(frame == NULL) {
    return RET_ERROR;
  }

  frame_set_cmd(frame, "SEND", 4);

  // set headers
  hdr_len = sprintf(hdr_context, "destination:%s\n", dest);
  frame_set_header(frame, hdr_context, hdr_len);
  hdr_len = sprintf(hdr_context, "content-len:%d\n", datalen);
  frame_set_header(frame, hdr_context, hdr_len);

  frame_set_header(frame, "content-type:text/plain; charset=UTF-8\n", 39);

  // set body
  frame_set_body(frame, data, datalen);

  frame_send(frame, session->conn);

  frame_free(frame);

  return RET_SUCCESS;
}

int stomp_subscribe(stomp_session_t *session, char *dest) {
  frame_t *frame;
  char hdr_context[LD_MAX] = {0};
  int hdr_len;

  if(session->conn == NULL) {
    return RET_ERROR;
  }

  frame = frame_init();
  if(frame == NULL) {
    return RET_ERROR;
  }

  frame_set_cmd(frame, "SUBSCRIBE", 9);

  // set headers
  hdr_len = sprintf(hdr_context, "destination:%s\n", dest);
  frame_set_header(frame, hdr_context, hdr_len);
  frame_set_header(frame, "content-length:0\n", 17);
  frame_set_header(frame, "content-type:text/plain; charset=UTF-8\n", 39);

  frame_send(frame, session->conn);

  frame_free(frame);

  return RET_SUCCESS;
}

int stomp_disconnect(stomp_session_t *session) {
  frame_t *frame;

  if(session->conn == NULL) {
    return RET_ERROR;
  }

  frame = frame_init();
  if(frame == NULL) {
    return RET_ERROR;
  }

  frame_set_cmd(frame, "DISCONNECT", 10);
  frame_set_header(frame, "content-length:0\n", 17);
  frame_set_header(frame, "content-type:text/plain; charset=UTF-8\n", 39);

  frame_send(frame, session->conn);

  frame_free(frame);

  return RET_SUCCESS;
}

frame_t *stomp_recv(stomp_session_t *session) {
  struct timespec time;
  time_t timeout;
  frame_t *frame = NULL;

  if(session->conn != NULL && clock_gettime(CLOCK_REALTIME, &time) == 0) {
    timeout = time.tv_sec + DEFAULT_TIMEOUT_SEC;

    do {
      if(! list_empty(&session->h_frames)) {
        frame = list_first_entry(&session->h_frames, frame_t, list);

        pthread_mutex_lock(&session->mutex_frames);
        {
          list_del(&frame->list);
        }
        pthread_mutex_unlock(&session->mutex_frames);

        break;
      }

      clock_gettime(CLOCK_REALTIME_COARSE, &time);
    } while(time.tv_sec < timeout);
  }

  return frame;
}
