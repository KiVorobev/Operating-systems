#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <linux/ptrace.h>

#include "cmd_mode.h"

static void print_line(void) {
	printf("________________________\n");
}

static void print_my_syscall_info(const struct my_syscall_info* si) {
	printf("\nSyscall_info:\n\n");
	printf("sp %lld\n", si->sp);
	printf("nr %d\n", si->data.nr);
	printf("instruction_pointer %lld\n", si->data.instruction_pointer);
	for (int i = 0; i < 6; i++) printf("arg %d  0x%08X\n", i, (unsigned int) si->data.args[i]);
}

static void print_page(const struct my_page* mp) {
	printf("\nPage:\n\n");
	printf("flags: %ld\n", mp->flags);
	printf("Virtual address: %ld\n", mp->vm_address);
	printf("Page address: %p\n", mp->mapping);
}

int main(int argc, char *argv[]) {
	int fd;
	if(argc < 2) {
		printf("The program needs an argument - a pid!\n");
		return -1;
	}
	int32_t pid = atoi(argv[1]);
	if (pid < 1) {
		printf("Pid can be only greater than 0\n");
		return -1;
	}
	printf("\nOpening a driver...\n");
	fd = open("/dev/etc_device", O_WRONLY);
	if(fd < 0) {
		printf("Cannot open device file:(\n");
		return 0;
	}
	struct my_page mp;
	struct my_syscall_info msi;
	// Writing a pid to driver
	ioctl(fd, WR_PID, (int32_t*) &pid);
	// Reading my_page from driver
	ioctl(fd, RD_MY_PAGE, &mp);
	print_line();
	print_page(&mp);
	// Reading my_syscall_info from driver
	ioctl(fd, RD_MY_SYSCALL_INFO, &msi);
	print_line();
	print_my_syscall_info(&msi);
	print_line();
	printf("Closing a driver...\n");
	close(fd);
}
