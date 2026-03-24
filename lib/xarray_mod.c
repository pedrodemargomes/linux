#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static struct proc_dir_entry *ent;

static int getfilepagecacheinfo(char *filename) {
	struct inode *inode;
	struct path path;
	int error;

	printk(KERN_INFO "Path name: %s\n", filename);
	// Use kern_path to look up the path and fill the 'path' structure
	error = kern_path(filename, LOOKUP_FOLLOW, &path);
	if (error) {
		printk(KERN_ERR "Error in kern_path: %d\n", error);
		return error;
	}

	// Get the inode from the dentry associated with the path
	inode = path.dentry->d_inode;

	struct address_space *mapping = inode->i_mapping;
	int nnodes = xarray_get_num_nodes(&mapping->i_pages);
	int nemptyslots = xarray_get_num_null_entries(&mapping->i_pages);
	printk("ino: %lu xarray_num_nodes: %d xarray_get_num_null_entries: %d\n", inode->i_ino, nnodes, nemptyslots);

	// Log the inode number (i_ino field of the inode structure)
	printk(KERN_INFO "Path name: %s, inode number: %lu\n", filename, inode->i_ino);

	// You can now access other inode information, e.g., inode->i_size, inode->i_uid
	printk(KERN_INFO "File size: %lld bytes\n", inode->i_size);

	// Release the path reference after use
	path_put(&path);

	return 0;
}

static ssize_t mywrite(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos)
{
	printk( KERN_DEBUG "write handler\n");
	char buf[100];

	if(copy_from_user(buf, ubuf, 100))
		return -EFAULT;
	buf[count-1] = '\0';
	getfilepagecacheinfo(buf);

	return 100;
}

static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos)
{
	printk( KERN_DEBUG "read handler\n");
	return 0;
}

static struct proc_ops myops =
{
	.proc_read = myread,
	.proc_write = mywrite,
};

static int myinit(void) {
	ent = proc_create("getpagecacheinfo", 0660, NULL, &myops);
	return 0;
}

static void myexit(void) {
    return;
}

module_init(myinit);
module_exit(myexit);

MODULE_LICENSE("GPL v2");
