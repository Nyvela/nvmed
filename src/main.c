#include <nyvela/user/syslib.h>
#include <npcid/query.h>
#include <nyvd/query.h>

#include "../include/queue.h"

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

  volatile uint32_t *vs_reg = (volatile uint32_t*)(USER_MMIO_MIN + 0x8);

  char ver[32];

  uint64_t converted = uitoa(*vs_reg, ver, 32);
  
  char ver_msg[] = "NVMED: NVMe Controller Version: ";
  sys_write(1, ver_msg, sizeof(ver_msg) - 1);
  sys_write(1, ver, converted);
  sys_write(1, "\n", 1);

  volatile uint32_t *cc_reg = (volatile uint32_t*)(USER_MMIO_MIN + 0x14);
  *cc_reg &= ~1u;

  volatile uint32_t *csts_reg = (volatile uint32_t*)(USER_MMIO_MIN + 0x1C);

  while (*csts_reg & 1u) {
    __asm__ volatile ("pause");
  }

  char disabled_msg[] = "NVMED: disabled controller.\n";
  sys_write(1, disabled_msg, sizeof(disabled_msg) - 1);
  
  // TODO: 1 page / queue is overkill, but I am too lazy to add dma_alloc for now
  nvme_queue_t* sq = (nvme_queue_t*)sys_dma_alloc_page();

  if (!sq) {
    char msg[] = "NVMED: Failed to allocate submission queue.\n";
    sys_write(1, msg, sizeof(msg) - 1);
  }

  nvme_queue_t* cq = (nvme_queue_t*)sys_dma_alloc_page();
  
  if (!cq) {
    char msg[] = "NVMED: Failed to allocate completition queue.\n";
    sys_write(1, msg, sizeof(msg) - 1);
  }

  char alloc_msg[] = "NVMED: allocated CQ/SQ.\n";
  sys_write(1, alloc_msg, sizeof(alloc_msg) - 1);

  sys_exit(0);
}
