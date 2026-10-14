use super::SyscallFn;
use crate::arch::Context;
use crate::log;
use crate::mm::virt;
use crate::mm::vmb;
use crate::object::handle;
use crate::sched;
use crate::sched::process;
use crate::sched::thread;
use crate::status_codes::PxStatus;

fn run<Args, F: SyscallFn<Args>>(f: F, ctx: &mut Context) {
    let status = f.call(ctx);
    ctx.set_ret(status as usize);
}

pub fn dispatch(context: &mut Context) {
    match context.get_syscall_nr() {
        super::SYSLOG => run(log::syscall_log, context),
        super::CLOSE => run(handle::syscall_close, context),
        super::WAIT_FOR_SINGLE_OBJECT => {
            run(sched::dispatch::syscall_wait_on_single_object, context)
        }
        super::WAIT_FOR_MULTIPLE_OBJECTS => {
            run(sched::dispatch::syscall_wait_on_multiple_objects, context)
        }
        super::NEW_THREAD => run(thread::syscall_new_thread, context),
        super::NEW_THREAD_GIVEN_CONTEXT => run(thread::syscall_new_thread_given_context, context),
        super::GET_THREAD_CONTEXT => run(thread::syscall_get_thread_context, context),
        super::SET_THREAD_CONTEXT => run(thread::syscall_set_thread_context, context),
        super::TERMINATE_THREAD => run(thread::syscall_terminate_thread, context),
        super::NEW_VMB => run(vmb::syscall_new_vmb, context),
        super::MAP_VMB => run(virt::syscall_map_vmb, context),
        super::UNMAP_VMB => run(virt::syscall_unmap_vmb, context),
        super::NEW_PROCESS => run(process::syscall_new_process, context),
        super::TERMINATE_PROCESS => run(process::syscall_terminate_process, context),
        super::CLONE => run(process::syscall_clone, context),
        n => {
            log!("Unknown syscall {n}");
            context.set_ret(PxStatus::InvalidArguments as usize);
        }
    }
}
