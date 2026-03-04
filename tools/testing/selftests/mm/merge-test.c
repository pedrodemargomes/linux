// SPDX-License-Identifier: GPL-2.0-or-later

#define _GNU_SOURCE
#include "kselftest_harness.h"
#include <linux/prctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <linux/perf_event.h>
#include "vm_util.h"
#include <linux/mman.h>

FIXTURE(merge)
{
	unsigned int page_size;
	char *carveout;
	struct procmap_fd procmap;
};

static char *map_carveout(unsigned int page_size)
{
	return mmap(NULL, 30 * page_size, PROT_NONE,
		    MAP_ANON | MAP_PRIVATE, -1, 0);
}

static pid_t do_fork(struct procmap_fd *procmap)
{
	pid_t pid = fork();

	if (pid == -1)
		return -1;
	if (pid != 0) {
		wait(NULL);
		return pid;
	}

	/* Reopen for child. */
	if (close_procmap(procmap))
		return -1;
	if (open_self_procmap(procmap))
		return -1;

	return 0;
}

FIXTURE_SETUP(merge)
{
	self->page_size = psize();
	/* Carve out PROT_NONE region to map over. */
	self->carveout = map_carveout(self->page_size);
	ASSERT_NE(self->carveout, MAP_FAILED);
	/* Setup PROCMAP_QUERY interface. */
	ASSERT_EQ(open_self_procmap(&self->procmap), 0);
}

FIXTURE_TEARDOWN(merge)
{
	ASSERT_EQ(munmap(self->carveout, 30 * self->page_size), 0);
	/* May fail for parent of forked process. */
	close_procmap(&self->procmap);
	/*
	 * Clear unconditionally, as some tests set this. It is no issue if this
	 * fails (KSM may be disabled for instance).
	 */
	prctl(PR_SET_MEMORY_MERGE, 0, 0, 0, 0);
}

FIXTURE(merge_with_fork)
{
	unsigned int page_size;
	char *carveout;
	struct procmap_fd procmap;
};

FIXTURE_VARIANT(merge_with_fork)
{
	bool forked;
};

FIXTURE_VARIANT_ADD(merge_with_fork, forked)
{
	.forked = true,
};

FIXTURE_VARIANT_ADD(merge_with_fork, unforked)
{
	.forked = false,
};

FIXTURE_SETUP(merge_with_fork)
{
	self->page_size = psize();
	self->carveout = map_carveout(self->page_size);
	ASSERT_NE(self->carveout, MAP_FAILED);
	ASSERT_EQ(open_self_procmap(&self->procmap), 0);
}

FIXTURE_TEARDOWN(merge_with_fork)
{
	ASSERT_EQ(munmap(self->carveout, 30 * self->page_size), 0);
	ASSERT_EQ(close_procmap(&self->procmap), 0);
	/* See above. */
	prctl(PR_SET_MEMORY_MERGE, 0, 0, 0, 0);
}

TEST_F(merge, handle_uprobe_upon_merged_vma)
{
	const size_t attr_sz = sizeof(struct perf_event_attr);
	unsigned int page_size = self->page_size;
	const char *probe_file = "./foo";
	char *carveout = self->carveout;
	struct perf_event_attr attr;
	unsigned long type;
	void *ptr1, *ptr2;
	int fd;

	fd = open(probe_file, O_RDWR|O_CREAT, 0600);
	ASSERT_GE(fd, 0);

	ASSERT_EQ(ftruncate(fd, page_size), 0);
	if (read_sysfs("/sys/bus/event_source/devices/uprobe/type", &type) != 0) {
		SKIP(goto out, "Failed to read uprobe sysfs file, skipping");
	}

	memset(&attr, 0, attr_sz);
	attr.size = attr_sz;
	attr.type = type;
	attr.config1 = (__u64)(long)probe_file;
	attr.config2 = 0x0;

	ASSERT_GE(syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0), 0);

	ptr1 = mmap(&carveout[page_size], 10 * page_size, PROT_EXEC,
		    MAP_PRIVATE | MAP_FIXED, fd, 0);
	ASSERT_NE(ptr1, MAP_FAILED);

	ptr2 = mremap(ptr1, page_size, 2 * page_size,
		      MREMAP_MAYMOVE | MREMAP_FIXED, ptr1 + 5 * page_size);
	ASSERT_NE(ptr2, MAP_FAILED);

	ASSERT_NE(mremap(ptr2, page_size, page_size,
			 MREMAP_MAYMOVE | MREMAP_FIXED, ptr1), MAP_FAILED);

out:
	close(fd);
	remove(probe_file);
}

TEST_HARNESS_MAIN
