#include <errno.h>
#include <stdint.h>

void* _sbrk(ptrdiff_t incr) {
  errno = ENOMEM;
  return (void*)-1;
}
