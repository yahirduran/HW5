#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NULL 0
#define READER_ITERS 50
#define WRITER_ITERS 100

typedef struct {
  int value;        // shared "database" value
  int readercount;  // number of active readers
  sem_t mutex;      // protects readercount
  sem_t wrt;        // writers' lock (and readers' first/last lock)
} rw_t;

rw_t *rw;

void
reader(void)
{
  while (1) {
        // Entry section for readers
        sem_wait(&mutex); // Acquire mutex to protect read_count
        read_count++;
        if (read_count == 1) { // If this is the first reader
            sem_wait(&db); // Acquire db semaphore, blocking writers
        }
        sem_post(&mutex); // Release mutex

        // Critical section: Reading the database
        // ... (access_database() function) ...

        // Exit section for readers
        sem_wait(&mutex); // Acquire mutex to protect read_count
        read_count--;
        if (read_count == 0) { // If this is the last reader
            sem_post(&db); // Release db semaphore, allowing writers
        }
        sem_post(&mutex); // Release mutex

        // Non-critical section
        // ... (non_access_database() function) ...
    }
    return NULL;
}

void
writer(void)
{
  while (1) {
        // Non-critical section
        // ... (non_access_database() function) ...

        // Entry section for writers
        sem_wait(&db); // Acquire db semaphore for exclusive access

        // Critical section: Writing to the database
        // ... (access_database() function) ...

        // Exit section for writers
        sem_post(&db); // Release db semaphore

        // Non-critical section
        // ... (non_access_database() function) ...
    }
    return NULL;
}
int
main(int argc, char *argv[])
{
  if (argc != 3) {
    printf("usage: %s <nreaders> <nwriters>\n", argv[0]);
    exit(0);
  }

  int nreaders = atoi(argv[1]);
  int nwriters = atoi(argv[2]);
  int i;

  rw = (rw_t *) mmap(NULL, sizeof(rw_t),
                     PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  if (rw == (void *)-1 || rw == NULL) {
    printf("rw-sem: mmap failed\n");
    exit(1);
  }

  // initialize shared state
  rw->value = 0;
  rw->readercount = 0;
  sem_init(&rw->mutex, 1, 1);
  sem_init(&rw->wrt,   1, 1);

  // fork readers
  for (i = 0; i < nreaders; i++) {
    if (!fork()) {
      reader();
    }
  }

  // fork writers
  for (i = 0; i < nwriters; i++) {
    if (!fork()) {
      writer();
    }
  }

  // wait for all children
  for (i = 0; i < nreaders + nwriters; i++)
    wait(0);

  // check final value
  int final = rw->value;
  int expected = nwriters * WRITER_ITERS;
  printf("rw-sem: final value = %d, expected = %d\n", final, expected);

  // cleanup
  sem_destroy(&rw->mutex);
  sem_destroy(&rw->wrt);
  munmap(rw, sizeof(rw_t));

  exit(0);
}
