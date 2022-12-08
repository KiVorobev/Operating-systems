#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/ptrace.h>
#include <linux/pid.h>
#include <linux/netdevice.h>
#include <asm/syscall.h>
#include <linux/sched.h>
#include <linux/namei.h>
#include <linux/mm_types.h>
#include <asm/page.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Vorobyev");
MODULE_DESCRIPTION("My ioctl driver");
MODULE_VERSION("1.0");

#define MYMAJOR 28
#define BUFFER 1024
#define WR_AVALUE _IOW('a','a',struct answer*)
#define WR_SVALUE _IOW('a','c',struct answer*)
#define RD_VALUE _IOR('a','b',struct answer*)

struct my_page {
    unsigned long flags;
    void *mapping;
    long vm_address;
};

struct answer {
    struct my_page *mp;
    struct syscall_info *si;
};

static void print_my_page(struct my_page *mp) {
    printk("\nPage:\n");
    printk("flags: %lu\n", mp->flags);
    printk("Virtual address: %ld, Page address: %p\n", mp->vm_address, mp->mapping);
}

static void print_my_syscall_info(struct syscall_info *si) {
    printk("\nSyscall_info:\n");
    printk("sp %lld\n", si->sp);
    printk("nr %d\n", si->data.nr);
    printk("instruction_pointer %lld\n", si->data.instruction_pointer);
}

static struct page *get_page_by_mm_and_address(struct mm_struct *mm, long address) {
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page = NULL;
    pgd = pgd_offset(mm, address);
    if (!pgd_present(*pgd)) return NULL;
    p4d = p4d_offset(pgd, address);
    if (!p4d_present(*p4d)) return NULL;
    pud = pud_offset(p4d, address);
    if (!pud_present(*pud)) return NULL;
    pmd = pmd_offset(pud, address);
    if (!pmd_present(*pmd)) return NULL;
    pte = pte_offset_map(pmd, address);
    if (!pte_present(*pte)) return NULL;
    page = pte_page(*pte);
    return page;
}

/*
static struct my_page* get_my_page(void) {
	struct my_page* my_page;
	struct page* page;
	struct task_struct* task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
	struct mm_struct* mm = task->mm;
	if (mm == NULL) {
		printk(KERN_INFO "Task struct hasn't mm\n");
		return my_page;
	}
	struct vm_area_struct* vm = mm->mmap;
	long address = vm->vm_start;
	long end = vm->vm_end;
	while (address <= end) {
		page = get_page_by_mm_and_address(mm, address);
		if (page != NULL) {
			my_page->flags = page->flags;
			my_page->mapping = (void*)page->mapping;
			my_page->vm_address = address;
			printk("\nPage:\n");
			printk("flags: %lu\n", my_page->flags);
			printk("Virtual address: %ld, Page address: %p\n", my_page->vm_address, my_page->mapping);
			break;
		}
		address += PAGE_SIZE;
	}
	return my_page;
}

static struct syscall_info* get_my_syscall_info(void) {
	struct syscall_info* result;
	struct task_struct* task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
	unsigned long args[6] = {};
	result = vmalloc(sizeof(struct syscall_info*));
	struct pt_regs* regs = task_pt_regs(task);
	result->sp = user_stack_pointer(regs);
	result->data.instruction_pointer = instruction_pointer(regs);
	result->data.nr = syscall_get_nr(task, regs);
	if (result->data.nr != -1L) syscall_get_arguments(task, regs, args);
	result->sp = user_stack_pointer(regs);
	return result;
}
*/

static int driver_open(struct inode *inode, struct file *file) {
    pr_info("Device File Opened...!!!\n");
    return 0;
}


// This function will be called when we close the Device file
static int driver_close(struct inode *inode, struct file *file) {
    pr_info("Device File Closed...!!!\n");
    return 0;
}

// This function will be called when we write IOCTL on the Device file
static long driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int pid;
    struct path path;
    const int buffer = BUFFER;
    char path_arg[buffer];
    struct answer *answer;
    answer = vmalloc(sizeof(struct answer *));
    answer->mp = vmalloc(sizeof(struct my_page *));
    answer->si = vmalloc(sizeof(struct syscall_info *));
    struct page *page;
    struct task_struct *task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
    struct mm_struct *mm = task->mm;
    switch (cmd) {
        case WR_SVALUE:
            if (copy_from_user(&pid, (int *) arg, sizeof(pid))) {
                pr_err("Data write error!\n");
            }
            pr_info("Pid = %d\n", pid);
            break;
        case WR_AVALUE:
            copy_from_user(path_arg, (char *) arg, buffer);
            kern_path(path_arg, LOOKUP_FOLLOW, &path);
            break;
        case RD_VALUE:

            // get my_page
            if (mm == NULL) {
                printk(KERN_INFO
                "Task struct hasn't mm\n");
            }
            struct vm_area_struct *vm = mm->mmap;
            long address = vm->vm_start;
            long end = vm->vm_end;
            while (address <= end) {
                page = get_page_by_mm_and_address(mm, address);
                if (page != NULL) {
                    answer->mp->flags = page->flags;
                    answer->mp->mapping = (void *) page->mapping;
                    answer->mp->vm_address = address;
                    break;
                }
                address += PAGE_SIZE;
            }

            // get syscall_info
            unsigned long args[6] = {};
            struct pt_regs *regs = task_pt_regs(task);
            answer->si->sp = user_stack_pointer(regs);
            answer->si->data.instruction_pointer = instruction_pointer(regs);
            answer->si->data.nr = syscall_get_nr(task, regs);
            if (answer->si->data.nr != -1L) syscall_get_arguments(task, regs, args);
            answer->si->sp = user_stack_pointer(regs);

            // prints
            print_my_page(answer->mp);
            print_my_syscall_info(answer->si);
            if (copy_to_user((struct answer *) arg, answer, sizeof(struct answer *))) {
                pr_err("Data read error!\n");
            }
            break;
    }
    vfree(answer);
    return 0;
}

static struct file_operations fops = {
        .owner          = THIS_MODULE,
        .open           = driver_open,
        .release        = driver_close,
        .unlocked_ioctl = driver_ioctl,
};


static int __init ioctl_core_init(void) {
    printk(KERN_INFO
    "Core mode is started, hello, world!\n");
    int retval;
    retval = register_chrdev(MYMAJOR, "my_driver", &fops);
    if (0 == retval) {
        printk("my_driver device number Major:%d , Minor:%d\n", MYMAJOR, 0);
    } else if (retval > 0) {
        printk("my_driver device number Major:%d , Minor:%d\n", retval >> 20, retval & 0xffff);
    } else {
        printk("Couldn't register device number!\n");
        return -1;
    }
    return 0;
}

static void __exit ioctl_core_exit(void) {
    unregister_chrdev(MYMAJOR, "my_driver");
    printk(KERN_INFO
    "Core mode is finished, goodbye!\n");
}

module_init(ioctl_core_init);
module_exit(ioctl_core_exit);