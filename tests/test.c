#include "unit/unit.h"

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define ADD_TESTS(statement) ret = statement(suite); if(ret != CUE_SUCCESS) return ret;

int main(int argc, char **argv) {
  CU_pSuite suite;
  int ret, failed_num;
  pid_t pid;

  signal(SIGPIPE, SIG_IGN);

  CU_initialize_registry();

  ADD_TESTS(test_frame);

  CU_basic_run_tests();

  failed_num = CU_get_number_of_tests_failed();

  CU_cleanup_registry();

  return failed_num;
}
