#include <linux/hamt.h>

#define BUCKET_MASK 0xF
#define BUCKET_SIZE 16
#define BUCKET_SIZE_BITS 4
#define MAX_LEVEL (32/BUCKET_SIZE_BITS)

#define TAG_MASK 0x1
#define IS_LEAF(x) (((uintptr_t)x) & 0x1)
#define SET_LEAF(x) (((uintptr_t)x) | 0x1)
#define GET_POINTER(x) (((uintptr_t)x) & ~TAG_MASK)

// value pointer cannot be NULL
int hamt_insert(struct hamt_root *hamt_root, void *value, u32 key) {
	int bucket_key;
	struct hamt_node *hamtp = &hamt_root->root;
	int level = 0;
	int keymasked = key;

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
	struct hamt_leaf *hamt_leaf = malloc(sizeof(struct hamt_leaf));
	hamt_leaf->key = key;
	hamt_leaf->bucket = value; // TODO: INSERT AT THE END OF THE BUCKET
	hamtp->hashmap[bucket_key] = (void *) SET_LEAF(hamt_leaf);
	return 0;

insert_on_leaf:
	// Old existing leaf
	struct hamt_leaf *hamt_old_leaf = (struct hamt_leaf *) GET_POINTER(hamtp->hashmap[bucket_key]);
	
	if (hamt_old_leaf->key == key) {
		// TODO: INSERT AT THE END OF THE BUCKET
		printf("HASH COLISION\n");
		return 1;
	}

	keymasked = keymasked >> BUCKET_SIZE_BITS;
	while ( (level < (MAX_LEVEL-1)) && ((hamt_old_leaf->key >> (BUCKET_SIZE_BITS*(level+1))) & BUCKET_MASK) == (keymasked & BUCKET_MASK)) {
		struct hamt_node *hamt_new_node = malloc(sizeof(struct hamt_node));
		hamtp->hashmap[bucket_key] = hamt_new_node;
		
		hamtp = hamt_new_node;
		bucket_key = keymasked & BUCKET_MASK;
		keymasked = keymasked >> BUCKET_SIZE_BITS;
		level++;
	}

	if(level == MAX_LEVEL-1) {
		// TODO: INSERT AT THE END OF THE BUCKET
		printf("HASH COLISION\n");
		return 1;
	}


	// New leaf to insert	
	struct hamt_leaf *hamt_new_leaf = malloc(sizeof(struct hamt_leaf));
	hamt_new_leaf->key = key;
	hamt_new_leaf->bucket = value; // TODO: INSERT AT THE END OF THE BUCKET
	
	struct hamt_node *hamt_new_node = malloc(sizeof(struct hamt_node));
	hamtp->hashmap[bucket_key] = hamt_new_node;
	hamt_new_node->hashmap[keymasked & BUCKET_MASK] = (void *) SET_LEAF(hamt_new_leaf);
	hamt_new_node->hashmap[(hamt_old_leaf->key >> (BUCKET_SIZE_BITS*(level+1))) & BUCKET_MASK] = (void *)SET_LEAF(hamt_old_leaf);

}

void *hamt_search(struct hamt_root *hroot, u32 key) {
	int keymasked = key;
	struct hamt_node *hnode = &hroot->root;
	while (1) {
		if(IS_LEAF(hnode->hashmap[keymasked & BUCKET_MASK])) {
			struct hamt_leaf *hleaf = (struct hamt_leaf *) GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK]);
			printf("hleaf->key = %X (%d) hleaf->bucket = %d\n", hleaf->key, hleaf->key, (unsigned int) hleaf->bucket);
			return hleaf->bucket;
		}	
		// Not found
		if (!GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK])) {
			printf("%X %d NOT FOUND SEARCH\n", key, key);
			return NULL;
		}

		hnode = hnode->hashmap[keymasked & BUCKET_MASK];
		keymasked = keymasked >> BUCKET_SIZE_BITS;
	}
}

int isEmpty(struct hamt_node *hnode) {
	for (int i = 0; i < BUCKET_SIZE; i++)
		if (GET_POINTER(hnode->hashmap[i]))
			return 0;
	return 1;
}

void *hamt_remove(struct hamt_root *hroot, u32 key) {
	int keymasked = key;
	struct hamt_node *hnode = &hroot->root;
	struct hamt_node *path[MAX_LEVEL] = {NULL};
	void *bucket;
	int level = 0;

	while (1) {
		path[level++] = hnode;
		if(IS_LEAF(hnode->hashmap[keymasked & BUCKET_MASK])) {
			struct hamt_leaf *hleaf = (struct hamt_leaf *) GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK]);
			printf("removing hleaf->key = %X (%d) hleaf->bucket = %d\n", hleaf->key, hleaf->key, (unsigned int) hleaf->bucket);
			hnode->hashmap[keymasked & BUCKET_MASK] = NULL;
			bucket = hleaf->bucket;
			free(hleaf);
			goto out;
		}
		// Not found
		if (!GET_POINTER(hnode->hashmap[keymasked & BUCKET_MASK])) {
			printf("%X %d NOT FOUND REMOVE\n", key, key);
			return NULL;
		}

		hnode = hnode->hashmap[keymasked & BUCKET_MASK];
		keymasked = keymasked >> BUCKET_SIZE_BITS;
	}
out:
	for (int i = level-1; i >= 1; i--) {
		if (isEmpty(path[i])) {
			free(path[i]);
			path[i-1]->hashmap[ (key >> (BUCKET_SIZE_BITS*(i-1))) & BUCKET_MASK] = NULL;
		}
	}
}