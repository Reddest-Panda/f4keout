#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>

/* ---- Data Encoder ---- */ 
#define GET_PTR(mem_ptr, data_byte) (mem_ptr + (((int)data_byte) << 6))

/* ---- Variables ---- */ 
static size_t pagesize = 0;
char *mem;
uint64_t diff;
int vict_setting;
int att_setting;


/* ---- Utility Functions ---- */
static inline __attribute__((always_inline)) void mfence() {
	asm volatile("mfence\n");
}

static inline __attribute__((always_inline)) uint64_t rdtsc_begin() {
        uint64_t a, d;
        asm volatile(
                        "CPUID\n\t"
                        "RDTSCP\n\t"
                        "mov %%rdx, %0\n\t"
                        "mov %%rax, %1\n\t"
			
                        : "=r" (d), "=r" (a)
                        :
                        : "%rax", "%rbx", "%rcx", "%rdx");
        a = (d << 32) | a;
        return a;
}

static inline __attribute__((always_inline)) uint64_t rdtsc_end() {
        uint64_t a, d;
        asm volatile(
                        "RDTSCP\n\t"
                        "mov %%rdx, %0\n\t"
                        "mov %%rax, %1\n\t"
                        "CPUID\n\t"
			
                        : "=r" (d), "=r" (a)
                        :
                        : "%rax", "%rbx", "%rcx", "%rdx");
        a = (d << 32) | a;
        return a;
}


/* ---- Attacker Thread ---- */
void *attacker(void *args) {
	// Thread Setup
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
	
	// Timing + Averaging Reads with Contention in Attacker Thread
	uint64_t start, end;
	int ITS = 1000000;
	unsigned char *ptr;

	// AR //
	if (att_setting) {	
		for (int i = 0; i < ITS; i++) {
			// No Contention
			ptr = GET_PTR(mem + 0x1000, 6);
			mfence();
			start = rdtsc_begin();
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
				: 
				: "r" (ptr)
				: "r15"
				);
			mfence();
			end = rdtsc_end();
			printf("%lu\t", end - start);
			
			// Contention
			ptr = GET_PTR(mem + 0x1000, 32);
			mfence();
			start = rdtsc_begin();
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
				: 
				: "r" (ptr)
				: "r15"
				);
			mfence();
			end = rdtsc_end();
			printf("%lu\n", end - start);
		}
	// AW //
	} else {
		for (int i = 0; i < ITS; i++) {
			// No Contention
			ptr = GET_PTR(mem + 0x1000, 6);
			mfence();
			start = rdtsc_begin();
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
				: 
				: "r" (ptr)
				: 
			);
			mfence();
			end = rdtsc_end();
			printf("%lu\t", end - start);
			
			// Contention
			ptr = GET_PTR(mem + 0x1000, 32);
			mfence();
			start = rdtsc_begin();
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
				: 
				: "r" (ptr)
				: 
			);
			mfence();
			end = rdtsc_end();
			printf("%lu\n", end - start);
		}
	}
}

// void *attacker(void *args) {
// 	// Thread Setup
// 	cpu_set_t cpuset;
// 	CPU_ZERO(&cpuset);
// 	CPU_SET(0, &cpuset);
// 	pthread_t thread;
// 	thread = pthread_self();
// 	int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
// 	if (ret != 0) {
// 		fprintf(stderr, "pthread_setaffinity_np() failed\n");
// 		exit(EXIT_FAILURE);
// 	}
	
// 	// Timing + Averaging Reads with Contention in Attacker Thread
// 	uint64_t start, end;
// 	int ITS = 1000000;
// 	unsigned char *ptr;

// 	// AR //
// 	if (att_setting) {	
// 		for (int i = 0; i < ITS; i++) {
// 			// No Contention
// 			ptr = GET_PTR(mem + 0x1000, 6);
// 			mfence();
// 			start = rdtsc_begin();
// 			asm volatile (
// 				"clflush 0x0000(%0)\n\t"
// 				"clflush 0x1000(%0)\n\t" 
// 				"clflush 0x3000(%0)\n\t" 
// 				"clflush 0x2000(%0)\n\t" 
// 				"clflush 0x4000(%0)\n\t" 
// 				"clflush 0x5000(%0)\n\t" 
// 				"clflush 0x6000(%0)\n\t" 
// 				"clflush 0x7000(%0)\n\t"
// 				: 
// 				: "r" (ptr)
// 				: "r15"
// 				);
// 			mfence();
// 			end = rdtsc_end();
// 			printf("%lu\t", end - start);
			
