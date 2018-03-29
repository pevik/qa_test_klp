/*
 * sleep_test_mod - create file in /sys/kernel/debug/ that makes processes
 * reading it spin in kernel for some time
 *
 *  Copyright (c) 2018 SUSE
 *   Author: Libor Pechacek
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/fs.h>

static struct dentry *debugfs_root;

static ssize_t sleep_read(struct file *f, char __user *u, size_t s, loff_t *o)
{
	ssleep(10);

	return 0;
}

static const struct file_operations sleep_fops = {
        .read = &sleep_read,
};

static int sleep_test_init(void)
{
	int ret;
	struct dentry *d;

#ifdef CONFIG_DEBUG_FS
	debugfs_root = debugfs_create_dir("klp_tc9", NULL);
        if (IS_ERR_OR_NULL(debugfs_root)) {
                ret = -ENXIO;
		goto out;
	}

	d = debugfs_create_file("sleep_10s", S_IRUGO,
			debugfs_root, NULL, &sleep_fops);
        if (!d) {
                ret = -ENOMEM;
		goto dealloc_root;
	}

	return 0;

dealloc_root:
	debugfs_remove(debugfs_root);

out:
        return ret;
#else
	#error "This patch needs CONFIG_DEBUG_FS set"
#endif
}

static void sleep_test_exit(void)
{
	debugfs_remove_recursive(debugfs_root);
}

module_init(sleep_test_init);
module_exit(sleep_test_exit);
MODULE_LICENSE("GPL");
