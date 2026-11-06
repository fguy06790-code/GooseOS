/*
 * PROJECT:     THYFOS Desktop tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Spectrum silence, stereo phase and boundary regressions
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
import core from "../../goose";
import math;

fn assert_bounded(bands: const f32[:]) {
    for value in bands { assert(value >= 0 && value <= 1); }
}

fn main() {
    var left: f32[1024] = [0; 1024];
    var right: f32[1024] = [0; 1024];
    var bands: f32[32] = [-1; 32];

    spectrum::thyfos_spectrum(left, right, 48000, bands);
    for value in bands { assert(value == 0); }

    // With no rate, even empty inputs are valid and stale levels are cleared.
    let empty_left: f32[0] = [];
    let empty_right: f32[0] = [];
    bands = [1; 32];
    spectrum::thyfos_spectrum(empty_left, empty_right, 0, bands);
    for value in bands { assert(value == 0); }

    for i in 1024 {
        left[i] = (0.25 * sin(TAU * 1000.0 * i / 48000.0)) as f32;
        right[i] = left[i];
    }
    var same_phase: f32[32] = [0; 32];
    spectrum::thyfos_spectrum(left, right, 48000, same_phase);
    for i in 1024 { right[i] = -left[i]; }
    spectrum::thyfos_spectrum(left, right, 48000, bands);
    assert_bounded(bands);
    var peak_band = 0;
    for band in 32 {
        let difference = bands[band] - same_phase[band];
        assert(difference >= -0.000001 && difference <= 0.000001);
        if bands[band] > bands[peak_band] { peak_band = band; }
    }
    assert(peak_band == 16 && bands[peak_band] > 0.75);
    // Golden level from the original C++ Transform/ThyfosSpectrum routine.
    assert(bands[16] >= 0.78839953 && bands[16] <= 0.78840153);

    // A loud signal saturates instead of exceeding the UI's [0, 1] range.
    for i in 1024 { left[i] *= 16; right[i] = -left[i]; }
    spectrum::thyfos_spectrum(left, right, 48000, bands);
    assert_bounded(bands);
    assert(bands[16] == 1);

    // Exercise distinct channels, narrow Nyquist ranges, and high rates.
    var state: u32 = 1;
    for i in 1024 {
        state = state * 1664525 + 1013904223;
        left[i] = ((state % 20001) as i64 - 10000) as f32 / 10000;
        state = state * 1664525 + 1013904223;
        right[i] = ((state % 20001) as i64 - 10000) as f32 / 10000;
    }
    let rates: u32[6] = [1, 8000, 44100, 48000, 96000, 192000];
    for rate in rates {
        spectrum::thyfos_spectrum(left, right, rate, bands);
        assert_bounded(bands);
    }

    // A larger destination retains its trailing elements.
    var larger: f32[34] = [-1; 34];
    spectrum::thyfos_spectrum(left, right, 48000, larger);
    assert_bounded(larger[0..32]);
    assert(larger[32] == -1 && larger[33] == -1);
    print("Goose spectrum: silence, zero rate, opposite stereo phase, 1 kHz peak and bounded output passed");
}
