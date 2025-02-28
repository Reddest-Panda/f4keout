#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <x86intrin.h>

//! ---- Variables ---- !//
#define ITS 10000
#define THREAD1_CPU 2

char *mem;
char att_setting;

unsigned int base_MHz;
struct freq_sample_t {
	uint64_t aperf;
	uint64_t mperf;
};

#define TIME_BETWEEN_MEASUREMENTS 5000000L // 5 millisecond

#define MSR_IA32_MPERF 0x000000e7
#define MSR_IA32_APERF 0x000000e8

//! FREQUENCY MONITORING STUFF !//
int set_frequency_units() {
	uint32_t val;
	asm volatile(
		"mov $0x16, %%eax \n\t"
		"cpuid \n\t"
		"mov %%eax, %0 \n\t"
		: "=r" (val)
		:
		: "%rax", "%rbx", "%rcx", "%rdx");
	base_MHz = val & 0xFFFF;
	// printf("base frequency: %u\n", base_MHz);
	return 0;
}

uint64_t my_rdmsr_on_cpu(int core_ID, int reg) {
	uint64_t data;
	static int fd = -1;
	static int last_core_ID;

	if (fd < 0 || last_core_ID != core_ID) {
		char msr_filename[BUFSIZ];
		if (fd >= 0)
			close(fd);
		sprintf(msr_filename, "/dev/cpu/%d/msr", core_ID);
		fd = open(msr_filename, O_RDONLY);
		if (fd < 0) {
			if (errno == ENXIO) {
				fprintf(stderr, "rdmsr: No CPU %d\n", core_ID);
				exit(2);
			} else if (errno == EIO) {
				fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n", core_ID);
				exit(3);
			} else {
				perror("rdmsr: open");
				exit(127);
			}
		}
		last_core_ID = core_ID;
	}

	if (pread(fd, &data, sizeof data, reg) != sizeof data) {
		if (errno == EIO) {
			fprintf(stderr, "rdmsr: CPU %d cannot read MSR 0x%08" PRIx32 "\n", core_ID, reg);
			exit(4);
		} else {
			perror("rdmsr: pread");
			exit(127);
		}
	}

	return data;
}

struct freq_sample_t frequency_msr_raw(int core_ID) {
	struct freq_sample_t freq_sample;
	freq_sample.aperf = my_rdmsr_on_cpu(core_ID, MSR_IA32_APERF);
	freq_sample.mperf = my_rdmsr_on_cpu(core_ID, MSR_IA32_MPERF);
	return freq_sample;
}

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
			"mfence\n\t"
			: "=r" (d), "=r" (a)
			:
			: "%rax", "%rbx", "%rcx", "%rdx");
        a = (d << 32) | a;
        return a;
}

static inline __attribute__((always_inline)) uint64_t rdtsc_end() {
        uint64_t a, d;
        asm volatile(
			"mfence\n\t"
			"RDTSCP\n\t"
			"mov %%rdx, %0\n\t"
			"mov %%rax, %1\n\t"
			"CPUID\n\t"
			"mfence\n\t"
			: "=r" (d), "=r" (a)
			:
			: "%rax", "%rbx", "%rcx", "%rdx");
        a = (d << 32) | a;
        return a;
}


static inline __attribute__((always_inline)) void writes(void *ptr) {
    asm volatile (
        "mov %%r14, 0x0000(%0)\n\t"
        "mov %%r14, 0x1000(%0)\n\t"	
        "mov %%r14, 0x2000(%0)\n\t"
        "mov %%r14, 0x3000(%0)\n\t"	
        "mov %%r14, 0x4000(%0)\n\t"
        "mov %%r14, 0x5000(%0)\n\t"	
        "mov %%r14, 0x6000(%0)\n\t"
        "mov %%r14, 0x7000(%0)\n\t"
        :
        : "b" (ptr)
        : 
    );
}

static inline __attribute__((always_inline)) void reads(void *ptr) {
    asm volatile (
        "mov 0x0000(%0), %%r15\n\t"
        "mov 0x1000(%0), %%r15\n\t" 
        "mov 0x2000(%0), %%r15\n\t" 
        "mov 0x3000(%0), %%r15\n\t" 
        "mov 0x4000(%0), %%r15\n\t" 
        "mov 0x5000(%0), %%r15\n\t" 
        "mov 0x6000(%0), %%r15\n\t" 
        "mov 0x7000(%0), %%r15\n\t"
        :
        : "b" (ptr)
        : 
    );
}

static inline __attribute__((always_inline)) void mixed(void *ptr) {
    asm volatile (
        "mov 0x0000(%0), %%r15\n\t" 
        "mov 0x2000(%0), %%r15\n\t" 
        "mov 0x4000(%0), %%r15\n\t"
        "mov 0x6000(%0), %%r15\n\t"
        "mov %%r14, 0x1000(%0)\n\t"
        "mov %%r14, 0x3000(%0)\n\t"	
        "mov %%r14, 0x5000(%0)\n\t"	
        "mov %%r14, 0x7000(%0)\n\t"
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
		case 'm':
			return mixed;
		default:
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
	unsigned char *ptr = mem + 0xABCD; // Different address as victim
	set_frequency_units();
	struct freq_sample_t freq_sample, prev_freq_sample = frequency_msr_raw(THREAD1_CPU);

	for (int i = 0; i < ITS; i++) {
		// nanosleep((const struct timespec[]){{0, TIME_BETWEEN_MEASUREMENTS}}, NULL);
		// Get cycle data
		start = rdtsc_begin();
		instruction(ptr);
		end = rdtsc_end();

		// Get freq data
		freq_sample = frequency_msr_raw(THREAD1_CPU);
		uint64_t aperf_delta = freq_sample.aperf - prev_freq_sample.aperf;
		uint64_t mperf_delta = freq_sample.mperf - prev_freq_sample.mperf;
		uint32_t mhz = (base_MHz * aperf_delta) / mperf_delta;
		prev_freq_sample = freq_sample;

		// Print results
		printf("%lu\t%u\n", (end - start), mhz);
		sched_yield();
	}
}

//! ---- Main ---- !//
int main(int argc, char **argv[]) {
	// Parameter Processing //
	att_setting = (char)argv[1][0];

	mem = (unsigned char *)mmap(NULL, 50 * 4096, 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
	memset(mem, 0x80, 50 * 4096);

	// Launching Threads
	pthread_t attacker_thread;
	pthread_create(&attacker_thread, NULL, attacker, NULL);
	
	pthread_join(attacker_thread, NULL);
	return EXIT_SUCCESS;
}

