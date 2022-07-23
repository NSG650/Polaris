#include <debug/debug.h>
#include <fs/ramdisk.h>
#include <fs/testfs.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>
#include <sys/timer.h>

void syscall_is_computer_on(struct syscall_arguments *args) {
	args->ret = 0xFF;
}

void kernel_main(void *args) {
	vfs_install_fs(&testfs);

	struct fs_node *root_node = vfs_node_create(NULL, "root");
	vfs_node_mount(root_node, "/", "testfs");

	uint64_t *module_info = (uint64_t *)args;
	kprintf("Ramdisk located at 0x%p\n", module_info[0]);

	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	syscall_register_handler(1, syscall_is_computer_on);

	ramdisk_install(module_info[0], module_info[1]);
	struct fs_node *folder = vfs_path_to_node("/fun/hi.txt");
	struct file *file = NULL;
	for (int i = 0; i < folder->files.length; i++) {
		file = folder->files.data[i];
		if (!strcmp(file->name, "hi.txt"))
			break;
	}
	kprintf("/fun/hi.txt: %s", file->data);
	folder = vfs_path_to_node("/bin/program64.elf");
	file = NULL;
	for (int i = 0; i < folder->files.length; i++) {
		file = folder->files.data[i];
		if (!strcmp(file->name, "program64.elf"))
			break;
	}
	kprintf("Running init binary /bin/program64.elf\n");
	process_create_elf("init", PROCESS_READY_TO_RUN, 2000, file->data);
	for (;;)
		;
}
