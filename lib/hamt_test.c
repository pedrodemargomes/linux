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

static int test_insert_search_del(void)
{
	struct hamt_root hroot = {0};

	prandom_seed_state(&rnd, seed);

	unsigned int size = 10000*16;
	//int tests[] = {0x0111, 0x1111, 0x2111, 0x3111, 0x3011, 0x0021, NULL};
	unsigned int *tests = kmalloc_array(size, sizeof(unsigned int), GFP_KERNEL);
	for (unsigned int i = 0; i < size; i+=16) {
		unsigned int x = (unsigned int) prandom_u32_state(&rnd) & ~0xF;
		tests[i] = reverse_bits(x);
		for (unsigned int j = 1; j < 16; j++)
			tests[i+j] = reverse_bits(x+j);
		// printk("%X (%u) ", tests[i], tests[i]);
	}
	printk("++++++ HAMT TEST ++++++\n");
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
	for (unsigned int i = 0; tests[i]; i++)
		hamt_remove(&hroot, tests[i]);

	printk("searching...\n");
	for (unsigned int i = 0; tests[i]; i++) {
		struct hamt_entry *entry;
		struct hlist_head *head = hamt_search(&hroot, tests[i]);

		if (head) {
			printk("ERRO: hamt_search %u not found\n", tests[i]);
			hlist_for_each_entry(entry, head, node) {
				printk("entry->value: %u\n", *((unsigned int *)entry->value));
			}
		}
	}

	kfree(tests);
	return 0; /* Fail will directly unload the module */
}

static void test_remove_path(void) 
{
	unsigned int x;
	struct hamt_root hroot = {0};
	prandom_seed_state(&rnd, seed);

	unsigned int tests[2];
	x = (unsigned int) prandom_u32_state(&rnd) & ~0xF;
	tests[0] = reverse_bits(x);
	tests[1] = reverse_bits(x+1);

	hamt_insert(&hroot, (void *)&tests[0], tests[0]);
	hamt_insert(&hroot, (void *)&tests[1], tests[1]);
	
	hamt_remove(&hroot, tests[0]);	
	hamt_remove(&hroot, tests[1]);
	printk("hroot: %px\n", &hroot);
}

static int __init hamt_test_init(void)
{
	printk("test_insert_search_del\n");
	test_insert_search_del();
	
	printk("test_remove_path\n");
	test_remove_path();
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
