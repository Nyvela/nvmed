#include <nyvela/user/syslib.h>
#include <npcid/query.h>
#include <nyvd/query.h>

#include "../include/queue.h"

void nvme_write32(volatile void* base, uint32_t offset, uint32_t value) {
  *(volatile uint32_t*)((uintptr_t)base + offset) = value; 
}

uint32_t nvme_read32(volatile void* base, uint32_t offset) {
  return *(volatile uint32_t*)((uintptr_t)base + offset);
}


void nvme_write64(volatile void* base, uint32_t offset, uint64_t value) {
  *(volatile uint64_t*)((uintptr_t)base + offset) = value; 
}

uint64_t nvme_read64(volatile void* base, uint32_t offset) {
  return *(volatile uint64_t*)((uintptr_t)base + offset);
}

uint64_t nvme_submit(nvme_queue_t *queue, const nvme_sqe_t* cmd) {

}

void _start(void) {
  uint64_t npcid_tid = 0;

  ipc_nyvd_query_t npcid_tid_q = (ipc_nyvd_query_t){
    .func = GET_TID
  };
  
  npcid_tid_q.path[0] = '\0';
  umemcpy(npcid_tid_q.path, "/bin/npcid", sizeof("/bin/npcid"));

  sys_ipc_send(1, &npcid_tid_q, sizeof(ipc_nyvd_query_t));
  
  while (true) {
    int64_t read = sys_ipc_poll(&npcid_tid, sizeof(uint64_t), NULL);

    if (read < sizeof(uint64_t)) {
      continue;
    }

    break;
  }

  ipc_npcid_query_t nvme_query = (ipc_npcid_query_t){
    .func = NPCID_GET_DEVICE,
    .args = { 
      .dev_query = (npcid_dev_query_t){
        .base_class = 0x1, .sub_class = 0x8, .prog_if = 0x2
      }
    }
  };

  sys_ipc_send(npcid_tid, &nvme_query, sizeof(ipc_npcid_query_t));
  
  pci_device_t dev; 

  while (true) {
    int64_t read = sys_ipc_poll(&dev, sizeof(pci_device_t), NULL);

    if (read < sizeof(pci_device_t)) {
      continue;
    }

    break;
  }

  if (dev.base_class != 0x1 || dev.sub_class != 0x8 || dev.prog_if != 0x2) {
    char buf[] = "\nNVMED: Invalid device.\n";
    sys_write(1, buf, sizeof(buf));
    sys_exit(1);
  }

  ipc_npcid_query_t bar_query = (ipc_npcid_query_t){
    .func = NPCID_GET_BAR,
    .args = {
      .bar_query = (npcid_bar_query_t){
        .bus = dev.bus, .dev = dev.device, .func = dev.function, .bar = 0
      }
    }
  };

  sys_ipc_send(npcid_tid, &bar_query, sizeof(ipc_npcid_query_t));

  uint64_t bar_addr;

  while (true) {
    int64_t read = sys_ipc_poll(&bar_addr, sizeof(uint64_t), NULL);

    if (read < sizeof(uint64_t)) {
      continue;
    }

    break;
  }

  if (!bar_addr) {
    char buf[] = "\nNVMED: Invalid BAR address.\n";
    sys_write(1, buf, sizeof(buf) - 1);
    sys_exit(1);
  }

  if (!sys_map_mmio(USER_MMIO_MIN, bar_addr, 0x4)) {
    char buf[] = "\nNVMED: Failed to map BAR.\n";
    sys_write(1, buf, sizeof(buf) - 1);
    sys_exit(1);
  }

  volatile uint32_t *vs_reg = (volatile uint32_t*)(USER_MMIO_MIN + NVME_REG_VS);

  char ver[32];

  uint64_t converted = uitoa(*vs_reg, ver, 32);
  
  char ver_msg[] = "NVMED: NVMe Controller Version: ";
  sys_write(1, ver_msg, sizeof(ver_msg) - 1);
  sys_write(1, ver, converted);
  sys_write(1, "\n", 1);

  volatile uint32_t *cc_reg = (volatile uint32_t*)(USER_MMIO_MIN + NVME_REG_CC);
  *cc_reg &= ~1u;

  volatile uint32_t *csts_reg = (volatile uint32_t*)(USER_MMIO_MIN + NVME_REG_CSTS);

  while (*csts_reg & 1u) {
    __asm__ volatile ("pause");
  }

  char disabled_msg[] = "NVMED: disabled controller.\n";
  sys_write(1, disabled_msg, sizeof(disabled_msg) - 1);
  
  nvme_queue_t admin_q = {0};
  admin_q.cq_phase = 1;

  // TODO: 1 page / queue is overkill, but I am too lazy to add dma_alloc for now
  admin_q.sq = (nvme_sqe_t*)sys_dma_alloc_page(&admin_q.sq_phys);

  if (!admin_q.sq) {
    char msg[] = "NVMED: Failed to allocate submission queue.\n";
    sys_write(1, msg, sizeof(msg) - 1);
    // TODO: cleanup
    sys_exit(1);
  }

  umemset(admin_q.sq, 0, 0x1000);
  
  admin_q.cq = (nvme_cqe_t*)sys_dma_alloc_page(&admin_q.cq_phys);
  
  if (!admin_q.cq) {
    char msg[] = "NVMED: Failed to allocate completition queue.\n";
    sys_write(1, msg, sizeof(msg) - 1);
    // TODO: cleanup
    sys_exit(1);
  }

  umemset(admin_q.cq, 0, 0x1000);

  char alloc_msg[] = "NVMED: allocated CQ/SQ.\n";
  sys_write(1, alloc_msg, sizeof(alloc_msg) - 1);
  
  uint32_t aqa = (NVME_ADMIN_QUEUE_SIZE - 1) | ((NVME_ADMIN_QUEUE_SIZE - 1) << 16);
  nvme_write32((void*)USER_MMIO_MIN, NVME_REG_AQA, aqa);

  char aqa_set_msg[] = "NVMED: Configured AQA.\n";
  sys_write(1, aqa_set_msg, sizeof(aqa_set_msg) - 1);

  nvme_write64((void*)USER_MMIO_MIN, NVME_REG_ASQ, (uint64_t)admin_q.sq_phys);
  nvme_write64((void*)USER_MMIO_MIN, NVME_REG_ACQ, (uint64_t)admin_q.cq_phys);
  
  char asq_acq_msg[] = "NVMED: Configured ASQ/ACQ.\n";
  sys_write(1, asq_acq_msg, sizeof(asq_acq_msg) - 1);

  uint32_t cc = 0;

  cc |= 1u; // EN
  cc |= (6u << 16); // IOSQES
  cc |= (4u << 20); // IOCQES

  nvme_write32((void*)USER_MMIO_MIN, NVME_REG_CC, cc);

  while (!(*csts_reg & 1u)) {
    __asm__ volatile ("pause");
  }

  char cc_msg[] = "NVMED: Configured CC.\n";
  sys_write(1, cc_msg, sizeof(cc_msg) - 1);
  
  nvme_sqe_t cmd = {0};

  cmd.cdw0 = 0x06 | (1u << 16);
  cmd.nsid = 0;
 
  uint64_t cmd_prp1_phys;
  uint64_t cmd_prp1_virt = sys_dma_alloc_page(&cmd_prp1_phys);
  
  if (!cmd_prp1_virt) {
    char msg[] = "NVMED: Failed to allocate buffer for Identify command.\n";
    sys_write(1, msg, sizeof(msg) - 1);
    // TODO: cleanup
    sys_exit(1);
  }

  cmd.prp1 = cmd_prp1_phys;

  cmd.prp2 = 0;

  cmd.cdw10 = 1; // Identify Controller
  
  umemcpy(&admin_q.sq[admin_q.sq_tail], &cmd, sizeof(cmd));

  admin_q.sq_tail = (admin_q.sq_tail + 1) % NVME_ADMIN_QUEUE_SIZE;
  
  nvme_write32((void*)USER_MMIO_MIN, NVME_REG_DBS, admin_q.sq_tail);
  
  char db_msg[] = "NVMED: Rang doorbell...\n";
  sys_write(1, db_msg, sizeof(db_msg) - 1);

  volatile nvme_cqe_t *cqe = &admin_q.cq[admin_q.cq_head];
  
  while ((cqe->status & 1u) != admin_q.cq_phase) {
    __asm__ volatile ("pause");
  }

  uint16_t status = cqe->status >> 1;

  if (status != 0) {
    char msg[] = "NVMED: Identify command failed.\n";
    sys_write(1, msg, sizeof(msg) - 1);
    // TODO: cleanup
    sys_exit(1);
  }

  char resp_received_msg[] = "NVMED: Identify command response received.\n";
  sys_write(1, resp_received_msg, sizeof(resp_received_msg) - 1);
  
  uint8_t* identify = (uint8_t*)cmd_prp1_virt;

  char serial[21] = {0};
  char model[41] = {0};
  char firmware[9] = {0};

  umemcpy(serial, identify + 4, 20);
  umemcpy(model, identify + 24, 40);
  umemcpy(firmware, identify + 64, 8);

  sys_write(1, "NVMED: Serial: ", 15);
  sys_write(1, serial, 20);
  sys_write(1, "\n", 1);

  sys_write(1, "NVMED: Model: ", 14);
  sys_write(1, model, 40);
  sys_write(1, "\n", 1);

  sys_write(1, "NVMED: Firmware: ", 17);
  sys_write(1, firmware, 8);
  sys_write(1, "\n", 1);

  admin_q.sq_head = cqe->sq_head;
  admin_q.cq_head++;

  if (admin_q.cq_head == NVME_ADMIN_QUEUE_SIZE) {
    admin_q.cq_head = 0;
    admin_q.cq_phase ^= 1;
  }

  uint64_t cap = nvme_read64((void*)USER_MMIO_MIN, NVME_REG_CAP);
  uint32_t dstrd = (cap >> 32) & 0xF;
  uint32_t db_stride = 4u << dstrd;

  nvme_write32((void*)USER_MMIO_MIN, NVME_REG_DBS + db_stride, admin_q.cq_head);

  sys_exit(0);
}
