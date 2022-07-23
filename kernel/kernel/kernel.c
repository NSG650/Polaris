#include <debug/debug.h>
#include <fs/testfs.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <sched/sched.h>
#include <sys/timer.h>
#include <sys/prcb.h>

void syscall_is_computer_on(struct syscall_arguments *args) {
	args->ret = 0xFF;
}

void kernel_thread(void) {
	kprintf("I am a kernel thread\n");
	thread_kill(prcb_return_current_cpu()->running_thread, true);
}

void kernel_main(void *args) {
	thread_create((uintptr_t)kernel_thread, 0, 0,
				  prcb_return_current_cpu()->running_thread->mother_proc);
	vfs_install_fs(&testfs);


	struct fs_node *root_node = vfs_node_create(NULL, "root");
	vfs_node_mount(root_node, "/", "testfs");
	
	kprintf("Creating file /fun/hi.txt\n");
	root_node->fs->mkdir(root_node, "fun");
	struct file *fun_folder_file = root_node->fs->open(root_node, "fun");
	struct fs_node *fun_folder = fun_folder_file->readdir(fun_folder_file);
	fun_folder->fs->create(fun_folder, "hi.txt");
	struct file *hi_dot_txt = fun_folder->fs->open(fun_folder, "hi.txt");
	kprintf("Writing 'Hello There!' in /fun/hi.txt\n");
	hi_dot_txt->write(hi_dot_txt, strlen("Hello There!"), 0,
					  (uint8_t *)"Hello There!");
	char *hi_dot_txt_data = kmalloc(hi_dot_txt->size);
	hi_dot_txt->read(hi_dot_txt, hi_dot_txt->size, 0,
					 (uint8_t *)hi_dot_txt_data);
	kprintf("/fun/hi.txt: %s\n", hi_dot_txt_data);

	kprintf("Creating folder /fun/even_more_fun\n");
	fun_folder->fs->mkdir(fun_folder, "even_more_fun");
	struct file *even_more_fun_file = fun_folder->fs->open(fun_folder, "even_more_fun");
	struct fs_node *even_more_fun = even_more_fun_file->readdir(even_more_fun_file);
	kprintf("Creating file /fun/even_more_fun/fun.txt\n");
	even_more_fun->fs->create(even_more_fun, "fun.txt");
	
	vfs_dump_fs_tree(root_node);

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
