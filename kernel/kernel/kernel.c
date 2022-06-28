#include <debug/debug.h>
#include <fs/testfs.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/prcb.h>

void syscall_is_computer_on(struct syscall_arguments *args) {
	args->ret = 0xFF;
}

void kernel_main(void *args) {
	vfs_install_fs(&testfs);
	struct fs_node *root_node = vfs_node_create(NULL, "root");
	vfs_node_mount(root_node, "/", "testfs");
	kprintf("Creating file hi.txt\n");
	root_node->fs->create(root_node, "hi.txt");
	struct file *hi_dot_txt = root_node->fs->open(root_node, "hi.txt");
	kprintf("Writing 'Hello There!' in hi.txt\n");
	hi_dot_txt->write(hi_dot_txt, strlen("Hello There!"), 0,
					  (uint8_t *)"Hello There!");
	char *hi_dot_txt_data = kmalloc(hi_dot_txt->size);
	hi_dot_txt->read(hi_dot_txt, hi_dot_txt->size, 0,
					 (uint8_t *)hi_dot_txt_data);
	kprintf("/hi.txt: %s\n", hi_dot_txt_data);
	kfree(hi_dot_txt_data);
	uint64_t *module_info = (uint64_t *)args;
	kprintf("ELF binary located at 0x%p\n", module_info[0]);
	kprintf("Hello I am %s\n",
			prcb_return_current_cpu()->running_thread->mother_proc->name);
	syscall_register_handler(1, syscall_is_computer_on);
	kprintf("Creating elf user process\n");
	process_create_elf("init", 0, 2000, (uint8_t *)module_info[0]);
	for (;;)
		;
}
