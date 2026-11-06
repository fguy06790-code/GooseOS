/*
 * PROJECT:     THYFOS Desktop Goose port
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Hann-windowed stereo audio spectrum
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
import math;
namespace spectrum;

let SAMPLE_COUNT = 1024;
let BAND_COUNT = 32;

// The two channels are transformed independently. Combining their power,
// rather than their samples, preserves audio with opposite stereo phase.
fn transform(input: const f32[:], real: f64[:], imaginary: f64[:]) {
    for i in SAMPLE_COUNT {
        real[i] = input[i] * (0.5 - 0.5 * cos(TAU * i / (SAMPLE_COUNT - 1)));
        imaginary[i] = 0;
    }

    var reversed = 0;
    for i in 1..SAMPLE_COUNT {
        var bit = SAMPLE_COUNT >> 1;
        while (reversed & bit) != 0 {
            reversed ^= bit;
            bit >>= 1;
        }
        reversed ^= bit;
        if i < reversed {
            let temporary = real[i];
            real[i] = real[reversed];
            real[reversed] = temporary;
        }
    }

    var length = 2;
    while length <= SAMPLE_COUNT {
        let angle = -TAU / length;
        let step_real = cos(angle);
        let step_imaginary = sin(angle);
        var start = 0;
        while start < SAMPLE_COUNT {
            var rotation_real = 1.0;
            var rotation_imaginary = 0.0;
            for offset in length / 2 {
                let a = start + offset;
                let b = a + length / 2;
                let b_real = real[b] * rotation_real - imaginary[b] * rotation_imaginary;
                let b_imaginary = real[b] * rotation_imaginary + imaginary[b] * rotation_real;
                real[b] = real[a] - b_real;
                imaginary[b] = imaginary[a] - b_imaginary;
                real[a] += b_real;
                imaginary[a] += b_imaginary;
                let next = rotation_real * step_real - rotation_imaginary * step_imaginary;
                rotation_imaginary = rotation_real * step_imaginary + rotation_imaginary * step_real;
                rotation_real = next;
            }
            start += length;
        }
        length <<= 1;
    }
}

// Process the first 1024 samples into the first 32 output elements. A zero
// sample rate clears the output without reading either input slice.
export fn thyfos_spectrum(left: const f32[:], right: const f32[:], rate: u32, bands: f32[:]) {
    assert(bands.len >= BAND_COUNT);
    if rate == 0 {
        for band in BAND_COUNT { bands[band] = 0; }
        return;
    }
    assert(left.len >= SAMPLE_COUNT && right.len >= SAMPLE_COUNT);

    var left_real: f64[SAMPLE_COUNT] = [0; SAMPLE_COUNT];
    var left_imaginary: f64[SAMPLE_COUNT] = [0; SAMPLE_COUNT];
    var right_real: f64[SAMPLE_COUNT] = [0; SAMPLE_COUNT];
    var right_imaginary: f64[SAMPLE_COUNT] = [0; SAMPLE_COUNT];
    transform(left, left_real, left_imaginary);
    transform(right, right_real, right_imaginary);

    var high = rate / 2.0;
    if high > 16000 { high = 16000; }
    for band in BAND_COUNT {
        // Truncation selects FFT bins, as in the original C++ implementation.
        var first = (60 * pow(high / 60.0, band as f64 / BAND_COUNT) * SAMPLE_COUNT / rate) as! i64;
        var last = (60 * pow(high / 60.0, (band + 1) as f64 / BAND_COUNT) * SAMPLE_COUNT / rate) as! i64;
        if first < 1 { first = 1; }
        if last <= first { last = first + 1; }
        if last > SAMPLE_COUNT / 2 { last = SAMPLE_COUNT / 2; }

        var peak = 0.0;
        var bin = first;
        while bin < last {
            let power = (left_real[bin] * left_real[bin] + left_imaginary[bin] * left_imaginary[bin] +
                         right_real[bin] * right_real[bin] + right_imaginary[bin] * right_imaginary[bin]) / 2;
            if power > peak { peak = power; }
            bin += 1;
        }
        let decibels = 20 * log10(sqrt(peak) / (SAMPLE_COUNT / 2) + 1e-7) + 6;
        let level = (decibels + 60) / 60;
        bands[band] = (if level < 0 { 0.0 } else if level > 1 { 1.0 } else { level }) as f32;
    }
}
