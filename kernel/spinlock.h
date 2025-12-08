#include "param.h"
// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

struct semaphore {
  struct spinlock lock;  
  int count;             
  int valid;             // if the entry is in use, 1
};

// semaphore table
struct semtab {
  struct spinlock lock;        // lock protecting the table
  struct semaphore sem[NSEM];  // fixed-size array of semaphores
};
