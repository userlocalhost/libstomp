#include "unit.h"

#include <sys/socket.h>

#include <stomp/common.h>
#include <stomp/connection.h>

connection_t *conn = NULL;

#define STOMP_HOST "127.0.0.1"
#define STOMP_PORT (61613)

static int is_socket_valid(int sock) {
  int error = 0;
  socklen_t len = sizeof (error);
  int retval = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

  if (retval != 0 || error != 0) {
    return RET_ERROR;
  }

  return RET_SUCCESS;
}

static void check_init() {
  conn = conn_init(STOMP_HOST, STOMP_PORT);

  CU_ASSERT(conn != NULL);
}

static void check_cleanup() {
  int sock;

  CU_ASSERT_FATAL(conn != NULL);

  sock = conn->sock;
  CU_ASSERT(is_socket_valid(sock) == RET_SUCCESS);

  conn_cleanup(conn);

  CU_ASSERT(is_socket_valid(sock) == RET_ERROR);
}

int test_conn(CU_pSuite suite) {
  suite = CU_add_suite("tcp connection wrapper test", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "initialization", check_init);
  CU_add_test(suite, "cleanup", check_cleanup);

  return CU_SUCCESS;
}
