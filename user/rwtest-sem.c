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
  for (int i = 0; i < READER_ITERS; i++) {
    // 1. Acquire mutex to safely update readercount (Entry Section)
    sem_wait(&rw->mutex);

    // Increment reader count
    rw->readercount++;

    // 2. If this is the first reader, acquire the writer lock (wrt)
    //    This blocks any concurrent writers.
    if (rw->readercount == 1) {
      sem_wait(&rw->wrt);
    }

    // Release mutex
    sem_post(&rw->mutex);

    // Critical Section: Reading (multiple readers allowed)
    // int val = rw->value; // Read the value, though just reading without storing is fine
    // printf("Reader %d read value %d\n", getpid(), rw->value);

    // 3. Acquire mutex to safely update readercount (Exit Section)
    sem_wait(&rw->mutex);

    // Decrement reader count
    rw->readercount--;

    // 4. If this is the last reader, release the writer lock (wrt)
    //    This allows a waiting writer to proceed.
    if (rw->readercount == 0) {
      sem_post(&rw->wrt);
    }

    // Release mutex
    sem_post(&rw->mutex);
  }
  exit(0);
}

void
writer(void)
{
  for (int i = 0; i < WRITER_ITERS; i++) {
    // 1. Acquire writer lock (wrt) (Entry Section)
    // This blocks other writers and readers (if a reader has not yet acquired it).
    sem_wait(&rw->wrt);

    // Critical Section: Writing (exclusive access)
    // 2. Increment the shared value by 1
    rw->value++;
    // printf("Writer %d wrote value %d\n", getpid(), rw->value);

    // 3. Release writer lock (wrt) (Exit Section)
    sem_post(&rw->wrt);
  }
  exit(0);
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
