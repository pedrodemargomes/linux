#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/prandom.h>
#include <linux/slab.h>
#include <asm/timex.h>
#include <linux/hamt.h>
#include <linux/prandom.h>

static unsigned long long seed = 3141592653589793238ULL;
static struct rnd_state rnd;

static unsigned int reverse_bits(unsigned int x) {
    x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
    x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
    x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
    x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
    x = (x >> 16) | (x << 16);
    return x;
}

static int test_insert_search_del_speed(void)
{
	DEFINE_HAMT(hroot);
	cycles_t time1, time2, time;
	int k;
	prandom_seed_state(&rnd, seed);

	unsigned int perf_loops = 10;
	unsigned int size = 1000*16;
	unsigned int *tests = kmalloc_array(size, sizeof(unsigned int), GFP_KERNEL);
	for (unsigned int i = 0; i < size; i++) {
		tests[i] = (unsigned int) prandom_u32_state(&rnd);
	}
	tests[size-1] = 0;
	
	printk("insert+remove...\n");
	time1 = get_cycles();
	for (k = 0; k < perf_loops; k++) {
		for (unsigned int i = 0; tests[i]; i++) {
			hamt_insert(&hroot, (void *)&tests[i], tests[i]);
		}
		for (unsigned int i = 0; tests[i]; i++) {
			hamt_remove(&hroot, tests[i]);
		}
	}
	time2 = get_cycles();
	time = time2 - time1;
	time = div_u64(time, perf_loops);
	printk("	insert+remove %u elements: %llu cycles\n", size-1, (unsigned long long)time);

	for (unsigned int i = 0; tests[i]; i++) {
		hamt_insert(&hroot, (void *)&tests[i], tests[i]);
	}

	printk("searching...\n");
	time1 = get_cycles();
	for (k = 0; k < perf_loops; k++) {
		for (unsigned int i = 0; tests[i]; i++) {
			struct hamt_entry *entry;
			struct hlist_head *head = hamt_search(&hroot, tests[i]);

			if (!head)
				printk("ERRO: hamt_search %u not found\n", tests[i]);
			else {
				hlist_for_each_entry(entry, head, node) {
					// printk("entry->value: %u\n", *((unsigned int *)entry->value));
				}
			}
		}
	}
	time2 = get_cycles();
	time = time2 - time1;
	time = div_u64(time, perf_loops);
	printk("	search %u elements: %llu cycles\n", size-1, (unsigned long long)time);

	for (unsigned int i = 0; tests[i]; i++) {
		hamt_remove(&hroot, tests[i]);
	}

	kfree(tests);
	FREE_HAMT_ROOT(hroot);
	return 0;
}


static int test_insert_search_del(void)
{
	DEFINE_HAMT(hroot);

	prandom_seed_state(&rnd, seed);

	unsigned int size = 10000*16;
	unsigned int *tests = kmalloc_array(size, sizeof(unsigned int), GFP_KERNEL);
	for (unsigned int i = 0; i < size; i+=256) {
		unsigned int x = (unsigned int) prandom_u32_state(&rnd) & ~0xFF;
		
		tests[i] = reverse_bits(x);
		for (unsigned int j = 1; j < 256; j++)
			tests[i+j] = reverse_bits(x+j);
		// printk("%X (%u) ", tests[i], tests[i]);
	}
	tests[size-1] = 0;

	
	printk("inserting...\n");
	for (unsigned int i = 0; tests[i]; i++) {
		hamt_insert(&hroot, (void *)&tests[i], tests[i]);
	}

	printk("searching...\n");
	for (unsigned int i = 0; tests[i]; i++) {
		struct hamt_entry *entry;
		struct hlist_head *head = hamt_search(&hroot, tests[i]);

		if (!head)
			printk("ERRO: hamt_search %u not found\n", tests[i]);
		else {
			hlist_for_each_entry(entry, head, node) {
				// printk("entry->value: %u\n", *((unsigned int *)entry->value));
			}
		}
	}

	printk("removing...\n");
	for (unsigned int i = 0; tests[i]; i++) {
		hamt_remove(&hroot, tests[i]);
	}

	printk("searching...\n");
	for (unsigned int i = 0; tests[i]; i++) {
		struct hamt_entry *entry;
		struct hlist_head *head = hamt_search(&hroot, tests[i]);

		if (head) {
			printk("ERRO: hamt_search %u found\n", tests[i]);
			hlist_for_each_entry(entry, head, node) {
				printk("entry->value: %u\n", *((unsigned int *)entry->value));
			}
		}
	}

	kfree(tests);
	FREE_HAMT_ROOT(hroot);
	return 0; /* Fail will directly unload the module */
}

static void test_remove_path(void)
{
	unsigned int x;
	DEFINE_HAMT(hroot);
	prandom_seed_state(&rnd, seed);

	unsigned int tests[2];
	x = (unsigned int) prandom_u32_state(&rnd) & ~0xFF;
	tests[0] = reverse_bits(x);
	tests[1] = reverse_bits(x+1);

	hamt_insert(&hroot, (void *)&tests[0], tests[0]);
	hamt_insert(&hroot, (void *)&tests[1], tests[1]);

	for (int i = 0; i < 2; i++) {
		printk("search tests[%d]:\n", i);
		struct hamt_entry *entry;
		struct hlist_head *head = hamt_search(&hroot, tests[i]);
		if (head) {
			hlist_for_each_entry(entry, head, node) {
				printk("entry->value: %x\n", *((unsigned int *)entry->value));
			}
		}
	}


	hamt_remove(&hroot, tests[0]);
	hamt_remove(&hroot, tests[1]);
	
	FREE_HAMT_ROOT(hroot);
}

static int __init hamt_test_init(void)
{
	printk("++++++ HAMT TEST ++++++\n");
	
	printk("test_insert_search_del\n");
	test_insert_search_del();

	printk("test_remove_path\n");
	test_remove_path();
	
	printk("test_insert_search_del_speed\n");
	test_insert_search_del_speed();
	
	return 0; /* Fail will directly unload the module */
}

static void __exit hamt_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
}

module_init(hamt_test_init)
module_exit(hamt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pedro Demarchi Gomes");
MODULE_DESCRIPTION("HAMT test");
