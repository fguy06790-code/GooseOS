// PROJECT: THYFOS widget Goose port
// LICENSE: GPL-3.0-or-later
// Numeric policy is independent of the native counter and audio APIs.
namespace widgets;

fn percent(value: f64) -> f64 {
    if value < 0 { 0 } else if value > 100 { 100 } else { value }
}

export fn thyfos_cpu_percent(previous_total: u64, previous_idle: u64,
                            total: u64, idle: u64, primed: bool, output: f64&) -> bool {
    if !primed || total <= previous_total || idle < previous_idle { return false; }
    output = percent(100.0 * (1.0 - (idle-previous_idle) as f64 / (total-previous_total)));
    return true;
}

export fn thyfos_ram_percent(total: u64, available: u64, output: f64&) -> bool {
    if total == 0 || available > total { return false; }
    output = 100.0 * (total-available) / total;
    return true;
}

export fn thyfos_gpu_percent(running_delta: u64, clock_delta: u64, frequency: u64) -> f64 {
    if clock_delta == 0 || frequency == 0 { return 0; }
    let seconds = clock_delta as f64 / frequency;
    return percent(running_delta / (seconds * 10000000.0) * 100.0);
}

export fn thyfos_pcm_sample(data: const u8[:], bits: u32) -> f32 {
    if bits == 8 {
        assert(data.len >= 1);
        return (data[0] as i32 - 128) / 128.0;
    }
    assert(bits == 16 || bits == 24 || bits == 32);
    let count = bits / 8;
    assert(data.len >= count);
    var word: u32 = 0;
    for i: u32 in 0..count { word |= (data[i] as u32) << (i * 8); }
    let sign: u32 = 1 << (bits-1);
    let signed_value = if (word & sign) != 0 { word as i64 - (1 as i64 << bits) } else { word };
    // 32-bit PCM used double division before the final float rounding.
    if bits == 32 { return (signed_value / 2147483648.0) as f32; }
    let divisor: f32 = if bits == 16 { 32768.0 } else { 8388608.0 };
    return signed_value / divisor;
}

export fn thyfos_float_sample(value: f32) -> f32 {
    if value != value { 0 } else if value < -1 { -1 } else if value > 1 { 1 } else { value }
}

export fn thyfos_smooth_bands(current: f32[:], next: const f32[:], stale: bool) {
    assert(current.len >= 32);
    if stale {
        for i in 32 { current[i] *= 0.82; }
        return;
    }
    assert(next.len >= 32);
    for i in 32 {
        current[i] += (next[i]-current[i]) * (if next[i] > current[i] { 0.65 } else { 0.18 });
    }
}
