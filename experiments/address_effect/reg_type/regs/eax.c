#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>

//! ---- Variables ---- !//
#define ITS 50000
#define THREAD1_CPU 2
#define THREAD2_CPU 10

char *mem;
char err_msg[] = "Invalid arguments, proper use: [victim setting] [attacker setting]\n\nVictim and Attacker Options:\nw - Writes\nr - Reads\nf - Flushes\n";
char vic_setting;
char att_setting;
u_int32_t OFFSET;

//! ---- ASM Functions ---- !//
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

static inline __attribute__((always_inline)) void writes(char *ptr) { //! CHANGE
    asm volatile (
        "mov %%eax, 0x0000(%0)\n\t"
        "mov %%eax, 0x1000(%0)\n\t"	
        "mov %%eax, 0x2000(%0)\n\t"
        "mov %%eax, 0x3000(%0)\n\t"	
        "mov %%eax, 0x4000(%0)\n\t"
        "mov %%eax, 0x5000(%0)\n\t"	
        "mov %%eax, 0x6000(%0)\n\t"
        "mov %%eax, 0x7000(%0)\n\t"
        :
        : "b" (ptr)
        : 
    );
}

static inline __attribute__((always_inline)) void reads(char *ptr) { //! CHANGE
	asm volatile (
        "mov 0x0000(%0), %%rax\n\t"
        "mov 0x1000(%0), %%rax\n\t" 
        "mov 0x2000(%0), %%rax\n\t" 
        "mov 0x3000(%0), %%rax\n\t" 
        "mov 0x4000(%0), %%rax\n\t" 
        "mov 0x5000(%0), %%rax\n\t" 
        "mov 0x6000(%0), %%rax\n\t" 
        "mov 0x7000(%0), %%rax\n\t"
        :
        : "b" (ptr)
        : 
    );
}

//! ---- Utility Functions ---- !//
void thread_setup(int core) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core, &cpuset);
	pthread_t thread;
	thread = pthread_self();
	int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (ret != 0) {
		fprintf(stderr, "pthread_setaffinity_np() failed\n");
		exit(EXIT_FAILURE);
	}
}

void* get_instruction(char setting) {
	switch (setting) {
		case 'w':
			return writes;
		case 'r':
			return reads;
		default: 
			printf("%s", err_msg);
			exit(EXIT_SUCCESS);
	}
}

//! ---- Threads ---- !//
void *attacker(void *args) {
	// Setup
	thread_setup(THREAD1_CPU); // Put on same physical core
	void (*instruction)(void*) = get_instruction(att_setting); // Testing instruction passed by inline options

	// Timing Instructions on the Same and Different Offsets to the Victim
	uint64_t start, end;
	char *ptr_diff = mem + 0xABCD; 				//! CHANGE
	char *ptr_lower_match = mem + OFFSET + 0xA000; //! CHANGE

	for (int i = 0; i < ITS; i++) {
		start = rdtsc_begin();
		mfence();
		instruction(ptr_diff);
		mfence();
		end = rdtsc_end();
		printf("%lu\n", end - start);
	}

	printf("|<END>|\n"); // Separater for data processing

	for (int i = 0; i < ITS; i++) {
		// Should be slow if theres contention
		start = rdtsc_begin();
		mfence();
		instruction(ptr_lower_match);
		mfence();
		end = rdtsc_end();
		printf("%lu\n", end - start);
	}
}

void *victim(void *args) {
	thread_setup(THREAD2_CPU); // Put on same physical core
	void (*instruction)(void*) = get_instruction(vic_setting);  // Testing instruction passed by inline options

	// Forever Running Instructions in Victim Thread
	char *ptr = mem + OFFSET;						//! CHANGE
	mfence();
	while(1) {
		instruction(ptr);
	}
}

//! ---- Main ---- !//
int main(int argc, char **argv[]) {
	// Parameter Processing //
	if (argc != 4) {
		printf("%s", err_msg);
		return EXIT_SUCCESS;
	} else {
		vic_setting = (char)argv[1][0];
		att_setting = (char)argv[2][0];
		OFFSET = atoi(argv[3]);
	}

	mem = (char *)mmap(NULL, 50 * 4096,				 //! CHANGE?
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0); 
	memset(mem, 0x80, 50 * 4096);

	// Launching Threads
	pthread_t victim_thread, attacker_thread;
	
	pthread_create(&victim_thread, NULL, victim, NULL);
	pthread_create(&attacker_thread, NULL, attacker, NULL);
	
	pthread_join(attacker_thread, NULL);
	pthread_cancel(victim_thread);

	return EXIT_SUCCESS;
}

