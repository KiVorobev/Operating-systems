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

#include "cmd_mode.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Vorobyev");
MODULE_DESCRIPTION("My ioctl driver");
MODULE_VERSION("1.0");

const int MYMAJOR = 28;

int pid = 0;

static void print_my_page(const struct my_page* mp) {
	printk("\nPage:\n");
	printk("flags: %lu\n", mp->flags);
	printk("Virtual address: %ld, Page address: %p\n", mp->vm_address, mp->mapping);
}

static void print_my_syscall_info(const struct my_syscall_info* msi) {
	printk("\nSyscall_info:\n");
	printk("sp %lld\n", msi->sp);
	printk("nr %d\n", msi->data.nr);
	printk("instruction_pointer %lld\n", msi->data.instruction_pointer);
	printk("arg %d  0x%08X\n", 0, (unsigned int) msi->data.args[0]);
	printk("arg %d  0x%08X\n", 1, (unsigned int) msi->data.args[1]);
	printk("arg %d  0x%08X\n", 2, (unsigned int) msi->data.args[2]);
	printk("arg %d  0x%08X\n", 3, (unsigned int) msi->data.args[3]);
	printk("arg %d  0x%08X\n", 4, (unsigned int) msi->data.args[4]);
	printk("arg %d  0x%08X\n", 5, (unsigned int) msi->data.args[5]);
}

static struct page* get_page_by_mm_and_address(struct mm_struct* mm, long address) {
	pgd_t* pgd;
	p4d_t* p4d;
	pud_t* pud;
	pmd_t* pmd;
	pte_t* pte;
	struct page* page;
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

// This function will be called when we write IOCTL on the Device file
static long driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	struct my_page mp;
	struct my_syscall_info msi;
	struct page* page;
	struct task_struct* task;
	struct mm_struct* mm;
	struct vm_area_struct* vm;
	switch(cmd) {
		case WR_PID:
			if(copy_from_user(&pid, (int*) arg, sizeof(pid))) pr_err("Data write error!\n");
			pr_info("Pid = %d\n", pid);
			break;
		case RD_MY_PAGE:
			task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
			mm = task->mm;
			vm = mm->mmap;
			if (mm == NULL) {
				printk(KERN_INFO "Task struct hasn't mm\n");
			}
			long address = vm->vm_start;
			long end = vm->vm_end;
			while (address <= end) {
				page = get_page_by_mm_and_address(mm, address);
				if (page != NULL) {
					mp.flags = page->flags;
					mp.mapping = (void*)page->mapping;
					mp.vm_address = address;
					break;
				}
				address += PAGE_SIZE;
			}
			print_my_page(&mp);
			if(copy_to_user((struct my_page*) arg, &mp, sizeof(struct my_page))) {
				printk(KERN_INFO "Data read error!\n");
			}
			break;
		case RD_MY_SYSCALL_INFO:
			task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
			unsigned long args[6] = {};
			struct pt_regs* regs = task_pt_regs(task);
			msi.sp = user_stack_pointer(regs);
			msi.data.instruction_pointer = instruction_pointer(regs);
			msi.data.nr = syscall_get_nr(task, regs);
			if (msi.data.nr != -1L) syscall_get_arguments(task, regs, args);
			print_my_syscall_info(&msi);
			if(copy_to_user((struct my_syscall_info*) arg, &msi, sizeof(struct my_syscall_info))) printk(KERN_INFO "Data read error!\n");
			break;
		default:
			pr_info("Command not found!");
			break;
	}
	return 0;
}

static struct file_operations fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = driver_ioctl,
};

static int __init ioctl_core_init(void) {
	printk(KERN_INFO "Core mode is started, hello, world!\n");
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
	printk(KERN_INFO "Core mode is finished, goodbye!\n");
}

module_init(ioctl_core_init);
module_exit(ioctl_core_exit);
