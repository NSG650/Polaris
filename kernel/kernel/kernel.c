#include <debug/debug.h>
#include <devices/tty/console.h>
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

const char *module_list[] = {
#if defined(__x86_64__)
	"/usr/lib/modules/ps2.ko",
	"/usr/lib/modules/i8254x.ko"
#endif
};

#define MODULE_LIST_SIZE (sizeof(module_list) / sizeof(module_list[0]))
#define ONE_SECOND (uint64_t)(1000 * 1000 * 1000)

#include "lwip/api.h"
#include <debug/printf.h>

extern struct utsname system_uname;
void http_server_thread(void) {
    struct netconn *listener = netconn_new(NETCONN_TCP);
    netconn_bind(listener, IP_ADDR_ANY, 80);
    netconn_listen(listener);

    kprintf("%s: listening on port 80\n", __FUNCTION__);

	char HTTP_RESPONSE[1024] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
	char HTML[512] = {0};
	snprintf_(HTML, 512, "<h1>It works!</h1><p>Hello from %s %s %s!</p>\r\n", system_uname.sysname, system_uname.release, system_uname.version);
	strcat(HTTP_RESPONSE, HTML);

	for (;;) {
        struct netconn *client;
        if (netconn_accept(listener, &client) != ERR_OK)
            continue;

        struct netbuf *buf;
        while (netconn_recv(client, &buf) == ERR_OK) {
            netbuf_delete(buf);
            break;
        }

        netconn_write(client, HTTP_RESPONSE, strlen(HTTP_RESPONSE), NETCONN_COPY);

        netconn_close(client);
        netconn_delete(client);
    }
}

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

	// Done so that gcc will stop REMOVING this function
	partition_enumerate(NULL, NULL);
	net_handle_packet_thread(NULL);

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
	syscall_register_handler(0x103, syscall_mknodat);
	syscall_register_handler(0x106, syscall_fstatat);
	syscall_register_handler(0x107, syscall_unlinkat);
	syscall_register_handler(0x109, syscall_linkat);
	syscall_register_handler(0x10b, syscall_readlinkat);
	syscall_register_handler(0x10c, syscall_fchmodat);
	syscall_register_handler(0x124, syscall_dup3);
	syscall_register_handler(0x125, syscall_pipe);
	syscall_register_handler(0xff, syscall_openpty);
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
	net_init();

	std_console_device =
		(vfs_get_node(vfs_root, "/dev/console", true))->resource;

	uint64_t mod_ret = 0;
	for (size_t i = 0; i < MODULE_LIST_SIZE; i++) {
		mod_ret = module_load(module_list[i]);
		if (mod_ret) {
			kprintf("Failed to load kernel module %s. Return value: %p\n",
					module_list[i], mod_ret);
		}
	}

	module_dump();

	thread_create((uintptr_t)http_server_thread, 0, false, kernel_proc);

	char *argv[] = {"init", NULL};
	char *envp[] = {"HOME=/", "TERM=linux", NULL, };
	char *init_path = "/usr/bin/init";
	if (kernel_arguments.kernel_args & KERNEL_ARGS_INIT_PATH_GIVEN) {
		init_path = kernel_arguments.init_binary_path;
	}

	kprintf("Running init binary %s\n", init_path);

	if (!process_run_init(init_path, argv, envp, sched_get_running_thread()->mother_proc)) {
		panic("Failed to run init binary!\n");
	}

	for (;;) {
		sched_yield(true);
	}
}
