#include <sched/sched.h>
#include <sched/crash.h>
#include <fb/fb.h>
#include <debug/debug.h>
#include <sys/prcb.h>
#include <sys/timer.h>

struct crash_message_handover {
    uintptr_t crash_addresss;
    struct process *proc;
    char *crash_reason;
};

struct crash_message_handover handover = {0};

static void sched_crash_message_thread(void) {
    if (!framebuff.address) thread_kill(prcb_return_current_cpu()->running_thread, 1);

    char *crash_message = kmalloc(4096);
    strcpy(crash_message, "\nAn user thread crashed under the process ");
    strcat(crash_message, handover.proc->name);
    strcat(crash_message, " because of a ");
    strcat(crash_message, handover.crash_reason);
    strcat(crash_message, ". It crashed at 0x");
    char *convert_me_to_hex = kmalloc(21);
    ultoa(handover.crash_addresss, convert_me_to_hex, 16);
    strcat(crash_message, convert_me_to_hex);
    strcat(crash_message, ".\n");
    framebuffer_puts(crash_message);
    framebuffer_puts("You can check the kernel logs for more information about the crash.\n");

    kfree(crash_message);
    kfree(convert_me_to_hex);
    memset(&handover, 0, sizeof(struct crash_message_handover));
    thread_kill(prcb_return_current_cpu()->running_thread, 1);
}

void sched_display_crash_message(uintptr_t crash_address,  struct process *proc, char *reason) {
    handover.crash_addresss = crash_address;
    handover.crash_reason = reason;
    handover.proc = proc;
    
    thread_create(sched_crash_message_thread, 0, 0, processes.data[0]);
    
}