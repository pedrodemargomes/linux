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
	u64 key;
};

struct hamt_node {
	u16 len;
	u16 len_hashmap;
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

int hamt_insert(struct hamt_root *root, void *value, u64 key);
struct hlist_head *hamt_search(struct hamt_root *root, u64 key);
void hamt_remove(struct hamt_root *root, u64 key);
int hamt_get_num_nodes(struct hamt_root *root);
unsigned long hamt_get_size(struct hamt_root *root);

#endif	/* _LINUX_HAMT_H */
