#include <linux/hamt.h>
#include <linux/slab.h>
#include <linux/string.h>

#define BUCKET_MASK 0xFF
#define BUCKET_SIZE_BITS 8
#define MAX_LEVEL (64/BUCKET_SIZE_BITS)

#define TAG_MASK 0x1
#define IS_LEAF(x) (((unsigned long)x) & 0x1)
#define SET_LEAF(x) (((unsigned long)x) | 0x1)
#define GET_POINTER(x) (((unsigned long)x) & ~TAG_MASK)

static void **get_node(struct hamt_node *hamtp, u64 key) {
	if (!test_bit(key, hamtp->index))
		return NULL;
	return (void **) &hamtp->hashmap[bitmap_weight(hamtp->index, key)];
}

static void set_node(struct hamt_node **hamtpp, u64 key, void *node) {
	struct hamt_node *hamtp = *hamtpp;
	if (test_bit(key, hamtp->index)) {
		hamtp->hashmap[bitmap_weight(hamtp->index, key)] = node;
		//printk("set_node hamtp: %px len: %d already inserted idx: %d\n", hamtp, hamtp->len, bitmap_weight(hamtp->index, key));
	} else {
		*hamtpp = krealloc(*hamtpp, sizeof(struct hamt_node) + (hamtp->len+1)*sizeof(void *), GFP_KERNEL);
		hamtp = *hamtpp;
		u64 k = bitmap_weight(hamtp->index, key);	
		//for (int i = hamtp->len; i > k; i--)
		//	hamtp->hashmap[i] = hamtp->hashmap[i-1]; 
		memmove(&hamtp->hashmap[k+1], &hamtp->hashmap[k], (hamtp->len-k) * sizeof(void *));

		set_bit(key, hamtp->index);
		hamtp->len++;
		hamtp->hashmap[k] = node;
		
		//printk("set_node hamtp: %p len: %d\n", hamtp, hamtp->len);
	}
}

static int remove_node(struct hamt_node **hamtpp, u64 key) {
	struct hamt_node *hamtp = *hamtpp;
	if (!test_bit(key, hamtp->index)) {
		//printk("remove_node: key not present in node\n");
		return 1;
	}

	u64 k = bitmap_weight(hamtp->index, key);
	//for (int i = k; i < hamtp->len-1; i++)
	//	hamtp->hashmap[i] = hamtp->hashmap[i+1]; 
	memmove(&hamtp->hashmap[k], &hamtp->hashmap[k+1], (hamtp->len-k-1) * sizeof(void *));

	// Realloc shrink hashmap
	*hamtpp = krealloc(*hamtpp, sizeof(struct hamt_node) + (hamtp->len-1)*sizeof(void *), GFP_KERNEL);
	hamtp = *hamtpp;

	clear_bit(key, hamtp->index);
	hamtp->len--;

	// printk("remove_node hamtp: %p len: %d already inserted idx: %d\n", hamtp, hamtp->len, bitmap_weight(hamtp->index, key));
	return 0;
}

struct hamt_node *stack[1000];
int hamt_get_num_nodes(struct hamt_root *root) {
	int len = 0;
	int top = 0;
	struct hamt_node **hamtp = &root->h_root;
	stack[top++] = *hamtp;
	while(top > 0) {
		struct hamt_node *n = stack[--top];
		len++;
		if (!IS_LEAF(n)) {
			for (int i = 0; i < n->len; i++) {
				stack[top++] = n->hashmap[i];
			}
		}
	}
	return len;
}
EXPORT_SYMBOL(hamt_get_num_nodes);

unsigned long hamt_get_size(struct hamt_root *root) {
	unsigned long size = 0;
	int top = 0;
	struct hamt_node **hamtp = &root->h_root;
	stack[top++] = *hamtp;
	while(top > 0) {
		struct hamt_node *n = stack[--top];
		size += sizeof(*n);
		if (!IS_LEAF(n)) {
			for (int i = 0; i < n->len; i++) {
				stack[top++] = n->hashmap[i];
			}
		}
	}
	return size;
}
EXPORT_SYMBOL(hamt_get_size);



