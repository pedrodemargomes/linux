#include <numa.h>
#include <numaif.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGESZ 4096

int main() {
	unsigned long long int numpages = 400000;
	char *ptrN0 = numa_alloc_onnode(numpages*PAGESZ, 0);
	char *ptrN1 = numa_alloc_onnode(numpages*PAGESZ, 1);

	for (unsigned long long i = 0; i < numpages; i++) {
		*((unsigned long long *) &ptrN0[i*PAGESZ]) = i;
		*((unsigned long long *) &ptrN1[i*PAGESZ]) = i;
	}

	if (madvise(ptrN0, numpages*PAGESZ, MADV_MERGEABLE) != 0) {
        	perror("madvise MADV_MERGEABLE failed");
        	return 1;
    	}	

	if (madvise(ptrN1, numpages*PAGESZ, MADV_MERGEABLE) != 0) {
        	perror("madvise MADV_MERGEABLE failed");
        	return 1;
    	}

	printf("Wait...\n");
	getchar();

	unsigned long nodemask = 1UL << 1; // node 1
	mbind(ptrN0, numpages*PAGESZ, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, MPOL_MF_MOVE);	

	printf("Move pages to same numa node\n");
	getchar();
	return 0;
}

