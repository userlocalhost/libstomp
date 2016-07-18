#include "unit.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include <stomp/common.h>
#include <stomp/stomp.h>
#include <stomp/frame.h>
#include <stomp/list.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 61613
#define AUTH_USERID "guest"
#define AUTH_PASSWD "guest"

static session_t *session;

static void check_init(void) {
  session = stomp_init();

  CU_ASSERT(session != NULL);
}

static void check_cleanup(void) {
  pthread_t worker_id;

  CU_ASSERT_FATAL(session != NULL);

  sleep(0.5);

  worker_id = session->tid_receiving;

  stomp_cleanup(session);

  // get thread status
  CU_ASSERT(pthread_kill(worker_id, 0) == ESRCH);
}

static void check_connect() {
  struct list_head *p;

  CU_ASSERT_FATAL(session != NULL);

  CU_ASSERT(stomp_connect(session, SERVER_HOST, SERVER_PORT, AUTH_USERID, AUTH_PASSWD) == RET_SUCCESS);

  frame_t *frame = stomp_recv(session);
  CU_ASSERT(frame != NULL);
}

int test_stomp(CU_pSuite suite) {
  suite = CU_add_suite("STOMP protocol", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "initialization", check_init);
  CU_add_test(suite, "connect to server", check_connect);
  CU_add_test(suite, "cleanup", check_cleanup);

  return CU_SUCCESS;
}
