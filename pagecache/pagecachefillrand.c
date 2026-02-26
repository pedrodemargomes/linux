#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define FILESIZE   (8ULL * 1024 * 1024 * 1024)  // 8 GB
#define ACCESSES   (20000000ULL)                // 20M lookups

static inline uint64_t fast_rand(uint64_t *seed)
{
    *seed = *seed * 6364136223846793005ULL + 1;
    return *seed;
}

int main(int argc, char **argv)
{
    const char *filename = "testfile.bin";
    int fd;
    uint8_t *map;
    size_t pagesize = getpagesize();
    uint64_t npages = FILESIZE / pagesize;
    uint64_t seed = time(NULL);
    volatile uint64_t sum = 0;

    fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ftruncate(fd, FILESIZE) != 0) {
        perror("ftruncate");
        return 1;
    }

    map = mmap(NULL, FILESIZE, PROT_READ,
               MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("File size: %lu pages\n", npages);
    printf("Performing %lu random page lookups...\n", ACCESSES);

    for (uint64_t i = 0; i < ACCESSES; i++) {
        uint64_t page = fast_rand(&seed) % npages;
        uint64_t offset = page * pagesize;

        sum += map[offset];   // force page lookup
    }
    printf("Done. checksum=%lu\n", sum);

    munmap(map, FILESIZE);
    close(fd);
    return 0;
}