// 			// Contention
// 			ptr = GET_PTR(mem + 0x1000, 32);
// 			mfence();
// 			start = rdtsc_begin();
// 			asm volatile (
// 				"clflush 0x0000(%0)\n\t"
// 				"clflush 0x1000(%0)\n\t" 
// 				"clflush 0x2000(%0)\n\t" 
// 				"clflush 0x3000(%0)\n\t" 
// 				"clflush 0x4000(%0)\n\t" 
// 				"clflush 0x5000(%0)\n\t" 
// 				"clflush 0x6000(%0)\n\t" 
// 				"clflush 0x7000(%0)\n\t"
// 				: 
// 				: "r" (ptr)
// 				: "r15"
// 				);
// 			mfence();
// 			end = rdtsc_end();
// 			printf("%lu\n", end - start);
// 		}
// 	// AW //
// 	} else {
// 		for (int i = 0; i < ITS; i++) {
// 			// No Contention
// 			ptr = GET_PTR(mem + 0x1000, 6);
// 			mfence();
// 			start = rdtsc_begin();
// 			asm volatile (
// 				"mov %%r14, 0x0000(%0)\n\t"
// 				"mov %%r14, 0x1000(%0)\n\t"	
// 				"mov %%r14, 0x2000(%0)\n\t"
// 				"mov %%r14, 0x3000(%0)\n\t"	
// 				"mov %%r14, 0x4000(%0)\n\t"
// 				"mov %%r14, 0x5000(%0)\n\t"	
// 				"mov %%r14, 0x6000(%0)\n\t"
// 				"mov %%r14, 0x7000(%0)\n\t"
// 				"mov %%r14, 0x0000(%0)\n\t"
// 				"mov %%r14, 0x1000(%0)\n\t"	
// 				"mov %%r14, 0x2000(%0)\n\t"
// 				"mov %%r14, 0x3000(%0)\n\t"	
// 				"mov %%r14, 0x4000(%0)\n\t"
// 				"mov %%r14, 0x5000(%0)\n\t"	
// 				"mov %%r14, 0x6000(%0)\n\t"
// 				"mov %%r14, 0x7000(%0)\n\t"
// 				: 
// 				: "r" (ptr)
// 				: 
// 			);
// 			mfence();
// 			end = rdtsc_end();
// 			printf("%lu\t", end - start);
			
// 			// Contention
// 			ptr = GET_PTR(mem + 0x1000, 32);
// 			mfence();
// 			start = rdtsc_begin();
// 			asm volatile (
// 				"mov %%r14, 0x0000(%0)\n\t"
// 				"mov %%r14, 0x1000(%0)\n\t"	
// 				"mov %%r14, 0x2000(%0)\n\t"
// 				"mov %%r14, 0x3000(%0)\n\t"	
// 				"mov %%r14, 0x4000(%0)\n\t"
// 				"mov %%r14, 0x5000(%0)\n\t"	
// 				"mov %%r14, 0x6000(%0)\n\t"
// 				"mov %%r14, 0x7000(%0)\n\t"
// 				"mov %%r14, 0x0000(%0)\n\t"
// 				"mov %%r14, 0x1000(%0)\n\t"	
// 				"mov %%r14, 0x2000(%0)\n\t"
// 				"mov %%r14, 0x3000(%0)\n\t"	
// 				"mov %%r14, 0x4000(%0)\n\t"
// 				"mov %%r14, 0x5000(%0)\n\t"	
// 				"mov %%r14, 0x6000(%0)\n\t"
// 				"mov %%r14, 0x7000(%0)\n\t"
// 				: 
// 				: "r" (ptr)
// 				: 
// 			);
// 			mfence();
// 			end = rdtsc_end();
// 			printf("%lu\n", end - start);
// 		}
// 	}
// }

/* ---- Victim Thread ---- */
void *victim(void *args) {
	// Thread Setup
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
	
	// Forever Running Writes/Reads in Victim Thread
	unsigned char *ptr = GET_PTR(mem, 32);
	
	if (vict_setting) {
		mfence();
		// VW //
		asm volatile (
			"forever_write:\n\t"
			"mov %%r14, 0x0000(%0)\n\t"
			"mov %%r14, 0x1000(%0)\n\t"	
			"mov %%r14, 0x2000(%0)\n\t"
			"mov %%r14, 0x3000(%0)\n\t"	
			"mov %%r14, 0x4000(%0)\n\t"
			"mov %%r14, 0x5000(%0)\n\t"	
			"mov %%r14, 0x6000(%0)\n\t"
			"mov %%r14, 0x7000(%0)\n\t"
			"jmp forever_write\n\t"	
			:
			: "r" (ptr)
			: 
		);
	} else {
		mfence();
		// VR //
		asm volatile (
			"forever_read:\n\t"
			"mov 0x0000(%0), %%r15\n\t"
			"mov 0x1000(%0), %%r15\n\t" 
			"mov 0x2000(%0), %%r15\n\t" 
			"mov 0x3000(%0), %%r15\n\t" 
			"mov 0x4000(%0), %%r15\n\t" 
			"mov 0x5000(%0), %%r15\n\t" 
			"mov 0x6000(%0), %%r15\n\t" 
			"mov 0x7000(%0), %%r15\n\t"
			"jmp forever_read\n\t"	
			:
			: "r" (ptr)
			: 
		);
	}
}

/* ---- Main ---- */
int main(int argc, char **argv[]) {
	// Parameter Processing //
	char err_msg[] = "Invalid arguments, proper use: [victim setting = w or r] [attacker setting = w or r]\n";

	if (argc != 3) {
		printf("%s", err_msg);
		return EXIT_SUCCESS;
	}
	switch((char)argv[1][0]) {
		case 'w':
			vict_setting = 1;
			break;
		case 'r': 
			vict_setting = 0;
			break;
		default: 
			printf("%s", err_msg);
			return EXIT_SUCCESS;
	}

	switch((char)argv[2][0]) {
		case 'w':
			att_setting = 0;
			break;
		case 'r':
			att_setting = 1;
			break;
		default: 
			printf("%s", err_msg);
			return EXIT_SUCCESS;
	}

	// Allocating Memory Space //
	mem = (unsigned char *)mmap(NULL, 10 * 4096, 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
	memset(mem, 0x80, 10 * 4096);
	
	// Launching Threads
	pthread_t victim_thread, attacker_thread;
	
	pthread_create(&victim_thread, NULL, victim, NULL);
	pthread_create(&attacker_thread, NULL, attacker, NULL);
	
	pthread_join(attacker_thread, NULL);
	pthread_cancel(victim_thread);

	return EXIT_SUCCESS;
}