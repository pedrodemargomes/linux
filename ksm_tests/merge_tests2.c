#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGESZ 4096

int main() {
	size_t size = 8ULL * 1024*1024*1024;
	unsigned long long int numpages = size/PAGESZ;
	char *pages = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	printf("pages: %p numpages: %llu size: %llu sizeof(size_t): %d\n", pages, numpages, size, sizeof(size_t));

	// Generate #numpages pages with different contents
	for (unsigned long long i = 0; i < numpages/2; i++) {
		*((unsigned long long *) &pages[i*PAGESZ]) = i;
		*((unsigned long long *) &pages[(numpages-i-1)*PAGESZ]) = i;
	}

	if (madvise(pages, size, MADV_MERGEABLE) != 0) {
        	perror("madvise MADV_MERGEABLE failed");
        	return 1;
    	}
	printf("Wait...\n");
	getchar();
	return 0;
}
