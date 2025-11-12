#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#define MB (1024*1024)
#define GB (1024*MB)
#define KB (1024)

int main() {
	unsigned long size = (unsigned long) 10*GB;
	srand(0); // use current time as seed for random generator
	char *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (m == MAP_FAILED) {
		perror("mmap fail\n");
		return 1;
	}

	madvise(m, size, MADV_MERGEABLE);
	for (int j = 0; j < 2; j++) {
		long int c = 0;
		for (unsigned long i = 0; i < size; i+=4*KB) {
			unsigned long int r = random();
			*((unsigned long int *)(m+i)) = r;
			c++;
		}
		printf("loop %d ended c: %ld\n", j, c);
		sleep(20);
		// Page sharing must be 1588 after full scan
	}

	return 0;
}

