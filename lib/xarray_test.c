#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/prandom.h>
#include <linux/slab.h>
#include <asm/timex.h>
#include <linux/prandom.h>
#include <linux/xarray.h>

static unsigned long long seed = 3141592653589793238ULL;
static struct rnd_state rnd;

static void *xa_mk_index(unsigned long index)
{
	return xa_mk_value(index & LONG_MAX);
}

static int test_insert_search_del_speed(void)
{
	DEFINE_XARRAY(hroot);
	cycles_t time1, time2, time;
	int k;
	prandom_seed_state(&rnd, seed);

	unsigned int perf_loops = 10;
	unsigned int size = 1000*20;
	unsigned long *tests = kmalloc_array(size, sizeof(unsigned long), GFP_KERNEL);
	for (unsigned int i = 0; i < size; i++) {
		tests[i] = (unsigned long) prandom_u32_state(&rnd);
		// printk("tests[%u]: %lx\n", i, tests[i]);
	}
	tests[size-1] = 0;
	
	printk("num nodes: %d\n", xarray_get_num_nodes(&hroot));

	printk("insert+remove...\n");
	time1 = get_cycles();
	for (k = 0; k < perf_loops; k++) {
		for (unsigned int i = 0; tests[i]; i++) {
			// hamt_insert(&hroot, (void *)&tests[i], tests[i]);
			xa_store(&hroot, tests[i], xa_mk_index(tests[i]), GFP_KERNEL);
		}
		
		for (unsigned int i = 0; tests[i]; i++) {
			// hamt_remove(&hroot, tests[i]);
			xa_erase(&hroot, tests[i]);
		}
	}
	time2 = get_cycles();
	time = time2 - time1;
	time = div_u64(time, perf_loops);
	printk("	insert+remove %u elements: %llu cycles\n", size-1, (unsigned long long)time);

	for (unsigned int i = 0; tests[i]; i++) {
		xa_store(&hroot, tests[i], xa_mk_index(tests[i]), GFP_KERNEL);
	}

	printk("num nodes: %d\n", xarray_get_num_nodes(&hroot));
	
	printk("searching...\n");
	time1 = get_cycles();
	for (k = 0; k < perf_loops; k++) {
		for (unsigned int i = 0; tests[i]; i++) {
			void *entry = xa_load(&hroot, tests[i]);
			if (!entry)
				printk("ERRO: hamt_search %lu not found\n", tests[i]);
			//printk("found %lx\n", (unsigned long) xa_to_value(entry));
		}
	}
	time2 = get_cycles();
	time = time2 - time1;
	time = div_u64(time, perf_loops);
	printk("	search %u elements: %llu cycles\n", size-1, (unsigned long long)time);

	for (unsigned int i = 0; tests[i]; i++) {
		xa_erase(&hroot, tests[i]);
	}

	kfree(tests);
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
