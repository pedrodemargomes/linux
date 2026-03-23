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

/*
static int getfilepagecacheinfo(char *filename) {
    struct inode *inode;
    struct path path;
    int error;

    // Use kern_path to look up the path and fill the 'path' structure
    //error = kern_path(path_name, LOOKUP_FOLLOW, &path);
    if (error) {
        printk(KERN_ERR "Error in kern_path: %d\n", error);
        return error;
    }

    // Get the inode from the dentry associated with the path
    inode = path.dentry->d_inode;

    // Log the inode number (i_ino field of the inode structure)
    printk(KERN_INFO "Path name: %s, inode number: %lu\n", path_name, inode->i_ino);

    // You can now access other inode information, e.g., inode->i_size, inode->i_uid
    printk(KERN_INFO "File size: %lld bytes\n", inode->i_size);

    // Release the path reference after use
    path_put(&path);

    return 0;
}
*/

static ssize_t mywrite(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos)
{
	printk( KERN_DEBUG "write handler\n");
	char buf[10];
	int n, ret ;

	if(copy_from_user(buf, ubuf, 10))
		return -EFAULT;

	ret = kstrtouint(buf, 10, &n);
	if (ret)
		return ret;
	if (n < 0)
		return -EINVAL;
	printk("n: %d\n", n);

	return 10;
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
