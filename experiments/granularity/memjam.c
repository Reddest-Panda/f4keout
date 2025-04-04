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
#define OFFSET_DIFF		0xBCD
#define OFFSET_SAME     0x123
#define OFFSET_CL       0x11C
#define OFFSET_OFFSET   0xEE3
#define OFFSET_MIDDLE   0xF24
#define ITS 500000
#define THREAD1_CPU 1
#define THREAD2_CPU 9

unsigned char *mem1;
unsigned char *mem2;
char err_msg[] = "Invalid arguments, proper use: [victim setting] [attacker setting]\n\nVictim and Attacker Options:\nw - Writes\nr - Reads\nf - Flushes\n";
char vic_setting;
char att_setting;
char off_setting;

//! ---- ASM Functions ---- !//
static inline __attribute__((always_inline)) void mfence() {
	asm volatile("mfence\n");
}

static inline __attribute__((always_inline)) uint64_t rdtsc_begin() {
        uint64_t a, d;
        asm volatile(	
            "mfence\n\t"
            "CPUID\n\t"
            "RDTSCP\n\t"
            "mov %%rdx, %0\n\t"
            "mov %%rax, %1\n\t"
            "mfence\n"
            : "=r" (d), "=r" (a)
            :
            : "%rax", "%rbx", "%rcx", "%rdx");
        a = (d << 32) | a;
        return a;
}

static inline __attribute__((always_inline)) uint64_t rdtsc_end() {
        uint64_t a, d;
        asm volatile(	
            "mfence\n"
            "RDTSCP\n\t"
            "mov %%rdx, %0\n\t"
            "mov %%rax, %1\n\t"
            "CPUID\n\t"
            "mfence\n"
            : "=r" (d), "=r" (a)
            :
            : "%rax", "%rbx", "%rcx", "%rdx");
        a = (d << 32) | a;
        return a;
}

static inline __attribute__((always_inline)) void writes(char *ptr) {
    asm volatile (
        "mov %%al, 0x0000(%0)\n\t"
        "mov %%al, 0x1000(%0)\n\t"	
        "mov %%al, 0x2000(%0)\n\t"
        "mov %%al, 0x3000(%0)\n\t"	
        "mov %%al, 0x4000(%0)\n\t"
        "mov %%al, 0x5000(%0)\n\t"	
        "mov %%al, 0x6000(%0)\n\t"
        "mov %%al, 0x7000(%0)\n\t"
        :
        : "rbx" (ptr)
        : 
    );
}

static inline __attribute__((always_inline)) void reads(char *ptr) {
    asm volatile (
        "mov 0x0000(%0), %%al\n\t"
        "mov 0x1000(%0), %%al\n\t" 
        "mov 0x2000(%0), %%al\n\t" 
        "mov 0x3000(%0), %%al\n\t" 
        "mov 0x4000(%0), %%al\n\t" 
        "mov 0x5000(%0), %%al\n\t" 
        "mov 0x6000(%0), %%al\n\t" 
        "mov 0x7000(%0), %%al\n\t"
        :
        : "rbx" (ptr)
        : 
    );
}

static inline __attribute__((always_inline)) void single_write(char *ptr) {
    asm volatile (
        "mov %%al, 0x0000(%0)\n\t"
        :
        : "rbx" (ptr)
        : 
    );
}

static inline __attribute__((always_inline)) void single_read(char *ptr) {
    asm volatile (
        "mov 0x0000(%0), %%al\n\t"
        :
        : "rbx" (ptr)
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

void* get_vic_instruction(char setting) {
	switch (setting) {
		case 'w':
			return single_write;
		case 'r':
			return single_read;
		default: 
			printf("%s", err_msg);
			exit(EXIT_SUCCESS);
	}
}

char* get_offset(char setting) {
	switch (setting) {
		case 'd': // Different addresses (max 1 consecutive bit of overlap)
			return mem1 + OFFSET_DIFF;
		case 's': // Same exact address within the same memory
			return mem2 + OFFSET_SAME;
		case 'b': // Same bits [11:0], different address space
			return mem1 + OFFSET_SAME;
		case 'c': // Same bits [11:6], offset has 0 matching bits
			return mem1 + OFFSET_CL;
		case 'o': // Same bits [5:0], cache set has 0 matching bits
			return mem1 + OFFSET_OFFSET;
		case 'm': // Same bits [8:3], cache set/offset each have 3 matching bits
			return mem1 + OFFSET_MIDDLE;
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
	unsigned char *ptr = mem2 + OFFSET_SAME;
    mfence();

	for (int i = 0; i < ITS; i++) {
		start = rdtsc_begin();
		instruction(ptr);
		end = rdtsc_end();
		printf("%lu\n", end - start);
	}
}

void *victim(void *args) {
	thread_setup(THREAD2_CPU); // Put on same physical core
	void (*instruction)(void*) = get_vic_instruction(vic_setting);  // Testing instruction passed by inline options

	// Forever Running Instructions in Victim Thread
	unsigned char *ptr = get_offset(off_setting);	
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
		off_setting = (char)argv[3][0];
	}


	mem1 = (unsigned char *)mmap(NULL, 50 * 4096, 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
	memset(mem1, 0x80, 50 * 4096);

	mem2 = (unsigned char *)mmap(NULL, 50 * 4096, 
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
	memset(mem2, 0x80, 50 * 4096);

	// Launching Threads
	pthread_t victim_thread, attacker_thread;
	
	pthread_create(&victim_thread, NULL, victim, NULL);
	pthread_create(&attacker_thread, NULL, attacker, NULL);
	
	pthread_join(attacker_thread, NULL);
	pthread_cancel(victim_thread);

	return EXIT_SUCCESS;
}
