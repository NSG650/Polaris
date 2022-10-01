#include <debug/debug.h>
#include <devices/console.h>
#include <fs/devtmpfs.h>
#include <fs/ramdisk.h>
#include <fs/streams.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <mm/mmap.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

void kernel_main(void *args) {
	vfs_init();
	tmpfs_init();
	devtmpfs_init();
	vfs_mount(vfs_root, NULL, "/", "tmpfs");
	vfs_create(vfs_root, "/dev", 0755 | S_IFDIR);
	vfs_mount(vfs_root, NULL, "/dev", "devtmpfs");
	streams_init();

	uint64_t *module_info = (uint64_t *)args;
	kprintf("Ramdisk located at 0x%p\n", module_info[0]);

	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	ramdisk_install(module_info[0], module_info[1]);

	syscall_register_handler(0x0, syscall_read);
	syscall_register_handler(0x1, syscall_write);
	syscall_register_handler(0x2, syscall_open);
	syscall_register_handler(0x3, syscall_close);
	syscall_register_handler(0x8, syscall_seek);
	syscall_register_handler(0x9, syscall_mmap);
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
	console_init();

	struct vfs_node *file = vfs_get_node(vfs_root, "/bin/init.elf", true);

	uint8_t *init_binary = kmalloc(file->resource->stat.st_size);
	file->resource->read(file->resource, NULL, init_binary, 0,
						 file->resource->stat.st_size);

	kprintf("Running init binary /bin/init.elf\n");

	process_create_elf("init", PROCESS_READY_TO_RUN, 2000, init_binary,
					   prcb_return_current_cpu()->running_thread->mother_proc);
	for (;;)
		;
}
