#include <debug/debug.h>
#include <devices/tty/pty.h>
#include <errno.h>
#include <fb/fb.h>
#include <fs/devtmpfs.h>
#include <fs/partition.h>
#include <fs/ramdisk.h>
#include <fs/streams.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <ipc/pipe.h>
#include <ipc/socket.h>
#include <kernel.h>
#include <klibc/kargs.h>
#include <klibc/module.h>
#include <klibc/random.h>
#include <mm/mmap.h>
#include <mm/pmm.h>
#include <net/net.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>
#include <devices/tty/console.h>

const char *module_list[] = {
#if defined(__x86_64__)
	"/usr/lib/modules/ps2.ko", "/usr/lib/modules/nvme.ko"
#endif
};

#define MODULE_LIST_SIZE (sizeof(module_list) / sizeof(module_list[0]))
#define ONE_SECOND (uint64_t)(1000 * 1000 * 1000)

struct sysinfo {
	long uptime;			 /* Seconds since boot */
	unsigned long loads[3];	 /* 1, 5, and 15 minute load averages */
	unsigned long totalram;	 /* Total usable main memory size */
	unsigned long freeram;	 /* Available memory size */
	unsigned long sharedram; /* Amount of shared memory */
	unsigned long bufferram; /* Memory used by buffers */
	unsigned long totalswap; /* Total swap space size */
	unsigned long freeswap;	 /* swap space still available */
	unsigned short procs;	 /* Number of current processes */
	unsigned long totalhigh; /* Total high memory size */
	unsigned long freehigh;	 /* Available high memory size */
	unsigned int mem_unit;	 /* Memory unit size in bytes */
	char _f[20 - 2 * sizeof(long) - sizeof(int)]; /* Padding to 64 bytes */
};

void syscall_sysinfo(struct syscall_arguments *args) {
	struct sysinfo to_user = {0};

	uint64_t page_info[2] = {0};
	pmm_get_memory_info(page_info);

	to_user.uptime = (long)timer_count() / 1000;
	to_user.loads[0] = 0;
	to_user.loads[1] = 0;
	to_user.loads[2] = 0;
	to_user.totalram = page_info[0] * PAGE_SIZE;
	to_user.freeram = page_info[1] * PAGE_SIZE;
	to_user.sharedram = 0;
	to_user.bufferram = 0;
	to_user.totalswap = 0;
	to_user.freeswap = 0;
	extern int64_t pid;
	to_user.procs = (uint16_t)(pid);
	to_user.mem_unit = 0;

	args->ret = syscall_helper_copy_to_user(args->args0, &to_user,
											sizeof(struct sysinfo))
					? 0
					: -1;
}

#ifdef KERNEL_ABUSE
void kernel_dummy_sleeping_thread(void) {
	for (;;) {
		thread_sleep(sched_get_running_thread(), ONE_SECOND * 5);
	}
}

void kernel_dummy_threads(uint64_t id) {
	for (;;) {
		kputchar_('0' + id);
		kputchar_('A' + prcb_return_current_cpu()->cpu_number);
		halt();
		sched_yield(true);
	}
}
#endif

void kernel_main(void *args) {
	vfs_init();
	tmpfs_init();
	devtmpfs_init();
	vfs_mount(vfs_root, NULL, "/", "tmpfs");
	vfs_create(vfs_root, "/tmp", 0755 | S_IFDIR);
	vfs_mount(vfs_root, NULL, "/tmp", "tmpfs");
	vfs_create(vfs_root, "/dev", 0755 | S_IFDIR);
	vfs_mount(vfs_root, NULL, "/dev", "devtmpfs");
	streams_init();
	randdev_init();

	kprintf("Hello I am %s\n", sched_get_running_thread()->mother_proc->name);

	if (args != NULL) {
		uint64_t *module_info = (uint64_t *)args;
		kprintf("Ramdisk located at %p\n", module_info[0]);
		ramdisk_install(module_info[0], module_info[1]);
	}

	uint64_t mod_ret = 0;
	for (size_t i = 0; i < MODULE_LIST_SIZE; i++) {
		mod_ret = module_load(module_list[i]);
		if (mod_ret) {
			kprintf("Failed to load kernel module %s. Return value: %p\n",
					module_list[i], mod_ret);
		}
	}

	module_dump();

	// Done so that gcc will stop REMOVING this function
	partition_enumerate(NULL, NULL);

	fbdev_init();

	syscall_register_handler(0x0, syscall_read);
	syscall_register_handler(0x1, syscall_write);
	syscall_register_handler(0x2, syscall_open);
	syscall_register_handler(0x3, syscall_close);
	syscall_register_handler(0x8, syscall_seek);
	syscall_register_handler(0x9, syscall_mmap);
	syscall_register_handler(0xa, syscall_mprotect);
	syscall_register_handler(0xb, syscall_munmap);
	syscall_register_handler(0x10, syscall_ioctl);
	syscall_register_handler(0x48, syscall_fcntl);
	syscall_register_handler(0x4f, syscall_getcwd);
	syscall_register_handler(0x50, syscall_chdir);
	syscall_register_handler(0x59, syscall_readdir);
	syscall_register_handler(0x101, syscall_openat);
	syscall_register_handler(0x102, syscall_mkdirat);
	syscall_register_handler(0x106, syscall_fstatat);
	syscall_register_handler(0x107, syscall_unlinkat);
	syscall_register_handler(0x109, syscall_linkat);
	syscall_register_handler(0x10b, syscall_readlinkat);
	syscall_register_handler(0x10c, syscall_fchmodat);
	syscall_register_handler(0x124, syscall_dup3);
	syscall_register_handler(0x125, syscall_pipe);
	syscall_register_handler(0xff, syscall_openpty);
	syscall_register_handler(0x63, syscall_sysinfo);
	syscall_register_handler(0x10f, syscall_ppoll);

	syscall_register_handler(0x29, syscall_socket);
	syscall_register_handler(0x2a, syscall_connect);
	syscall_register_handler(0x2b, syscall_accept);
	syscall_register_handler(0x2f, syscall_recvmsg);
	syscall_register_handler(0x31, syscall_bind);
	syscall_register_handler(0x32, syscall_listen);
	syscall_register_handler(0x34, syscall_getpeername);
	syscall_register_handler(0x53, syscall_socketpair);

	console_init();
	
	std_console_device =
		(vfs_get_node(vfs_root, "/dev/console", true))->resource;

	char *argv[] = {"/usr/bin/init", NULL};
	if (kernel_arguments.kernel_args & KERNEL_ARGS_INIT_PATH_GIVEN) {
		argv[0] = kernel_arguments.init_binary_path;
	}

	kprintf("Running init binary %s\n", argv[0]);

	if (!process_create_elf("init", PROCESS_READY_TO_RUN, 20000, argv[0],
							sched_get_running_thread()->mother_proc))
		panic("Failed to run init binary!\n");

#ifdef KERNEL_ABUSE
	for (uint64_t i = 0; i < prcb_return_installed_cpus(); i++) {
		thread_create((uintptr_t)kernel_dummy_threads, i, false,
					  sched_get_running_thread()->mother_proc);
	}

	thread_create((uintptr_t)kernel_dummy_sleeping_thread, 0, false,
				  sched_get_running_thread()->mother_proc);
#endif

	for (;;) {
		sched_yield(true);
	}
}
