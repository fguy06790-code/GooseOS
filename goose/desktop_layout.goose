// PROJECT: THYFOS desktop Goose port
// LICENSE: GPL-3.0-or-later
import std;
namespace desktop;

struct Rect { left: i32, top: i32, right: i32, bottom: i32 }
struct Layout {
    dock: Rect, launcher: Rect, panel: Rect, input: Rect, open_button: Rect,
    resources: Rect, audio: Rect, search: Rect, tasks: Rect[12], rows: Rect[5]
}

export fn thyfos_arrange(width: i32, height: i32, scale: f32,
                        task_count: i32, motion: f32, resources: bool, result: Layout&) {
    assert(width > 0 && height > 0 && scale > 0);
    assert(task_count >= 0 && task_count <= 12);
    fn s(value: i32) -> i32 { (value * scale + 0.5) as! i32 }
    let dock_height = s(76);
    let dock_width = min(s(240 + task_count * 54), width - s(40));
    result.dock = Rect { (width-dock_width)/2, height-dock_height-s(18), (width+dock_width)/2, height-s(18) };
    let d = result.dock;
    result.launcher = Rect { d.left+s(12), d.top+s(9), d.left+s(70), d.bottom-s(9) };
    for i: i32 in 0..task_count {
        result.tasks[i] = Rect { d.left+s(86+i*54), d.top+s(10), d.left+s(134+i*54), d.bottom-s(10) };
    }
    let shift = (s(24) * (1.0 - motion)) as! i32;
    let panel_top = max(s(80), d.top-s(510)) + shift;
    result.panel = Rect { s(24), panel_top, min(width-s(24),s(464)), d.top-s(16)+shift };
    let p = result.panel;
    result.input = Rect { p.left+s(20), p.top+s(24), p.right-s(20), p.top+s(72) };
    result.open_button = Rect { p.right-s(120), p.bottom-s(66), p.right-s(20), p.bottom-s(24) };
    let search_left = result.input.left+s(16);
    let search_top = result.input.top+s(12);
    result.search = Rect { search_left, search_top, search_left+result.input.right-result.input.left-s(32), search_top+s(26) };
    let row_top = result.input.bottom+s(82);
    let row_step = min(s(62),(result.open_button.top-s(12)-row_top)/5);
    for i: i32 in 0..5 {
        result.rows[i] = Rect { p.left+s(20),row_top+i*row_step,p.right-s(20),row_top+(i+1)*row_step-s(8) };
    }
    let card_width = min(s(420),width-s(48));
    result.resources = Rect { width-s(24)-card_width,d.top-s(174),width-s(24),d.top-s(18) };
    let audio_bottom = if resources { result.resources.top-s(14) } else { d.top-s(18) };
    result.audio = Rect { width-s(24)-card_width,audio_bottom-s(182),width-s(24),audio_bottom };
}