// value pointer cannot be NULL
int hamt_insert(struct hamt_root *root, void *value, u64 key) {
	int bucket_key;
	void **n;
	struct hamt_node **hamtp = &root->h_root;
	int level = 0;
	int keymasked = key;

	struct hamt_entry *entry = kzalloc(sizeof(struct hamt_entry), GFP_KERNEL);
	entry->value = value;

	for (;;) {
		bucket_key = keymasked & BUCKET_MASK;
		n = get_node(*hamtp, bucket_key); 
		
		if ( !n || !GET_POINTER(*n))
			goto insert_on_empty;

		if(IS_LEAF(*n))
			goto insert_on_leaf;

		hamtp = (struct hamt_node **) n;
		keymasked = keymasked >> BUCKET_SIZE_BITS;
		level++;
	}

insert_on_empty:
	struct hamt_leaf *hamt_leaf = kzalloc(sizeof(struct hamt_leaf), GFP_KERNEL);
	hamt_leaf->key = key;
	INIT_HLIST_HEAD(&hamt_leaf->bucket);
	hlist_add_head(&entry->node, &hamt_leaf->bucket);
	set_node(hamtp, bucket_key, (void *) SET_LEAF(hamt_leaf));
	return 0;

insert_on_leaf:
	// Old existing leaf
	struct hamt_leaf *hamt_old_leaf = (struct hamt_leaf *) GET_POINTER(*n);

	if (hamt_old_leaf->key == key) {
		hlist_add_head(&entry->node, &hamt_old_leaf->bucket);
		return 0;
	}

	keymasked = keymasked >> BUCKET_SIZE_BITS;
	while ( (level < (MAX_LEVEL-1)) && ((hamt_old_leaf->key >> (BUCKET_SIZE_BITS*(level+1))) & BUCKET_MASK) == (keymasked & BUCKET_MASK)) {
		struct hamt_node *hamt_new_node = kzalloc(sizeof(struct hamt_node), GFP_KERNEL);
		set_node(hamtp, bucket_key, hamt_new_node);
		hamtp = (struct hamt_node **) get_node(*hamtp, bucket_key); 
		
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
	set_node(&hamt_new_node, keymasked & BUCKET_MASK, (void *) SET_LEAF(hamt_new_leaf));
	set_node(&hamt_new_node, (hamt_old_leaf->key >> (BUCKET_SIZE_BITS*(level+1))) & BUCKET_MASK, (void *) SET_LEAF(hamt_old_leaf));
	set_node(hamtp, bucket_key, hamt_new_node);

	return 0;
}
EXPORT_SYMBOL(hamt_insert);

struct hlist_head *hamt_search(struct hamt_root *root, u64 key) {
	int keymasked = key;
	struct hamt_node *hnode = root->h_root;
	while (1) {
		void **n = get_node(hnode, keymasked & BUCKET_MASK);
		// Not found
		if ( !n || !GET_POINTER(*n)) {
			// printk("%X %d NOT FOUND SEARCH\n", key, key);
			return NULL;
		}

		if (IS_LEAF(*n)) {
			struct hamt_leaf *hleaf = (struct hamt_leaf *) GET_POINTER(*n);
			// printk("hleaf->key = %X (%d) hleaf->bucket = %p\n", hleaf->key, hleaf->key, hleaf->bucket);
			return &hleaf->bucket;
		}

		hnode = (struct hamt_node *) *n;	
		keymasked = keymasked >> BUCKET_SIZE_BITS;
	}
}
EXPORT_SYMBOL(hamt_search);

static int isEmpty(struct hamt_node *hnode) {
	return !hnode->len;
}

void hamt_remove(struct hamt_root *root, u64 key) {
	int keymasked = key;
	struct hamt_node **hnode = &root->h_root;
	struct hamt_node **path[MAX_LEVEL] = {NULL};
	int level = 0;

	while (1) {
		path[level++] = hnode;
		void **n = get_node(*hnode, keymasked & BUCKET_MASK);
		
		// Not found
		if ( !n || !GET_POINTER(*n)) {
			// printk("%X %d NOT FOUND REMOVE\n", key, key);
			return ;
		}
		if(IS_LEAF(*n)) {
			struct hamt_leaf *hleaf = (struct hamt_leaf *) GET_POINTER(*n);
			//printk("removing hleaf->key = %X (%d) hleaf->bucket = %p\n", hleaf->key, hleaf->key, (void *)hleaf->bucket);
			remove_node(hnode, keymasked & BUCKET_MASK);

			// +++ DEBUG +++
			struct hamt_entry *entry;
			struct hlist_node *hn;
			hlist_for_each_entry_safe(entry, hn, &hleaf->bucket, node) {
				//printk("entry->value: %u\n", *((unsigned int *)entry->value));
				hlist_del(&entry->node);
				kfree(entry);
			}
			// ++++++++++++

			kfree(hleaf);
			goto out;
		}

		hnode = (struct hamt_node **) n;
		keymasked = keymasked >> BUCKET_SIZE_BITS;
	}
out:
	for (int i = level-1; i >= 1; i--) {
		if (isEmpty(*path[i])) {
			kfree(*path[i]);
			remove_node(path[i-1], (key >> (BUCKET_SIZE_BITS*(i-1))) & BUCKET_MASK);
		}
	}
}
EXPORT_SYMBOL(hamt_remove);
