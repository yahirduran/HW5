// Long-term locks for processes
struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};

struct semaphore {
  struct spinlock lock;
  int count;
  int valid;
};

struct semtab {
  struct spinlock lock;
  struct semaphore sem[NSEM];
};

extern struct semtab semtable
