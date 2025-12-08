#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;
  struct proc *p = myproc();

  if(argint(0, &n) < 0)
    return -1;

  addr = p->sz;
  if (n == 0)
    return addr;

  uint64 new_sz = addr + n;
  if(new_sz < p->sz){
    return (uint64)-1;
  }
  p->sz = new_sz;
  /*old eager allocatoin, we don't call growproc right away for lazy allocatoin*/
  /*if(growproc(n) < 0)
    return -1;*/
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_freepmem(void)
{
  uint64 pages = kfreepages_count();
  return pages * PGSIZE;
}

extern struct semtab semtable;

uint64 sys_sem_init(void) {
  uint64 sem_addr;
  int pshared;
  int value;
  
  if (argaddr(0, &sem_addr) < 0 || argint(1, &pshared) < 0 || argint(2, &value) < 0)
    return -1;

  if (value < 0)
    return -1; // POSIX compliance check

  int semid = semalloc();
  if (semid < 0) {
    return -1;
  }

  struct semaphore *s = &semtable.sem[semid];
  acquire(&s->lock); // Lock the specific semaphore entry
  s->count = value;
  release(&s->lock);

  if (copyout(myproc()->pagetable, sem_addr, (char *)&semid, sizeof(semid)) < 0) {
    semdealloc(semid);
    return -1;
  }
  return 0;
}

uint64 sys_sem_destroy(void) {
  uint64 sem_addr;
  int semid;
  if (argaddr(0, &sem_addr) < 0)
    return -1;
  
  if (copyin(myproc()->pagetable, (char *)&semid, sem_addr, sizeof(semid)) < 0) {
    return -1;
  }
  if (semid < 0 || semid >= NSEM || semtable.sem[semid].valid == 0) {
    return -1;
  }
  semdealloc(semid);

  return 0;
}

uint64 sys_sem_wait(void) {
  uint64 sem_addr;
  int semid;
  if (argaddr(0, &sem_addr) < 0)
    return -1;

  if (copyin(myproc()->pagetable, (char *)&semid, sem_addr, sizeof(semid)) < 0) {
    return -1;
  }

  if (semid < 0 || semid >= NSEM || semtable.sem[semid].valid == 0) {
    return -1;
  }
  struct semaphore *s = &semtable.sem[semid];
  acquire(&s->lock);
  while (s->count <= 0) {
    sleep(s, &s->lock); 
  }
  
  s->count--;
  release(&s->lock);
  return 0;
}

uint64 sys_sem_post(void) {
  uint64 sem_addr;
  int semid;
  if (argaddr(0, &sem_addr) < 0)
    return -1;

  if (copyin(myproc()->pagetable, (char *)&semid, sem_addr, sizeof(semid)) < 0) {
    return -1;
  }
  
  if (semid < 0 || semid >= NSEM || semtable.sem[semid].valid == 0) {
    return -1;
  }
  
  struct semaphore *s = &semtable.sem[semid];
  acquire(&s->lock);
  s->count++;
  wakeup(s);
  release(&s->lock);
  
  return 0; // Success
}
