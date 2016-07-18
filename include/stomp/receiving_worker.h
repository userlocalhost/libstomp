#ifndef __RECEIVING_WORKER_H__
#define __RECEIVING_WORKER_H__

void *receiving_worker(void *);

enum receiving_worker_status {
  WORKER_STATUS_STOP = (1 << 0),
};

#endif
