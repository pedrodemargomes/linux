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
	void *hashmap[];
};

struct hamt_root {
	struct hamt_node *h_root;
};

#define DEFINE_HAMT(name) \
	struct hamt_root name = { \
		.h_root = kzalloc(sizeof(struct hamt_node), GFP_KERNEL) \
	}
#define FREE_HAMT_ROOT(name) \
	kfree(name.h_root)

int hamt_insert(struct hamt_root *root, void *value, u32 key);
struct hlist_head *hamt_search(struct hamt_root *root, u32 key);
void hamt_remove(struct hamt_root *root, u32 key);

#endif	/* _LINUX_HAMT_H */
