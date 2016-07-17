#include <stomp/worker_receiver.h>
#include <stomp/common.h>
#include <stomp/frame.h>
#include <stomp/stomp.h>
#include <stomp/list.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/types.h>

#define RECV_BUFSIZE (512)
#define IS_BL(buf) (buf != NULL && buf[0] == '\0')
#define IS_NL(buf) (buf != NULL && (buf[0] == '\n' || buf[0] == '\r'))

struct receiver_info_t {
  char *prev_data;
  int prev_len;
  frame_t *frame;
  session_t *session;
};

struct stomp_frame_info {
  char *name;
  int len;
};
static struct stomp_frame_info finfo_arr[] = {
  {"SEND",        4},
  {"SUBSCRIBE",   9},
  {"CONNECT",     7},
  {"STOMP",       5},
  {"ACK",         3},
  {"BEGIN",       5},
  {"COMMIT",      6},
  {"ABORT",       5},
  {"NACK",        4},
  {"UNSUBSCRIBE", 11},
  {"DISCONNECT",  10},
  {0},
};

enum frame_status {
  STATUS_BORN         = (1 << 0),
  STATUS_INPUT_NAME   = (1 << 1),
  STATUS_INPUT_HEADER = (1 << 2),
  STATUS_INPUT_BODY   = (1 << 3),
  STATUS_IN_BUCKET    = (1 << 4),
  STATUS_IN_QUEUE     = (1 << 5),
};

static struct receiver_info_t *alloc_rinfo() {
  struct receiver_info_t *rinfo;

  rinfo = (struct receiver_info_t *)malloc(sizeof(struct receiver_info_t));
  if(rinfo != NULL) {
    rinfo->prev_data = NULL;
    rinfo->prev_len = 0;

    rinfo->frame = NULL;
    rinfo->session = NULL;
  }

  return rinfo;
}

static void prepare_frame(struct receiver_info_t *rinfo) {
  assert(rinfo != NULL);

  if(rinfo->frame == NULL) {
    rinfo->frame = frame_init();
  }
}

static int frame_setname(char *data, int len, frame_t *frame) {
  int i, ret = RET_ERROR;
  struct stomp_frame_info *finfo;

  assert(frame != NULL);

  for(i=0; finfo=&finfo_arr[i], finfo!=NULL; i++) {
    if(len < finfo->len) {
      continue;
    } else if(finfo->name == NULL) {
      break;
    }

    if(strncmp(data, finfo->name, finfo->len) == 0) {
      frame_set_cmd(frame, finfo->name, finfo->len);

      CLR(frame);
      SET(frame, STATUS_INPUT_HEADER);

      ret = RET_SUCCESS;
      break;
    }
  }

  return ret;
}

/*
static int is_contentlen(char *input, int len) {
  if(len > 15 && strncmp(input, "content-length:", 15) == 0) {
    return RET_SUCCESS;
  }
  return RET_ERROR;
}
static int get_contentlen(char *input) {
  return atoi(input + 15);
}
*/

static void frame_finish(frame_t *frame, struct receiver_info_t *rinfo) {
  // last processing for current frame_t object
  CLR(frame);
  SET(frame, STATUS_IN_BUCKET);

  pthread_mutex_lock(&rinfo->session->mutex_frames);
  { /* thread safe */
    list_add_tail(&frame->list, &rinfo->session->h_frames);
  }
  pthread_mutex_unlock(&rinfo->session->mutex_frames);
}

static int frame_update(char *line, int len, struct receiver_info_t *rinfo) {
  frame_t *frame = rinfo->frame;
  int ret = 0;

  assert(frame != NULL);

  if(GET(frame, STATUS_BORN)) {
    if(frame_setname(line, len, frame) == RET_ERROR) {
      return -1;
    }
  } else if(GET(frame, STATUS_INPUT_HEADER)) {
    if(IS_NL(line)) {
      CLR(frame);
      SET(frame, STATUS_INPUT_BODY);
    } else if(len > 0) {
      frame_set_header(frame, line, len);
    }
    /*
    if(IS_NL(line)) {
      if(frame->contentlen > 0) {
        CLR(frame);
        SET(frame, STATUS_INPUT_BODY);

        return 0;
      } else if(frame->contentlen == 0) {
        frame_finish(frame);

        return 1;
      } else {
        while(! (IS_NL(line))) {
          line++;
          len -= 1;
        }
      }
    }

    if(len > 0) {
      if(is_contentlen(line, len) == RET_SUCCESS) {
        frame->contentlen = get_contentlen(line);
      }

      frame_set_header(frame, line, len);
    }
    */
  } else if(GET(frame, STATUS_INPUT_BODY)) {
    if(IS_BL(line)) {
      frame_finish(frame, rinfo);

      ret = 1;
    } else if(len > 0) {
      frame_set_body(frame, line, len);
    }
    /*
    if(len > 0) {
      frame->has_contentlen += len;
      frame_set_body(frame, line, len);
    }

    if(frame->contentlen <= frame->has_contentlen) {
      frame_finish(frame);

      ret = 1;
    }
    */
  }

  return ret;
}

