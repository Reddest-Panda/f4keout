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
int vict_its;
int att_its;
pthread_barrier_t att_vic_sync;

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

static inline __attribute__((always_inline)) void write_instruct(char *ptr) {
    asm volatile (
        "mov %%r14, 0x0000(%0)\n\t"
        "mov %%r14, 0x1000(%0)\n\t"	
        "mov %%r14, 0x2000(%0)\n\t"
        "mov %%r14, 0x3000(%0)\n\t"
        :
        : "r" (ptr)
        : "r14"
    );
}

static inline __attribute__((always_inline)) void read_instruct(char *ptr) {
    asm volatile (
        "mov 0x0000(%0), %%r15\n\t"
        "mov 0x1000(%0), %%r15\n\t" 
        "mov 0x2000(%0), %%r15\n\t" 
        "mov 0x3000(%0), %%r15\n\t"
        : 
        : "r" (ptr)
        : "r15"
    );
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
    long int con, no_con;
	int ITS = 1000000;
	unsigned char *ptr;

    for (int i = 0; i < ITS; i ++) {
        // No Contention
        ptr = GET_PTR(mem + 0x1000, 6);

        pthread_barrier_wait(&att_vic_sync);
        mfence();
        start = rdtsc_begin();

        for (int j = 0; j < att_its; j++) {
            read_instruct(ptr);
        }

        mfence();
        end = rdtsc_end();
        no_con = end - start;

        // Contention
        ptr = GET_PTR(mem + 0x1000, 32);
        
        pthread_barrier_wait(&att_vic_sync);
        mfence();
        start = rdtsc_begin();
        
        for (int j = 0; j < att_its; j++) {
            read_instruct(ptr);	
        }
        
        mfence();
        end = rdtsc_end();
        con = end - start;

        printf("%ld\n", con - no_con);
    }
}

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
	
    // Encode in cache set 32
	unsigned char *ptr = GET_PTR(mem, 32);
    
    while (1) {
        // No Contention
        mfence();
        pthread_barrier_wait(&att_vic_sync);
        for (int i = 0; i < vict_its; i++) {
            write_instruct(ptr);
        }
        
        // Contention
        mfence();
        pthread_barrier_wait(&att_vic_sync);
        for (int i = 0; i < vict_its; i++) {
            write_instruct(ptr);	
        }
    }
}

/* ---- Main ---- */
int main(int argc, char **argv[]) {
	// Parameter Processing - Note each iteration is 4 instructions //
	char err_msg[] = "Invalid arguments, proper use: [victim setting = int num_its] [attacker setting = int num_its]\n";

	if (argc != 3) {
		printf("%s", err_msg);
		return EXIT_SUCCESS;
	}

    vict_its = atoi(argv[1]);
    att_its = atoi(argv[2]);
    printf("%d\t%d\n", vict_its, att_its);


	// Allocating Memory Space //
	mem = (unsigned char *)mmap(NULL, 10 * 4096, 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
	memset(mem, 0x80, 10 * 4096);
	
	// Launching Threads
	pthread_t victim_thread, attacker_thread;
    pthread_barrier_init(&att_vic_sync, NULL, 2);
	
	pthread_create(&victim_thread, NULL, victim, NULL);
	pthread_create(&attacker_thread, NULL, attacker, NULL);
	
	pthread_join(attacker_thread, NULL);
	pthread_cancel(victim_thread);

	return EXIT_SUCCESS;
}