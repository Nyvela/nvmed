#include <nyvela/user/syslib.h>

void _start(void) {
  for (;;) {
    __asm__ volatile ("pause");
  }

  sys_exit(0);
}
