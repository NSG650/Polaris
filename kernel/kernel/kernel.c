#include <debug/debug.h>
#include <fs/ramdisk.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

void kernel_main(void *args) {
	vfs_install_fs(&tmpfs);

	struct fs_node *root_node = vfs_node_create(NULL, "root");
	vfs_node_mount(root_node, "/", "tmpfs");

	uint64_t *module_info = (uint64_t *)args;
	kprintf("Ramdisk located at 0x%p\n", module_info[0]);

	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	ramdisk_install(module_info[0], module_info[1]);

	syscall_register_handler(0x0, syscall_read);
	syscall_register_handler(0x1, syscall_write);
	syscall_register_handler(0x2, syscall_open);

	struct file *file = vfs_open_file("/bin/init.elf", 0x0);

	if (!file)
		panic("No init found!\n");

	struct stat *file_stat = file->stat(file);
	uint8_t *init_binary = kmalloc(file_stat->st_size);
	file->read(file, init_binary, 0, file_stat->st_size);

	kprintf("Running init binary /bin/init.elf\n");
	process_create_elf("init", PROCESS_READY_TO_RUN, 2000, init_binary,
					   prcb_return_current_cpu()->running_thread->mother_proc);
	for (;;)
		;
}
