#include <nyvela/user/syslib.h>
#include "../include/queue.h"

bool create_admin_sq(nvme_queue_t* sq) {
  sq->addr = (uint64_t)sys_call(SYS_ALLOC_PAGE, 0, 0, 0, 0);

  if (!sq->addr) {
    return false;
  }

  sq->size = 63;

  // TODO: add nvme_write_req(0x28, sq->addr) after adding npcid

  return true;
}

bool create_admin_cq(nvme_queue_t* cq) {
  cq->addr = (uint64_t)sys_call(SYS_ALLOC_PAGE, 0, 0, 0, 0);

  if (!cq->addr) {
    return false;
  }

  cq->size = 63;

  // TODO: add nvme_write_reg(0x30, cq->addr) after adding npcid

  return true;
}
