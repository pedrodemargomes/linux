#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/prandom.h>
#include <linux/slab.h>
#include <asm/timex.h>
#include <linux/hamt.h>
#include <linux/prandom.h>

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
	unsigned int *p;
	
	printk("num nodes: %d\n", hamt_get_num_nonleaf_nodes(&hroot));
	printk("size: %lu\n", hamt_get_size(&hroot));
	
	p = kmalloc(sizeof(unsigned int), GFP_KERNEL);
	*p = 4194304;
	hamt_insert(&hroot, (void *) p, reverse_bits(*p));
	printk("num nodes: %d\n", hamt_get_num_nonleaf_nodes(&hroot));
	printk("size: %lu\n", hamt_get_size(&hroot));
	
	p = kmalloc(sizeof(unsigned int), GFP_KERNEL);
	*p = 4194305;
	hamt_insert(&hroot, (void *) p, reverse_bits(*p));
	printk("num nodes: %d\n", hamt_get_num_nonleaf_nodes(&hroot));
	printk("size: %lu\n", hamt_get_size(&hroot));
	
	hamt_remove(&hroot, 4194304);
	hamt_remove(&hroot, 4194305);
	/*
	for (unsigned int i = 1; i < 10; i++) {
		unsigned int *p = kmalloc(sizeof(unsigned int), GFP_KERNEL);
		*p = i;
		hamt_insert(&hroot, (void *) p, i-10);
		printk("num nodes: %d\n", hamt_get_num_nonleaf_nodes(&hroot));
		printk("size: %lu\n", hamt_get_size(&hroot));
	}

	for (unsigned int i = 1; i < 10; i++) {
		hamt_remove(&hroot, i-10);
	}
	*/

	FREE_HAMT_ROOT(hroot);
	return 0;
}

static int __init hamt_test_init(void)
{
	printk("++++++ HAMT TEST ++++++\n");

	printk("test_insert_search_del_speed\n");
	test_insert_search_del_speed();
	
	return -1; /* Fail will directly unload the module */
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
