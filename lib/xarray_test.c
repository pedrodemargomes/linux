#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/prandom.h>
#include <linux/slab.h>
#include <asm/timex.h>
#include <linux/prandom.h>
#include <linux/xarray.h>

static int test_insert_search_del_speed(void)
{
	DEFINE_XARRAY(hroot);
	int i = 0;
	printk("num nodes: %d num NULL entries: %d\n", xarray_get_num_nodes(&hroot), xarray_get_num_null_entries(&hroot));

	for (i = 1; i <= 64; i++) {
		int *v = kmalloc(sizeof(int), GFP_KERNEL);
		*v = i;
		printk("insert %d\n", i);
		xa_store(&hroot, i, v, GFP_KERNEL);
		printk("num nodes: %d num NULL entries: %d\n", xarray_get_num_nodes(&hroot), xarray_get_num_null_entries(&hroot));
	}

	for (i = 1; i <= 64; i++) {
		printk("remove %d\n", i);
		xa_erase(&hroot, i);
		printk("num nodes: %d num NULL entries: %d\n", xarray_get_num_nodes(&hroot), xarray_get_num_null_entries(&hroot));
	}

	xa_destroy(&hroot);
	return 0;
}

static int __init hamt_test_init(void)
{
	printk("++++++ XARRAY TEST ++++++\n");

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
