#include <nyvela/user/syslib.h>
#include <npcid/query.h>
#include <nyvd/query.h>

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

  ipc_device_query_t nvme_query = (ipc_device_query_t){
    .base_class = 0x1, .sub_class = 0x8, .prog_if = 0x2
  };

  sys_ipc_send(npcid_tid, &nvme_query, sizeof(ipc_device_query_t));
  
  pci_device_t dev; 

  while (true) {
    int64_t read = sys_ipc_poll(&dev, sizeof(pci_device_t), NULL);

    if (read < sizeof(pci_device_t)) {
      continue;
    }

    break;
  }

  if (dev.base_class != 0x1 || dev.sub_class != 0x8 || dev.prog_if != 0x2) {
    char buf[] = "Invalid device.\n";
    sys_write(1, buf, sizeof(buf));
  } else {
    char buf[] = "Received a valid NVME device! :)";
    sys_write(1, buf, sizeof(buf));
  }
  
  sys_exit(0);
}
