use crate::arch::x86_64::asm;
use limine::{BaseRevision, RequestsEndMarker, RequestsStartMarker, request::*};

use crate::log;

use crate::fbcon;
use flanterm;

#[used]
#[unsafe(link_section = ".requests_start")]
pub static REQUESTS_START: RequestsStartMarker = RequestsStartMarker::new();
#[unsafe(link_section = ".requests")]
pub static BASE_REVISION: BaseRevision = BaseRevision::new();
#[unsafe(link_section = ".requests")]
pub static STACK: StackSizeRequest = StackSizeRequest::new(65536);
#[unsafe(link_section = ".requests")]
static FRAMEBUFFER: FramebufferRequest = FramebufferRequest::new();
#[used]
#[unsafe(link_section = ".requests_end")]
pub static REQUESTS_END: RequestsEndMarker = RequestsEndMarker::new();

pub fn arch_entry() {
    if let Some(resp) = FRAMEBUFFER.response()
        && let Some(fb) = resp.framebuffers().first()
    {
        unsafe {
            fbcon::fbcon_init(
                fb.address(),
                fb.width as usize,
                fb.height as usize,
                fb.pitch as usize,
                fb.red_mask_size,
                fb.red_mask_shift,
                fb.green_mask_size,
                fb.green_mask_shift,
                fb.blue_mask_size,
                fb.blue_mask_shift,
                flanterm::fb::Rotation::Rot0,
            );
        }
    }

    log!("Hello x86_64!\r\n");
    loop {
        asm::halt_forever();
    }
}
