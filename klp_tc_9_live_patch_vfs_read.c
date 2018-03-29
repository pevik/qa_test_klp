/*
 * live_patch_vfs_read - patch vfs_read with the same code
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
#include <linux/livepatch.h>

#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/sched/xacct.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/fsnotify.h>
#include <linux/security.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
#include <linux/compat.h>
#include <linux/mount.h>
#include <linux/fs.h>

int (*klp_tc_rw_verify_area)(int read_write, struct file *file, const loff_t *ppos, size_t count);

ssize_t klp_test_vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
        ssize_t ret;

        if (!(file->f_mode & FMODE_READ))
                return -EBADF;
        if (!(file->f_mode & FMODE_CAN_READ))
                return -EINVAL;
        if (unlikely(!access_ok(VERIFY_WRITE, buf, count)))
                return -EFAULT;

        ret = klp_tc_rw_verify_area(READ, file, pos, count);
        if (!ret) {
                if (count > MAX_RW_COUNT)
                        count =  MAX_RW_COUNT;
                ret = __vfs_read(file, buf, count, pos);
                if (ret > 0) {
                        fsnotify_access(file);
                        add_rchar(current, ret);
                }
                inc_syscr(current);
        }

        return ret;
}

static struct klp_func funcs[] = {
        {
                .old_name = "vfs_read",
                .new_func = klp_test_vfs_read,
        }, { }
};

static struct klp_object objs[] = {
        {
                /* name being NULL means vmlinux */
                .funcs = funcs,
        }, { }
};

static struct klp_patch patch = {
        .mod = THIS_MODULE,
        .objs = objs,
};


static int livepatch_init(void)
{
        int ret;

	unsigned long addr;

        addr = kallsyms_lookup_name("rw_verify_area");
        if (!addr) {
                pr_err("klp_tc-patch: symbol rw_verify_area not resolved\n");
                return -EFAULT;
        }
        klp_tc_rw_verify_area = (int (*)(int , struct file *, const loff_t *, size_t )) addr;

        ret = klp_register_patch(&patch);
        if (ret)
                return ret;
        ret = klp_enable_patch(&patch);
        if (ret) {
                WARN_ON(klp_unregister_patch(&patch));
                return ret;
        }
        return 0;
}

static void livepatch_exit(void)
{
        WARN_ON(klp_unregister_patch(&patch));
}

module_init(livepatch_init);
module_exit(livepatch_exit);
MODULE_LICENSE("GPL");
MODULE_INFO(livepatch, "Y");
