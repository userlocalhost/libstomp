#include "unit.h"

#include <stomp/common.h>
#include <stomp/frame.h>
#include <stomp/list.h>

#define LARGE_LEN (LD_MAX + 1)

static frame_t *frame = NULL;
char data_large[LARGE_LEN];

static void before_processing() {
  int i;

  for(i=0; i<LARGE_LEN; i++) {
    data_large[i] = 'a';
  }
}

static void check_init() {
  frame = frame_init();
  CU_ASSERT(frame != NULL);
}

static void check_set_command() {
  CU_ASSERT_FATAL(frame != NULL);

  CU_ASSERT(frame_set_cmd(frame, "TEST", 4) == RET_SUCCESS);
  CU_ASSERT(frame->cmd != NULL);
  CU_ASSERT(frame->cmd_len == 4);
}

static void check_set_invalid_header() {
  CU_ASSERT_FATAL(frame != NULL);

  CU_ASSERT(frame_set_header(frame, data_large, LARGE_LEN) == RET_ERROR);
  CU_ASSERT(list_empty(&frame->h_headers));
}
static void check_set_header() {
  CU_ASSERT_FATAL(frame != NULL);
  
  CU_ASSERT(list_empty(&frame->h_headers));
  CU_ASSERT(frame_set_header(frame, "hoge:fuga\n", 10) == RET_SUCCESS);
  CU_ASSERT(! list_empty(&frame->h_headers));
}

static void check_set_body() {
  CU_ASSERT_FATAL(frame != NULL);

  CU_ASSERT(list_empty(&frame->h_body));
  CU_ASSERT(frame_set_body(frame, "message", 7) == RET_SUCCESS);
  CU_ASSERT(! list_empty(&frame->h_body));
}
static void check_set_large_body() {
  struct list_head *curr;
  int before_len, after_len;

  before_len = after_len = 0;
  CU_ASSERT_FATAL(frame != NULL);

  list_for_each(curr, &frame->h_body) {
    before_len++;
  }

  // large body will be separated with two data_entry
  CU_ASSERT(frame_set_body(frame, data_large, LARGE_LEN) == RET_SUCCESS);

  list_for_each(curr, &frame->h_body) {
    after_len++;
  }
  CU_ASSERT((after_len - before_len) == 2);
}
static void check_free() {
  CU_ASSERT_FATAL(frame != NULL);

  frame_free(frame);
}

int test_frame(CU_pSuite suite) {
  suite = CU_add_suite("frame tests", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  before_processing();

  CU_add_test(suite, "test initialize frame_t", check_init);
  CU_add_test(suite, "test set command data to frame_t", check_set_command);
  CU_add_test(suite, "test set invalid header data to frame_t", check_set_invalid_header);
  CU_add_test(suite, "test set header data to frame_t", check_set_header);
  CU_add_test(suite, "test set body to frame_t", check_set_body);
  CU_add_test(suite, "test set large body to frame_t", check_set_large_body);
  CU_add_test(suite, "test set data data to frame_t", check_free);

  return CU_SUCCESS;
}