static int ssplit(char *start, char *end, int *len, int is_body) {
  char *p;
  int count = 0;
  int ret = RET_SUCCESS;

  assert(start != NULL);
  assert(end != NULL);

  int is_separation = IS_BL(start);
  if(! is_body) {
    is_separation |= IS_NL(start);
  }

  if(is_separation) {
    *len = 0;

    return RET_SUCCESS;
  }

  do {
    count++;
    p = start + count;

    if(p >= end) {
      ret = RET_ERROR;
    }
  } while(! (*p == '\0' || (*p == '\n' && ! is_body) || p >= end || count >= LD_MAX));

  *len = count;

  return ret;
}

static int is_frame_name(const char *name, int len) {
  int i;
  struct stomp_frame_info *finfo;

  for(i=0; finfo=&finfo_arr[i], finfo!=NULL; i++) {
    if(len < finfo->len) {
      continue;
    } else if(finfo->len == 0) {
      break;
    }
    if(strncmp(name, finfo->name, finfo->len) == 0) {
      return RET_SUCCESS;
    }
  }

  return RET_ERROR;
}

static int making_frame(char *recv_data, int len, struct receiver_info_t *rinfo) {
  char *curr, *next, *end, *line;

  curr = recv_data;
  end = (recv_data + len);

  while(curr < end) {
    int line_len, ret;
    int is_body_input = rinfo->frame != NULL && GET(rinfo->frame, STATUS_INPUT_BODY);

    // in header parsing
    ret = ssplit(curr, end, &line_len, is_body_input);
    if(ret == RET_ERROR && is_frame_name(curr, line_len) == RET_ERROR) {
      char *data;
      if(rinfo->prev_data != NULL) {
        data = realloc(rinfo->prev_data, rinfo->prev_len + LD_MAX);
      } else {
        data = malloc(LD_MAX * 2);
      }

      memset(data + rinfo->prev_len, 0, LD_MAX);
      strncpy(data + rinfo->prev_len, curr, line_len);

      rinfo->prev_data = data;
      rinfo->prev_len += line_len;

      break;
    }

    // set next position because following processing may change 'line_len' variable
    if(is_body_input) {
      next = curr + line_len;
    } else {
      next = curr + line_len + 1;
    }

    if(rinfo->prev_data != NULL) {
      line = rinfo->prev_data;

      strncpy(line + rinfo->prev_len, curr, line_len);

      line_len += rinfo->prev_len;
      line[line_len] = '\0';
    } else {
      line = curr;
    }

    if(rinfo->frame != NULL || line_len > 0) {
      prepare_frame(rinfo);

      ret = frame_update(line, line_len, rinfo);
      if(ret > 0) {
        rinfo->frame = NULL;
        if(is_body_input > 0) {
          next++;
        }
      } else if(ret < 0) {
        printf("[ERROR] (maing_frame) failed to parse frame\n");
      }
    }

    if(rinfo->prev_data != NULL) {
      free(rinfo->prev_data);

      rinfo->prev_data = NULL;
      rinfo->prev_len = 0;
    }

    curr = next;
  }

  return RET_SUCCESS;
}

void *receiving_worker(void *arg) {
  session_t *session = (session_t *)arg;
  struct receiver_info_t *rinfo;
  char buf[RECV_BUFSIZE];

  assert(session != NULL);

  rinfo = alloc_rinfo();
  if(rinfo == NULL) {
    return NULL;
  }
  rinfo->session = session;

  int len;
  do {
    memset(buf, 0, RECV_BUFSIZE);

    len = recv(session->conn->sock, buf, sizeof(buf), 0);

    making_frame(buf, len, rinfo);
  } while(len != 0);

  return NULL;
}
