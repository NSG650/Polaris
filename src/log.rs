use core::fmt::{Display, Formatter, FormattingOptions};

use crate::fbcon;

pub struct DebugCon;

pub fn log(msg: &dyn Display) {
    unsafe {
        if let Some(fb) = &mut *&raw mut fbcon::FLANTERM_CTX {
            let mut fmt = Formatter::new(&mut ***fb, FormattingOptions::new());
            let _ = msg.fmt(&mut fmt);
        }
    }

    let mut con = DebugCon;
    let mut fmt = Formatter::new(&mut con, FormattingOptions::new());
    let _ = msg.fmt(&mut fmt);
}

#[macro_export]
macro_rules! log {
    ($($args: expr),+ $(,)?) => {
        $crate::log::log(&format_args!($($args),+))
    };
}
