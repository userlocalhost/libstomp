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

static session_t *alloc_session() {
  session_t *session;

  session = (session_t *)malloc(sizeof(session_t));
  if(session != NULL) {
    session->conn = NULL;
    session->receiving_worker_status = 0;

    INIT_LIST_HEAD(&session->h_frames);
    pthread_mutex_init(&session->mutex_frames, NULL);
  }

  return session;
}

session_t *stomp_init() {
  session_t *session;

  session = alloc_session();

  pthread_create(&session->tid_receiving, NULL, receiving_worker, session);

  return session;
}

void stomp_cleanup(session_t *session) {
  conn_cleanup(session->conn);

  session->receiving_worker_status = WORKER_STATUS_STOP;

  pthread_cancel(session->tid_receiving);
  pthread_join(session->tid_receiving, NULL);

  free(session);
}

int stomp_connect(session_t *session, char *host, int port, char *userid, char *passwd) {
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

  frame_set_cmd(frame, "CONNECT\n", 8);
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

frame_t *stomp_recv(session_t *session) {
  struct timespec time;
  time_t timeout;

  if(clock_gettime(CLOCK_REALTIME, &time) == 0) {
    timeout = time.tv_sec + DEFAULT_TIMEOUT_SEC;

    do {
      if(! list_empty(&session->h_frames)) {
        frame_t *frame = list_first_entry(&session->h_frames, frame_t, list);

        pthread_mutex_lock(&session->mutex_frames);
        {
          list_del(&frame->list);
        }
        pthread_mutex_unlock(&session->mutex_frames);

        return frame;
      }

      clock_gettime(CLOCK_REALTIME_COARSE, &time);
    } while(time.tv_sec < timeout);
  }

  return NULL;
}

int stomp_disconnect(session_t *session) {
  return RET_SUCCESS;
}
