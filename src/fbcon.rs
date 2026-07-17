use flanterm::fb::{FlantermFb, Rotation};

pub static mut FLANTERM_CTX: Option<FlantermFb> = None;

pub unsafe fn fbcon_init(
    framebuffer: *mut (),
    width: usize,
    height: usize,
    pitch: usize,
    red_mask_size: u8,
    red_mask_shift: u8,
    green_mask_size: u8,
    green_mask_shift: u8,
    blue_mask_size: u8,
    blue_mask_shift: u8,
    rotation: Rotation,
) {
    unsafe {
        FLANTERM_CTX = FlantermFb::new(
            &mut *core::ptr::slice_from_raw_parts_mut(framebuffer as *mut u32, pitch * height),
            width,
            height,
            pitch,
            red_mask_size,
            red_mask_shift,
            green_mask_size,
            green_mask_shift,
            blue_mask_size,
            blue_mask_shift,
            None,
            0,
            0,
            None,
            None,
            None,
            None,
            None,
            None,
            None,
            0,
            rotation,
        );
    }
}
