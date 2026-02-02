#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/prandom.h>
#include <linux/slab.h>
#include <asm/timex.h>
#include <linux/hamt.h>

static int __init hamt_test_init(void)
{
	struct hamt_root hroot = {0};	

	unsigned int size = 100000;
	//int tests[] = {0x0111, 0x1111, 0x2111, 0x3111, 0x3011, 0x0021, NULL};
	unsigned int *tests = kmalloc_array(size, sizeof(unsigned int), GFP_KERNEL);
	for (unsigned int i = 0; i < size-1; i++) {
		tests[i] = (unsigned int) i;
		printk("%X (%u) ", tests[i], tests[i]);
	}
	printk("\n");
	tests[size-1] = 0;

	//tests[0] = 0x4B329F2A;
	//tests[1] = 0x30EB1E2A;

	printk("inserting...\n");
	for (unsigned int i = 0; tests[i]; i++) 
		hamt_insert(&hroot, (void *)tests[i], tests[i]);
	
	printk("searching...\n");
	for (unsigned int i = 0; tests[i]; i++) 
		hamt_search(&hroot, tests[i]);

	printk("removing...\n");
	for (unsigned int i = 0; tests[i]; i++) 
		hamt_remove(&hroot, tests[i]);

	printk("searching...\n");
	for (unsigned int i = 0; tests[i]; i++) 
		hamt_search(&hroot, tests[i]);	

	free(tests);
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


int main() {
	
}