/* See LICENSE of license details. */


#include <unistd.h>
#include <errno.h>
#include "stub.h"

void _exit(int code)
{
  const char message[] = "\nProgram has exited with code:";

  write(STDERR_FILENO, message, sizeof(message) - 1);
  write_hex(STDERR_FILENO, code);
  write(STDERR_FILENO, "\n", 1);

  for (;;);
}

int _getpid(void) {
    return 1; // 裸机环境没有进程概念，默认返回 1
}

int _kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}
