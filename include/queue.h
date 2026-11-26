#ifndef NVMED_QUEUE_H
#define NVMED_QUEUE_H

#include <stdint.h>

#define NVME_CQE_PHASE_BIT 0x0001
#define NVME_ADMIN_QUEUE_SIZE 64

#define NVME_REG_CAP   0x00
#define NVME_REG_VS    0x08
#define NVME_REG_CC    0x14
#define NVME_REG_CSTS  0x1C
#define NVME_REG_AQA   0x24
#define NVME_REG_ASQ   0x28
#define NVME_REG_ACQ   0x30
#define NVME_REG_DBS 0x1000

typedef struct nvme_queue {
  uint64_t addr, size;
} nvme_queue_t;

typedef struct nvme_sqe {
  uint32_t cdw0;
  uint32_t nsid;
  uint64_t _reserved;
  uint64_t metadata_pointer;
  uint64_t prp1, prp2;
  uint32_t cdw10, cdw11, cdw12, cdw13, cdw14, cdw15;
} nvme_sqe_t;

typedef struct nvme_cqe {
  uint32_t command_specific;
  uint32_t _reserved;
  uint16_t sq_head;
  uint16_t sqid;
  uint16_t cmdid;
  uint16_t status;
} nvme_cqe_t;

_Static_assert(sizeof(nvme_sqe_t) == 64, "nvme_sqe_t must be 64 bytes");
_Static_assert(sizeof(nvme_cqe_t) == 16, "nvme_cqe_t must be 16 bytes");

void cqe_set_phase(nvme_cqe_t* cqe, uint8_t phase);

uint32_t encode_cdw0(uint8_t opcode, uint8_t fused, uint8_t data_pointer_type, uint16_t cid);

bool create_admin_sq(nvme_queue_t *sq);
bool create_admin_cq(nvme_queue_t *cq);

#endif // NVMED_QUEUE_H
