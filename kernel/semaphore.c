#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h" 

struct semtab semtable;

void seminit(void)
{
  initlock(&semtable.lock, "semtable"); // [cite: 47, 49]
  for (int i = 0; i < NSEM; i++) {
    initlock(&semtable.sem[i].lock, "sem"); // [cite: 50, 52]
  }
}

int semalloc(void) {
  acquire(&semtable.lock);

  for (int i = 0; i < NSEM; i++) {
    if (semtable.sem[i].valid == 0) {
      semtable.sem[i].valid = 1;
      release(&semtable.lock);
      return i;
  }
}

  release(&semtable.lock);
  return -1;
} 

void semdealloc(int semid) {
  acquire(&semtable.lock);
  if (semid >= 0 && semid < NSEM && semtable.sem[semid].valid == 1) {
    semtable.sem[semid].valid = 0;
  }  
  release(&semtable.lock);
}
