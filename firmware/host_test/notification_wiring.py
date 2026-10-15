from pathlib import Path
import re
p = Path(__file__).resolve().parents[1] / 'main' / 'main.c'
s = p.read_text()
checks = {
    'queue advances while controls are hidden': 'notice_tick(&tank, dt, tank.ui_cover || setup_birth_due())' in s,
    'NEWS is independent of control visibility': re.search(r'else if \(notice_current\(\)\) view = .*"NEWS"', s),
    'notice pixels are independent of control visibility': re.search(r'if \(nt\) render_notice\(', s),
    'native scene cannot bypass a pending notice': '(g_ui349.controls_hidden||!notice_current())' not in s,
    'popup includes its bottom timer': '"NEWS", true, 92}' in s,
}
touch = (p.parent / 'touch_port_ft3168.c').read_text()
checks['tap uses screen-space feeding band'] = 's_press_world&&ui349_feed_zone(s_press_screen_y,false)' in touch
checks['drag uses screen-space feeding band'] = 's_press_world?ui349_feed_zone(s_press_screen_y,true):s_py<60' in touch
checks['fish and trimming retain priority'] = touch.index('if (s_trimmed) goto released') < touch.index('if (best >= 0)') < touch.index('s_press_world&&ui349_feed_zone')
for name, ok in checks.items(): print(('PASS' if ok else 'FAIL') + ': ' + name)
raise SystemExit(0 if all(checks.values()) else 1)

