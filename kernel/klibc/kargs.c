#include <klibc/kargs.h>
#include <mm/slab.h>

struct kernel_args kernel_arguments = {0};

void kargs_init(char *args) {
	char **tokens = NULL;
	int count = strsplit(args, ' ', &tokens);

	for (int i = 0; i < count; i++) {
		if (!strncmp(tokens[i], "cpus", 4)) {
			char *cpu_count = &tokens[i][5];
			kernel_arguments.kernel_args |= KERNEL_ARGS_CPU_COUNT_GIVEN;
			kernel_arguments.cpu_count = atol(cpu_count);
		}
		if (!strncmp(tokens[i], "no-lai", 6)) {
			kernel_arguments.kernel_args |= KERNEL_ARGS_NO_LAI;
		}
		if (!strncmp(tokens[i], "kprintf", 7)) {
			kernel_arguments.kernel_args |= KERNEL_ARGS_KPRINTF_LOGS;
		}
		if (!strncmp(tokens[i], "init", 4)) {
			kernel_arguments.kernel_args |= KERNEL_ARGS_INIT_PATH_GIVEN;
			char *init_path_from_args = &tokens[i][5];
			size_t length = strlen(init_path_from_args);
			kernel_arguments.init_binary_path = kmalloc(length + 1);
			strncpy(kernel_arguments.init_binary_path, init_path_from_args,
					length + 1);
		}
	}

	for (int i = 0; i < count; i++)
		kfree(tokens[i]);
	kfree(tokens);
}