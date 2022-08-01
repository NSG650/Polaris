#include <debug/debug.h>
#include <fs/ramdisk.h>
#include <fs/testfs.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

void kernel_main(void *args) {
	vfs_install_fs(&testfs);

	struct fs_node *root_node = vfs_node_create(NULL, "root");
	vfs_node_mount(root_node, "/", "testfs");

	uint64_t *module_info = (uint64_t *)args;
	kprintf("Ramdisk located at 0x%p\n", module_info[0]);

	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	ramdisk_install(module_info[0], module_info[1]);

	syscall_register_handler(0x0, syscall_read);
	syscall_register_handler(0x2, syscall_open);

	/*
		We shouldn't directly read the file data but instead should call the
		file system to read the file. This data can vary depending on the fs or
		the device it's stored on. For example this data can contain info about
		the sector or block address for the particular file. So this should be
		avoided.
	*/

	struct file *file = vfs_open_file("/bin/program64.elf");

	if (!file)
		panic("No init found!\n");

	kprintf("Running init binary /bin/program64.elf\n");
	process_create_elf("init", PROCESS_READY_TO_RUN, 2000, file->data);
	for (;;)
		;
}
