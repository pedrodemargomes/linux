#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/prandom.h>
#include <linux/slab.h>
#include <asm/timex.h>
#include <linux/rhashtable.h>

static unsigned long long seed = 3141592653589793238ULL;
static struct rnd_state rnd;

/* Entry stored in the rhashtable */
struct test_node {
	struct rhash_head node;
	unsigned long key;
};

/* rhashtable parameters */
static const struct rhashtable_params ht_params = {
	.head_offset = offsetof(struct test_node, node),
	.key_offset  = offsetof(struct test_node, key),
	.key_len     = sizeof(unsigned long),
	.automatic_shrinking = true,
};

static int test_insert_search_del_speed(void)
{
	struct rhashtable ht;
	cycles_t time1, time2, time;
	int k, ret;

	prandom_seed_state(&rnd, seed);

	unsigned int perf_loops = 10;
	unsigned int size = 1000 * 20;

	unsigned long *tests = kmalloc_array(size, sizeof(unsigned long), GFP_KERNEL);
	if (!tests)
		return -ENOMEM;

	for (unsigned int i = 0; i < size; i++)
		tests[i] = (unsigned long)prandom_u32_state(&rnd);

	tests[size - 1] = 0;

	ret = rhashtable_init(&ht, &ht_params);
	if (ret) {
		kfree(tests);
		return ret;
	}

	printk("insert+remove...\n");

	time1 = get_cycles();
	for (k = 0; k < perf_loops; k++) {

		/* Insert */
		for (unsigned int i = 0; tests[i]; i++) {
			struct test_node *entry;

			entry = kmalloc(sizeof(*entry), GFP_KERNEL);
			if (!entry)
				continue;

			entry->key = tests[i];
			rhashtable_insert_fast(&ht, &entry->node, ht_params);
		}

		/* Remove */
		for (unsigned int i = 0; tests[i]; i++) {
			struct test_node *entry;

			entry = rhashtable_lookup_fast(&ht, &tests[i], ht_params);
			if (entry) {
				rhashtable_remove_fast(&ht, &entry->node, ht_params);
				kfree(entry);
			}
		}
	}
	time2 = get_cycles();

	time = div_u64(time2 - time1, perf_loops);

	printk("	insert+remove %u elements: %llu cycles\n",
	       size - 1, (unsigned long long)time);

	/* Reinsert for search test */
	for (unsigned int i = 0; tests[i]; i++) {
		struct test_node *entry = kmalloc(sizeof(*entry), GFP_KERNEL);
		if (!entry)
			continue;
		entry->key = tests[i];
		rhashtable_insert_fast(&ht, &entry->node, ht_params);
	}

	printk("searching...\n");

	time1 = get_cycles();
	for (k = 0; k < perf_loops; k++) {
		for (unsigned int i = 0; tests[i]; i++) {
			struct test_node *entry;

			entry = rhashtable_lookup_fast(&ht, &tests[i], ht_params);
			if (!entry)
				printk("ERROR: key %lu not found\n", tests[i]);
		}
	}
	time2 = get_cycles();

	time = div_u64(time2 - time1, perf_loops);

	printk("	search %u elements: %llu cycles\n",
	       size - 1, (unsigned long long)time);

	/* Final cleanup */
	for (unsigned int i = 0; tests[i]; i++) {
		struct test_node *entry;

		entry = rhashtable_lookup_fast(&ht, &tests[i], ht_params);
		if (entry) {
			rhashtable_remove_fast(&ht, &entry->node, ht_params);
			kfree(entry);
		}
	}

	rhashtable_destroy(&ht);
	kfree(tests);

	return 0;
}

static int __init rhashtable_test_init(void)
{
	printk("++++++ RHASHTABLE TEST ++++++\n");
	printk("test_insert_search_del_speed\n");

	test_insert_search_del_speed();

	return -1; /* Force unload */
}

static void __exit rhashtable_test_exit(void)
{
	printk(KERN_ALERT "rhashtable test exit\n");
}

module_init(rhashtable_test_init);
module_exit(rhashtable_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pedro Demarchi Gomes");
MODULE_DESCRIPTION("rhashtable performance test");
