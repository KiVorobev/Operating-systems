#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <linux/ptrace.h>

#define WR_AVALUE _IOW('a','a',struct answer*)
#define WR_SVALUE _IOW('a','c',struct answer*)
#define RD_VALUE _IOR('a','b',struct answer*)

struct seccomp_data {
    int nr;
    __u32 arch;
    __u64 instruction_pointer;
    __u64 args[6];
};

struct syscall_info {
    __u64 sp;
    struct seccomp_data data;
};

struct my_page {
    unsigned long flags;
    void *mapping;
    long vm_address;
};

struct answer {
    struct my_page mp;
    struct syscall_info si;
};

static void print_line(void) {
    printf("________________________\n");
}

static void print_syscall_info(struct syscall_info si) {
    printf("\nSyscall_info:\n\n");
    printf("sp %lld\n", si.sp);
    printf("nr %d\n", si.data.nr);
    printf("instruction_pointer %lld\n", si.data.instruction_pointer);
    for (int i = 0; i < 6; i++) printf("arg %d  0x%08X\n", i, (unsigned int) si.data.args[i]);
}

static void print_page(struct my_page mp) {
    printf("\nPage:\n");
    printf("flags: %lu\n", mp.flags);
    printf("Virtual address: %ld, Page address: %p\n", mp.vm_address, mp.mapping);
}

int main(int argc, char *argv[]) {
    int fd;
    if (argc < 2) {
        printf("The program needs an argument - a path to file!\n");
        return -1;
    }
    int32_t pid = 1889;
    char *path = argv[1];
    struct answer answer;
    printf("\nOpening a driver...\n");
    fd = open("/dev/etc_device", O_WRONLY);
    if (fd < 0) {
        printf("Cannot open device file:(\n");
        return 0;
    }
    // Writing a pid to driver
    ioctl(fd, WR_SVALUE, (int32_t *) &pid);
    // Writing a path to driver
    ioctl(fd, WR_AVALUE, path);
    // Reading a value from driver
    ioctl(fd, RD_VALUE, (struct answer *) &answer);
    print_line();
    print_page(answer.mp);
    print_line();
    print_syscall_info(answer.si);
    print_line();
    printf("Closing a driver...\n");
    close(fd);
}