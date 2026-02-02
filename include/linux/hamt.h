#ifndef	_LINUX_HAMT_H
#define	_LINUX_HAMT_H

#define BUCKET_SIZE 16

struct hamt_leaf {
	u32 key;
	void *bucket;
};

struct hamt_node {
	void *hashmap[BUCKET_SIZE];
	u8 len;
};

struct hamt_root {
	struct hamt_node root;
};

int hamt_insert(struct hamt_root *hamt_root, void *value, u32 key);
void *hamt_search(struct hamt_root *hroot, u32 key);
void *hamt_remove(struct hamt_root *hroot, u32 key);

#endif	/* _LINUX_HAMT_H */
