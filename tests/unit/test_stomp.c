#include "unit.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <stomp/common.h>
#include <stomp/stomp.h>
#include <stomp/frame.h>
#include <stomp/list.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 61613
#define AUTH_USERID "guest"
#define AUTH_PASSWD "guest"

#define TEST_QNAME "/queue/test"
#define CONNECTION_TIMEOUT (3)

static stomp_session_t *session;

static void check_init(void) {
  session = stomp_init();

  CU_ASSERT(session != NULL);
}

static void check_cleanup(void) {
  pthread_t worker_id;

  CU_ASSERT_FATAL(session != NULL);

  worker_id = session->tid_receiving;

  stomp_cleanup(session);

  // get thread status
  CU_ASSERT(pthread_kill(worker_id, 0) == ESRCH);
}

static void check_connect() {
  struct list_head *p;
  frame_t *frame;

  CU_ASSERT_FATAL(session != NULL);

  // checking to send CONNECT frame successfully
  CU_ASSERT(stomp_connect(session, SERVER_HOST, SERVER_PORT, AUTH_USERID, AUTH_PASSWD) == RET_SUCCESS);

  // checking to receive CONNECTED frame successfully
  frame = stomp_recv_with_timeout(session, CONNECTION_TIMEOUT);

  CU_ASSERT_FATAL(frame != NULL);
  CU_ASSERT(strncmp(frame->cmd, "CONNECTED", 9) == 0);
  CU_ASSERT(frame->cmd_len == 9);
}

static void check_send() {
  CU_ASSERT_FATAL(session != NULL);

  CU_ASSERT(stomp_send(session, TEST_QNAME, "hogefuga", 8) == RET_SUCCESS);
}

static void check_subscribe() {
  frame_t *frame;

  CU_ASSERT_FATAL(session != NULL);

  CU_ASSERT(stomp_subscribe(session, TEST_QNAME) == RET_SUCCESS);

  frame = stomp_recv(session);
  CU_ASSERT(frame != NULL);
  CU_ASSERT(strncmp(frame->cmd, "MESSAGE", 7) == 0);
  CU_ASSERT(frame->cmd_len == 7);
}

static void check_disconnect() {
  CU_ASSERT_FATAL(session != NULL);

  // checking to send DISCONNECT frame successfully
  CU_ASSERT(stomp_disconnect(session) == RET_SUCCESS);
}

int test_stomp(CU_pSuite suite) {
  suite = CU_add_suite("STOMP protocol", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "initialization", check_init);
  CU_add_test(suite, "connect to server", check_connect);
  CU_add_test(suite, "send message to server", check_send);
  CU_add_test(suite, "subscribe message from server", check_subscribe);
  CU_add_test(suite, "disconnect to server", check_disconnect);
  CU_add_test(suite, "cleanup", check_cleanup);

  return CU_SUCCESS;
}
