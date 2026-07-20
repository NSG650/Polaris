use core::fmt::{Display, Formatter, FormattingOptions};

use crate::{fbcon, locks::spinlock::SpinLock};

pub struct DebugCon;

static LOG_LOCK: SpinLock<()> = SpinLock::new(());

pub fn log(msg: &dyn Display) {
    let _guard = LOG_LOCK.lock();
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
