#include <lib.h>
#define compact_memory	_compact_memory
#include <unistd.h>


PUBLIC int compact_memory()
{
  message m;
  if (_syscall(MM, COMPACTMEM, &m) < 0) return(-1);
  return(0);
}

