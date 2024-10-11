
/// Assigns bits of an unsigned integer to given boolean l-values.  
/// Bit-order LSB first.
/// Use a underscore(`_`) to skip a bit.
macro_rules! from_byte {
    // Base cases, ignore underscores.
    (@impl $b:expr, _) => {};
    (@impl $b:expr, $var:expr) => { $var = ($b & 1) == 1; };

    // Recursive cases, an underscore or expression followed by stuff.
    // At each step, extract LSB and shift right to prepare for the next step.
    (@impl $b:expr, _, $($tt:tt)+) => {
        crate::macros::from_byte!(@impl ($b >> 1), $($tt)+)
    };
    (@impl $b:expr, $var:expr, $($tt:tt)+) => {
        crate::macros::from_byte!(@impl $b, $var);
        crate::macros::from_byte!(@impl ($b >> 1), $($tt)+)
    };

    // We first grab whatever we see and then match for either
    // an underscore or an expression one-by-one from left-to-right.
    ($byte:expr, $($tt:tt)+) => { crate::macros::from_byte!(@impl $byte, $($tt)*) };
}

/// Packs a series of bools or 0/1s into a u8.
/// Bit-order LSB first.  
/// Use a underscore(`_`) to skip a bit, it will be assigned 0.  
macro_rules! to_byte {
    ($b:expr, $($bs:expr),+) => {
        crate::macros::to_byte!($b)
        | (crate::macros::to_byte!($($bs),+) << 1)
    };

    (_) => { 0u8 };
    ($b:expr) => { ($b as u8 & 1) };
}

