#ifndef	_LINUX_HAMT_H
#define	_LINUX_HAMT_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/bitmap.h>

#define BUCKET_SIZE 256

struct hamt_entry {
	struct hlist_node node;
	void *value;
};

struct hamt_leaf {
	struct hlist_head bucket;
	u32 key;
};

struct hamt_node {
	u16 len;
	DECLARE_BITMAP(index, BUCKET_SIZE);
	void **hashmap;
};

struct hamt_root {
	struct hamt_node root;
};

int hamt_insert(struct hamt_root *hamt_root, void *value, u32 key);
struct hlist_head *hamt_search(struct hamt_root *hroot, u32 key);
void hamt_remove(struct hamt_root *hroot, u32 key);

#endif	/* _LINUX_HAMT_H */
