#define WR_SVALUE _IOW('a','a', int32_t*)
#define WR_AVALUE _IOW('a','b', char*)
#define RD_MY_PAGE _IOR('a','c', struct my_page*)
#define RD_MY_SYSCALL_INFO _IOR('a','d', struct my_syscall_info*)

struct my_seccomp_data {
	int nr;
	__u32 arch;
	__u64 instruction_pointer;
	__u64 args[6];
};

struct my_syscall_info {
	__u64			sp;
	struct my_seccomp_data	data;
};

struct my_page {
	unsigned long flags;
	void* mapping;
	long vm_address;
};
