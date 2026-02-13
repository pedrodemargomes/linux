#include <linux/hamt.h>
#include <linux/slab.h>

#define BUCKET_MASK 0xFF
#define BUCKET_SIZE_BITS 8
#define MAX_LEVEL (32/BUCKET_SIZE_BITS)

#define TAG_MASK 0x1
#define IS_LEAF(x) (((unsigned long)x) & 0x1)
#define SET_LEAF(x) (((unsigned long)x) | 0x1)
#define GET_POINTER(x) (((unsigned long)x) & ~TAG_MASK)

// value pointer cannot be NULL
int hamt_insert(struct hamt_root *hamt_root, void *value, u32 key) {
	int bucket_key;
	struct hamt_node *hamtp = &hamt_root->root;
	int level = 0;
	int keymasked = key;

	struct hamt_entry *entry = kzalloc(sizeof(struct hamt_entry), GFP_KERNEL);
	entry->value = value;

	for (;;) {
		bucket_key = keymasked & BUCKET_MASK;
		if(IS_LEAF(hamtp->hashmap[bucket_key]))
			goto insert_on_leaf;

		if (!GET_POINTER(hamtp->hashmap[bucket_key]))
			goto insert_on_empty;

		hamtp = (struct hamt_node *) GET_POINTER(hamtp->hashmap[bucket_key]);
		keymasked = keymasked >> BUCKET_SIZE_BITS;
		level++;
	}

insert_on_empty:
	struct hamt_leaf *hamt_leaf = kzalloc(sizeof(struct hamt_leaf), GFP_KERNEL);
	hamt_leaf->key = key;
	INIT_HLIST_HEAD(&hamt_leaf->bucket);
	hlist_add_head(&entry->node, &hamt_leaf->bucket);
	hamtp->hashmap[bucket_key] = (void *) SET_LEAF(hamt_leaf);
	hamtp->len++;
	return 0;

insert_on_leaf:
	// Old existing leaf
	struct hamt_leaf *hamt_old_leaf = (struct hamt_leaf *) GET_POINTER(hamtp->hashmap[bucket_key]);

	if (hamt_old_leaf->key == key) {
		hlist_add_head(&entry->node, &hamt_old_leaf->bucket);
		// printk("HASH COLISION\n");
		return 0;
	}

	keymasked = keymasked >> BUCKET_SIZE_BITS;
	while ( (level < (MAX_LEVEL-1)) && ((hamt_old_leaf->key >> (BUCKET_SIZE_BITS*(level+1))) & BUCKET_MASK) == (keymasked & BUCKET_MASK)) {
		struct hamt_node *hamt_new_node = kzalloc(sizeof(struct hamt_node), GFP_KERNEL);
		hamt_new_node->len++;
		hamtp->hashmap[bucket_key] = hamt_new_node;

		hamtp = hamt_new_node;
		bucket_key = keymasked & BUCKET_MASK;
		keymasked = keymasked >> BUCKET_SIZE_BITS;
		level++;
	}

	// New leaf to insert
	struct hamt_leaf *hamt_new_leaf = kzalloc(sizeof(struct hamt_leaf), GFP_KERNEL);
	hamt_new_leaf->key = key;
	INIT_HLIST_HEAD(&hamt_new_leaf->bucket);
	hlist_add_head(&entry->node, &hamt_new_leaf->bucket);

	struct hamt_node *hamt_new_node = kzalloc(sizeof(struct hamt_node), GFP_KERNEL);
	hamtp->hashmap[bucket_key] = hamt_new_node;
	hamt_new_node->hashmap[keymasked & BUCKET_MASK] = (void *) SET_LEAF(hamt_new_leaf);
	hamt_new_node->hashmap[(hamt_old_leaf->key >> (BUCKET_SIZE_BITS*(level+1))) & BUCKET_MASK] = (void *)SET_LEAF(hamt_old_leaf);
	hamt_new_node->len+=2;

	return 0;
}

struct hlist_head *hamt_search(struct hamt_root *hroot, u32 key) {
	int keymasked = key;
	struct hamt_node *hnode = &hroot->root;
	while (1) {
		if(IS_LEAF(hnode->hashmap[keymasked & BUCKET_MASK])) {
			struct hamt_leaf *hleaf = (struct hamt_leaf *) GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK]);
			// printk("hleaf->key = %X (%d) hleaf->bucket = %p\n", hleaf->key, hleaf->key, hleaf->bucket);
			return &hleaf->bucket;
		}
		// Not found
		if (!GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK])) {
			// printk("%X %d NOT FOUND SEARCH\n", key, key);
			return NULL;
		}

		hnode = hnode->hashmap[keymasked & BUCKET_MASK];
		keymasked = keymasked >> BUCKET_SIZE_BITS;
	}
}

static int isEmpty(struct hamt_node *hnode) {

	for (int i = 0; i < BUCKET_SIZE; i++)
		if (GET_POINTER(hnode->hashmap[i])) {
			if (!hnode->len)
				panic("ERRO isEmpty\n");

			return 0;
		}

	if (hnode->len)
		panic("ERRO isEmpty\n");
	return 1;

	//return !hnode->len;
}

void hamt_remove(struct hamt_root *hroot, u32 key) {
	int keymasked = key;
	struct hamt_node *hnode = &hroot->root;
	struct hamt_node *path[MAX_LEVEL] = {NULL};
	int level = 0;

	while (1) {
		path[level++] = hnode;
		if(IS_LEAF(hnode->hashmap[keymasked & BUCKET_MASK])) {
			struct hamt_leaf *hleaf = (struct hamt_leaf *) GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK]);
			// printk("removing hleaf->key = %X (%d) hleaf->bucket = %p\n", hleaf->key, hleaf->key, hleaf->bucket);
			hnode->hashmap[keymasked & BUCKET_MASK] = NULL;
			hnode->len--;
			kfree(hleaf);
			goto out;
		}
		// Not found
		if (!GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK])) {
			// printk("%X %d NOT FOUND REMOVE\n", key, key);
			return ;
		}

		hnode = hnode->hashmap[keymasked & BUCKET_MASK];
		keymasked = keymasked >> BUCKET_SIZE_BITS;
	}
out:
	for (int i = level-1; i >= 1; i--) {
		if (isEmpty(path[i])) {
			kfree(path[i]);
			path[i-1]->hashmap[ (key >> (BUCKET_SIZE_BITS*(i-1))) & BUCKET_MASK] = NULL;
			path[i-1]->len--;
		}
	}
}
