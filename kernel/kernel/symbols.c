#include "symbols.h"

sym_table_t symbol_table[] = {
	{0xffffffff80000000, "acpi_checksum"},
	{0xffffffff8000001b, "init_ec"},
	{0xffffffff800007c3, "ioapic_read"},
	{0xffffffff800007dc, "ioapic_write"},
	{0xffffffff800007f5, "get_gsi_count"},
	{0xffffffff8000080b, "get_ioapic_by_gsi"},
	{0xffffffff80000871, "lapic_read"},
	{0xffffffff800008b4, "lapic_write"},
	{0xffffffff800008f9, "lapic_set_nmi"},
	{0xffffffff80000dec, "wrxcr"},
	{0xffffffff80000dfc, "xsave"},
	{0xffffffff80000e07, "xrstor"},
	{0xffffffff80000e12, "fxsave"},
	{0xffffffff80000e16, "fxrstor"},
	{0xffffffff80000f2e, "cpu_init"},
	{0xffffffff800029ae, "pic_disable"},
	{0xffffffff80002bb3, "octal_to_int"},
	{0xffffffff80002d32, "devtmpfs_mount"},
	{0xffffffff80002d3a, "devtmpfs_populate"},
	{0xffffffff80002d40, "devtmpfs_mkdir"},
	{0xffffffff80002da5, "devtmpfs_open"},
	{0xffffffff80002f7f, "devtmpfs_close"},
	{0xffffffff80002f1b, "devtmpfs_read"},
	{0xffffffff80002e75, "devtmpfs_write"},
	{0xffffffff80002fe7, "tmpfs_populate"},
	{0xffffffff80002fed, "tmpfs_mkdir"},
	{0xffffffff8000305f, "tmpfs_open"},
	{0xffffffff8000327f, "tmpfs_close"},
	{0xffffffff8000321b, "tmpfs_read"},
	{0xffffffff80003175, "tmpfs_write"},
	{0xffffffff80003130, "tmpfs_mount"},
	{0xffffffff800032b1, "fstype2fs"},
	{0xffffffff800036df, "path2node"},
	{0xffffffff80004102, "lookup"},
	{0xffffffff80004884, "_out_buffer"},
	{0xffffffff8000488e, "_out_null"},
	{0xffffffff8000488f, "_out_fct"},
	{0xffffffff800048a8, "_atoi"},
	{0xffffffff800048d0, "_out_rev"},
	{0xffffffff80004989, "_ntoa_format"},
	{0xffffffff80004b16, "_ntoa_long"},
	{0xffffffff80004bec, "_ntoa_long_long"},
	{0xffffffff80004cc2, "_etoa"},
	{0xffffffff80005043, "_ftoa"},
	{0xffffffff80005436, "_vsnprintf"},
	{0xffffffff80005eb6, "_out_char"},
	{0xffffffff800061af, "twist"},
	{0xffffffff800063ad, "stub_close"},
	{0xffffffff800063b3, "stub_read"},
	{0xffffffff800063bb, "stub_write"},
	{0xffffffff800063c3, "stub_ioctl"},
	{0xffffffff80006958, "inner_alloc"},
	{0xffffffff80006b7d, "get_next_level"},
	{0xffffffff800070f3, "alloc_new_process"},
	{0xffffffff80007cfb, "is_transmit_empty"},
	{0xffffffff80007e0b, "bcdtobin"},
	{0xffffffff80007e1a, "is_updating"},
	{0xffffffff80007e3c, "read"},
	{0xffffffff80007e6d, "rtc_get_time"},
	{0xffffffff80007eb6, "rtc_get_date_time"},
	{0xffffffff800088c0, "make_pci_address"},
	{0xffffffff800088dd, "mcfg_pci_write"},
	{0xffffffff800089ed, "mcfg_pci_read"},
	{0xffffffff80008b20, "legacy_pci_write"},
	{0xffffffff80008bce, "legacy_pci_read"},
	{0xffffffff80009a98, "lai_parse_u32"},
	{0xffffffff80009ade, "lai_exec_pop_blkstack_back"},
	{0xffffffff80009b01, "lai_exec_pop_stack_back"},
	{0xffffffff80009b24, "lai_exec_push_ctxstack"},
	{0xffffffff80009b79, "lai_exec_push_blkstack"},
	{0xffffffff80009bd0, "lai_exec_push_stack"},
	{0xffffffff80009c02, "lai_exec_push_opstack"},
	{0xffffffff80009c61, "lai_parse_varint"},
	{0xffffffff80009d81, "lai_exec_commit_pc"},
	{0xffffffff80009dac, "lai_exec_reserve_ctxstack"},
	{0xffffffff80009e47, "lai_exec_reserve_blkstack"},
	{0xffffffff80009ee8, "lai_exec_reserve_stack"},
	{0xffffffff80009f94, "lai_parse_name"},
	{0xffffffff80009fb8, "lai_exec_reduce_node"},
	{0xffffffff8000a705, "lai_exec_reduce_op"},
	{0xffffffff8000e00e, "lai_cleanup_free_string"},
	{0xffffffff8000e039, "lai_exec_parse"},
	{0xffffffff800141eb, "lai_exec_process"},
	{0xffffffff8001615c, "lai_exec_run"},
	{0xffffffff80016ef9, "lai_read_buffer"},
	{0xffffffff80016f8c, "lai_write_buffer"},
	{0xffffffff80017de4, "lai_hash_string"},
	{0xffffffff80017e07, "lai_load_table"},
	{0xffffffff80017e8e, "lai_hashtable_grow"},
	{0xffffffff800197d0, "lai_object_type_of_objref"},
	{0xffffffff80019815, "lai_object_type_of_node"},
	{0xffffffff800198cb, "lai_clone_string"},
	{0xffffffff80019a4d, "lai_clone_buffer"},
	{0xffffffff8001aed7, "lai_clone_package"},
	{0xffffffff8001b32d, "lai_buffer_put_at"},
	{0xffffffff8001b39b, "lai_buffer_get_at"},
	{0xffffffff8001b412, "lai_calculate_access_width"},
	{0xffffffff8001b4ba, "lai_find_parent_root_of"},
	{0xffffffff8001b5a1, "lai_get_pci_params"},
	{0xffffffff8001b815, "lai_perform_read"},
	{0xffffffff8001bc2c, "lai_perform_write"},
	{0xffffffff8001c85f, "laihost_free_package"},
	{0xffffffff8001ca37, "is_digit"},
	{0xffffffff8001ca45, "num_fmt"},
	{0xffffffff8001cfad, "poll_ibf"},
	{0xffffffff8001cfcb, "poll_obf"},
	{0xffffffff8001cfe9, "disable_burst"},
	{0xffffffff8001d019, "enable_burst"},
	{0xffffffff8001d4a5, "readq"},
	{0xffffffff8001d591, "readd"},
	{0xffffffff8001d614, "readw"},
	{0xffffffff8001d66c, "readb"},
	{0xffffffff8001d745, "writeq"},
	{0xffffffff8001d82d, "writed"},
	{0xffffffff8001d8b0, "writew"},
	{0xffffffff8001d905, "writeb"},
	{0xffffffff8001db5e, "lai_bios_calc_checksum"},
	{0xffffffff8001ef0b, "lai_get_header_info"},
	{0xffffffff8001fd16, "liballoc_memset"},
	{0xffffffff8001fd30, "liballoc_memcpy"},
	{0xffffffff8001fd6b, "allocate_new_page"},
	{0xffffffff800180ad, "lai_uninstall_nsnode"},
	{0xffffffff80007939, "running_thrd"},
	{0xffffffff80016d89, "lai_eval_vargs"},
	{0xffffffff8001c53d, "lai_write_indexfield"},
	{0xffffffff80006515, "strcpy"},
	{0xffffffff8001d6a0, "lai_write_ec"},
	{0xffffffff80007db6, "write_serial"},
	{0xffffffff80009669, "_putchar"},
	{0xffffffff80020386, "krealloc"},
	{0xffffffff80000d0a, "apic_init"},
	{0xffffffff800011a4, "smp_init"},
	{0xffffffff80000532, "laihost_sleep"},
	{0xffffffff800098b5, "bswap16"},
	{0xffffffff80000375, "laihost_unmap"},
	{0xffffffff80009996, "lai_eisaid"},
	{0xffffffff8000418b, "alloc"},
	{0xffffffff80003390, "vfs_get_absolute_path"},
	{0xffffffff80007c24, "thread_exit"},
	{0xffffffff80019672, "lai_ns_child_iterate"},
	{0xffffffff800179f6, "lai_exec_get_integer"},
	{0xffffffff8000988a, "lai_is_name"},
	{0xffffffff800179ce, "lai_exec_get_objectref"},
	{0xffffffff80000c7f, "apic_timer_init"},
	{0xffffffff800098e9, "bswap64"},
	{0xffffffff8001619f, "lai_init_state"},
	{0xffffffff80008878, "hpet_counter_value"},
	{0xffffffff80002a9e, "port_dword_out"},
	{0xffffffff80019aed, "lai_create_pkg"},
	{0xffffffff80000544, "laihost_timer"},
	{0xffffffff800060c5, "vprintf_"},
	{0xffffffff80017bf4, "lai_panic"},
	{0xffffffff800046da, "memmove"},
	{0xffffffff8001f520, "lai_resource_irq_is_active_low"},
	{0xffffffff80016edb, "lai_enable_tracing"},
	{0xffffffff80017cc4, "lai_calloc"},
	{0xffffffff80003ce0, "stivale2_get_tag"},
	{0xffffffff8000056b, "init_madt"},
	{0xffffffff80009970, "char_to_hex"},
	{0xffffffff800172a0, "lai_exec_ref_store"},
	{0xffffffff8000425f, "liballoc_unlock"},
	{0xffffffff8001a93e, "lai_obj_to_buffer"},
	{0xffffffff800004ee, "laihost_pci_readw"},
	{0xffffffff800004cc, "laihost_pci_readb"},
	{0xffffffff8001c760, "lai_var_finalize"},
	{0xffffffff8001704b, "lai_exec_string_length"},
	{0xffffffff8000299f, "isr_register_handler"},
	{0xffffffff80018d72, "lai_do_resolve_new_node"},
	{0xffffffff80004167, "symbols_return_function_name"},
	{0xffffffff80018053, "lai_current_instance"},
	{0xffffffff8001f80c, "lai_disable_acpi"},
	{0xffffffff8001acf0, "lai_obj_to_integer"},
	{0xffffffff80007043, "vmm_page_fault_handler"},
	{0xffffffff800068ec, "vec_swapsplice_"},
	{0xffffffff80006ced, "vmm_init"},
	{0xffffffff80019b84, "lai_obj_resize_string"},
	{0xffffffff80017259, "lai_store_ns"},
	{0xffffffff80000376, "laihost_scan"},
	{0xffffffff80017b3a, "lai_warn"},
	{0xffffffff8001805b, "lai_create_nsnode"},
	{0xffffffff8000426d, "memcpy"},
	{0xffffffff8001708e, "lai_exec_pkg_var_store"},
	{0xffffffff8001f169, "lai_resource_iterate"},
	{0xffffffff8001c731, "lai_do_rev_method"},
	{0xffffffff80017a80, "lai_debug"},
	{0xffffffff8001a163, "lai_obj_to_string"},
	{0xffffffff800201f1, "kfree"},
	{0xffffffff800012bd, "set_idt"},
	{0xffffffff800087af, "gdt_load_tss"},
	{0xffffffff80000146, "acpi_init"},
	{0xffffffff80007d8f, "write_serial_char"},
	{0xffffffff80002a9a, "port_dword_in"},
	{0xffffffff80006bb3, "vmm_switch_pagemap"},
	{0xffffffff80019838, "lai_create_string"},
	{0xffffffff80008887, "hpet_usleep"},
	{0xffffffff80007542, "process_sleep"},
	{0xffffffff8001c920, "lai_var_move"},
	{0xffffffff80005f9c, "sprintf_"},
	{0xffffffff8000352e, "vfs_new_node"},
	{0xffffffff80000a1e, "lapic_init"},
	{0xffffffff800075b3, "process_wait"},
	{0xffffffff8001ef69, "lai_read_resource"},
	{0xffffffff80006adb, "pmm_alloc"},
	{0xffffffff800067e4, "vec_compact_"},
	{0xffffffff8001995b, "lai_create_c_string"},
	{0xffffffff800199be, "lai_create_buffer"},
	{0xffffffff8001edeb, "lai_acpi_reset"},
	{0xffffffff8000874e, "gdt_init"},
	{0xffffffff80019427, "lai_resolve_search"},
	{0xffffffff800035fc, "vfs_mkdir"},
	{0xffffffff800183e7, "lai_ns_get_child"},
	{0xffffffff800187a8, "lai_stringify_node_path"},
	{0xffffffff800094d5, "putchar_at"},
	{0xffffffff8001f8e8, "lai_evaluate_sta"},
	{0xffffffff80007a6a, "thread_init"},
	{0xffffffff800163e1, "lai_populate"},
	{0xffffffff8001f9da, "lai_init_children"},
	{0xffffffff8000691d, "vec_swap_"},
	{0xffffffff80016219, "lai_finalize_state"},
	{0xffffffff80000bc9, "ioapic_redirect_irq"},
	{0xffffffff8001c4c1, "lai_read_indexfield"},
	{0xffffffff8001c62f, "lai_do_osi_method"},
	{0xffffffff80000452, "laihost_pci_writeb"},
	{0xffffffff80000c20, "apic_send_ipi"},
	{0xffffffff80000346, "laihost_malloc"},
	{0xffffffff8001673f, "lai_eval_args"},
	{0xffffffff8001d9cd, "lai_read_pm_timer_value"},
	{0xffffffff80006bba, "vmm_new_pagemap"},
	{0xffffffff800060ec, "vsnprintf_"},
	{0xffffffff80007d11, "serial_install"},
	{0xffffffff80004235, "liballoc_lock"},
	{0xffffffff80007fb0, "get_unix_timestamp"},
	{0xffffffff80019cb1, "lai_obj_resize_pkg"},
	{0xffffffff800068b9, "vec_splice_"},
	{0xffffffff800196a4, "lai_ns_override_notify"},
	{0xffffffff80007c14, "thread_unblock"},
	{0xffffffff80019e3c, "lai_obj_get_integer"},
	{0xffffffff80018689, "lai_stringify_amlname"},
	{0xffffffff80003cf8, "kernel_main"},
	{0xffffffff800081e1, "load_elf"},
	{0xffffffff80017180, "lai_operand_load"},
	{0xffffffff80006b5b, "pmm_free"},
	{0xffffffff8001c32b, "lai_read_field"},
	{0xffffffff8000610a, "fctprintf"},
	{0xffffffff8000805f, "dump_section_table"},
	{0xffffffff8001d06a, "lai_early_init_ec"},
	{0xffffffff80007478, "process_exit"},
	{0xffffffff8001730d, "lai_exec_mutate_ns"},
	{0xffffffff8001afa8, "lai_objecttype_ns"},
	{0xffffffff80007c93, "thread_sleep"},
	{0xffffffff80006707, "vec_expand_"},
	{0xffffffff800098c4, "bswap32"},
	{0xffffffff80007bee, "thread_block"},
	{0xffffffff80016e46, "lai_eval_largs"},
	{0xffffffff80000510, "laihost_pci_readd"},
	{0xffffffff800047bd, "memchr"},
	{0xffffffff8000047c, "laihost_pci_writew"},
	{0xffffffff8000973b, "kprint"},
	{0xffffffff800071bd, "process_create"},
	{0xffffffff800192c9, "lai_resolve_path"},
	{0xffffffff80003efc, "_start"},
	{0xffffffff8001dc5c, "lai_bios_detect_rsdp"},
	{0xffffffff80008c6c, "pci_init"},
	{0xffffffff80006317, "rand"},
	{0xffffffff80007462, "process_unblock"},
	{0xffffffff80018bd9, "lai_do_resolve"},
	{0xffffffff800194a7, "lai_check_device_pnp_id"},
	{0xffffffff8001f490, "lai_resource_irq_is_level_triggered"},
	{0xffffffff80017d32, "lai_strcmp"},
	{0xffffffff8001da0a, "lai_start_pm_timer"},
	{0xffffffff80006488, "strncmp"},
	{0xffffffff80004200, "liballoc_alloc"},
	{0xffffffff80003a10, "vfs_mount"},
	{0xffffffff800012dc, "isr_install"},
	{0xffffffff8000127d, "return_installed_cpus"},
	{0xffffffff80006583, "strncpy"},
	{0xffffffff80018544, "lai_amlname_parse"},
	{0xffffffff8001b00e, "lai_obj_exec_match_op"},
	{0xffffffff8001a2af, "lai_mutate_string"},
	{0xffffffff8000392e, "vfs_open"},
	{0xffffffff8000401d, "panic"},
	{0xffffffff8001cb02, "lai_vsnprintf"},
	{0xffffffff800096d2, "kprintbgc"},
	{0xffffffff800188a1, "lai_install_nsnode"},
	{0xffffffff80004856, "memcmp"},
	{0xffffffff8000939a, "knewline"},
	{0xffffffff80006b1e, "pmm_allocz"},
	{0xffffffff800183e2, "lai_ns_get_parent"},
	{0xffffffff800076da, "process_kill"},
	{0xffffffff80002acb, "syscall_init"},
	{0xffffffff80000e37, "wsmp_cpu_init"},
	{0xffffffff80017d65, "lai_snprintf"},
	{0xffffffff80018638, "lai_amlname_done"},
	{0xffffffff800003f7, "laihost_outb"},
	{0xffffffff80009370, "draw_px"},
	{0xffffffff8001db79, "lai_bios_detect_rsdp_within"},
	{0xffffffff80007279, "process_init"},
	{0xffffffff800045fd, "memset"},
	{0xffffffff8001c436, "lai_write_field"},
	{0xffffffff80009638, "putchar_color"},
	{0xffffffff800062c4, "srand"},
	{0xffffffff8001962f, "lai_ns_iterate"},
	{0xffffffff8000041a, "laihost_outd"},
	{0xffffffff80003315, "vfs_install_fs"},
	{0xffffffff800197b0, "lai_ns_get_opregion_address_space"},
	{0xffffffff8001f6b9, "lai_get_sci_event"},
	{0xffffffff80002a84, "port_byte_in"},
	{0xffffffff8001af47, "lai_objecttype_obj"},
	{0xffffffff8000032c, "laihost_panic"},
	{0xffffffff80002a93, "port_word_out"},
	{0xffffffff80019789, "lai_set_acpi_revision"},
	{0xffffffff800079b1, "alloc_new_thread"},
	{0xffffffff8001a898, "lai_obj_clone"},
	{0xffffffff80002919, "isr_handler"},
	{0xffffffff80009694, "kprint_color"},
	{0xffffffff8001ab85, "lai_obj_to_hex_string"},
	{0xffffffff800067c0, "vec_reserve_po2_"},
	{0xffffffff8001f5ae, "lai_resource_next_irq"},
	{0xffffffff80006467, "strcmp"},
	{0xffffffff80008d9d, "pci_write"},
	{0xffffffff8001d946, "lai_query_ec"},
	{0xffffffff80007341, "process_fork"},
	{0xffffffff800038f6, "vfs_new_node_deep"},
	{0xffffffff80007436, "process_block"},
	{0xffffffff80018091, "lai_create_nsnode_or_die"},
	{0xffffffff80018647, "lai_amlname_iterate"},
	{0xffffffff80019f3e, "lai_mutate_buffer"},
	{0xffffffff80017cf7, "lai_strlen"},
	{0xffffffff80002b4b, "dev_new_id"},
	{0xffffffff80018ed3, "lai_resolve_new_node"},
	{0xffffffff800029d2, "pic_init"},
	{0xffffffff800087ec, "hpet_init"},
	{0xffffffff8001dd3a, "lai_pci_parse_prt"},
	{0xffffffff8001daee, "lai_busy_wait_pm_timer"},
	{0xffffffff80000c6a, "apic_eoi"},
	{0xffffffff80000563, "acpi_get_lapic"},
	{0xffffffff80006415, "resource_create"},
	{0xffffffff800069c3, "pmm_init"},
	{0xffffffff80006036, "snprintf_"},
	{0xffffffff8000625f, "get_rdseed"},
	{0xffffffff80002a88, "port_byte_out"},
	{0xffffffff80000b19, "ioapic_redirect_gsi"},
	{0xffffffff8001e3c3, "lai_pci_find_bus"},
	{0xffffffff80019ea7, "lai_obj_get_handle"},
	{0xffffffff8001d40e, "lai_read_ec"},
	{0xffffffff8000096b, "timer_interrupt"},
	{0xffffffff80004218, "liballoc_free"},
	{0xffffffff8001e2ad, "lai_pci_find_device"},
	{0xffffffff80000e28, "wrmsr"},
	{0xffffffff80009750, "draw_rect"},
	{0xffffffff8000035c, "laihost_free"},
	{0xffffffff800170ab, "lai_exec_access"},
	{0xffffffff80018f18, "lai_create_root"},
	{0xffffffff8001775e, "lai_operand_emplace"},
	{0xffffffff80019712, "lai_ns_get_node_type"},
	{0xffffffff80008dc7, "ssfn_utf8"},
	{0xffffffff80016ec5, "lai_eval"},
	{0xffffffff80017074, "lai_exec_pkg_var_load"},
	{0xffffffff8001e950, "lai_enter_sleep"},
	{0xffffffff8001e8d9, "lai_pci_route"},
	{0xffffffff80005ecb, "printf_"},
	{0xffffffff80008e77, "ssfn_putc"},
	{0xffffffff80003b7a, "vfs_dump_nodes"},
	{0xffffffff8001c18c, "lai_write_field_internal"},
	{0xffffffff80017113, "lai_exec_ref_load"},
	{0xffffffff8001a4b7, "lai_obj_to_type_string"},
	{0xffffffff8001a723, "lai_mutate_integer"},
	{0xffffffff80000444, "laihost_ind"},
	{0xffffffff80002a8e, "port_word_in"},
	{0xffffffff800092e4, "vid_reset"},
	{0xffffffff8001facb, "lai_enable_acpi"},
	{0xffffffff800004a6, "laihost_pci_writed"},
	{0xffffffff80019d9a, "lai_obj_get_type"},
	{0xffffffff80001285, "set_idt_gate"},
	{0xffffffff8000676c, "vec_reserve_"},
	{0xffffffff8001fde0, "kmalloc"},
	{0xffffffff80008d7c, "pci_read"},
	{0xffffffff8001c5f7, "lai_write_opregion"},
	{0xffffffff80007917, "running_proc"},
	{0xffffffff80019c17, "lai_obj_resize_buffer"},
	{0xffffffff80000351, "laihost_realloc"},
	{0xffffffff80002aa4, "syscall_handler"},
	{0xffffffff80003c2f, "vfs_stat"},
	{0xffffffff800196cc, "lai_ns_override_opregion"},
	{0xffffffff8001e6ab, "lai_pci_route_pin"},
	{0xffffffff8001c07c, "lai_read_field_internal"},
	{0xffffffff80007769, "sched_init"},
	{0xffffffff8001dac2, "lai_stop_pm_timer"},
	{0xffffffff80006be6, "vmm_map_page"},
	{0xffffffff80019130, "lai_create_namespace"},
	{0xffffffff800064c5, "strlen"},
	{0xffffffff80009813, "lai_api_error_to_string"},
	{0xffffffff80002bd5, "initramfs_init"},
	{0xffffffff800002e5, "laihost_log"},
	{0xffffffff800174ee, "lai_operand_mutate"},
	{0xffffffff80000428, "laihost_inb"},
	{0xffffffff8001aa25, "lai_obj_to_decimal_string"},
	{0xffffffff800066e6, "strchr"},
	{0xffffffff8000684f, "vec_insert_"},
	{0xffffffff80007b21, "thread_create"},
	{0xffffffff8000094f, "sci_interrupt"},
	{0xffffffff80019e6e, "lai_obj_get_pkg"},
	{0xffffffff8001f767, "lai_set_sci_event"},
	{0xffffffff80002fb1, "devtmpfs_add_device"},
	{0xffffffff800092f9, "video_init"},
	{0xffffffff8001c6ea, "lai_do_os_method"},
	{0xffffffff8001f3a6, "lai_resource_get_type"},
	{0xffffffff80020356, "kcalloc"},
	{0xffffffff800097b2, "clear_screen"},
	{0xffffffff80017d15, "lai_strcpy"},
	{0xffffffff8000795b, "yield_to_scheduler"},
	{0xffffffff80006638, "strchrnul"},
	{0xffffffff80000e1a, "rdmsr"},
	{0xffffffff800183cf, "lai_ns_get_root"},
	{0xffffffff80000210, "acpi_find_sdt"},
	{0xffffffff80000436, "laihost_inw"},
	{0xffffffff80000409, "laihost_outw"},
	{0xffffffff8001d10c, "lai_init_ec"},
	{0xffffffff80009654, "putcharx"},
	{0xffffffff80002b88, "dev_add_new"},
	{0xffffffff80000367, "laihost_map"},
	{0xffffffff800041dd, "free"},
	{0xffffffff8001c8b7, "lai_swap_object"},
	{0xffffffff8001c5bc, "lai_read_opregion"},
	{0xffffffff8001c986, "lai_var_assign"},
	{0xffffffff, ""}};

static sym_table_t lookup(uint64_t address) {
	size_t i;
	for (i = 0; symbol_table[i].address != 0xffffffff; i++)
		if ((symbol_table[i].address << 52) == (address << 52))
			return symbol_table[i];
	return symbol_table[i];
}

const char *symbols_return_function_name(uint64_t address) {
	sym_table_t table = lookup(address);
	if (table.address == 0xffffffff)
		return "UNKNOWN";
	return table.function_name;
}