"""Sơ đồ cắm chân phần cứng: Blue Pill (nhìn từ trên, cổng USB ở trên) và các module.
Chạy: python3 docs/src/wiring.py  ->  docs/wiring.png
"""
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle

plt.rcParams['font.family'] = 'DejaVu Sans'
OUT = os.path.join(os.path.dirname(__file__), '..', 'wiring.png')

# Thứ tự chân Blue Pill (USB ở trên), từ trên xuống
LEFT = ['B12', 'B13', 'B14', 'B15', 'A8', 'A9', 'A10', 'A11', 'A12', 'A15', 'B3', 'B4', 'B5', 'B6', 'B7', 'B8', 'B9', '5V', 'G', '3V3']
RIGHT = ['G', 'G', '3V3', 'R', 'B11', 'B10', 'B1', 'B0', 'A7', 'A6', 'A5', 'A4', 'A3', 'A2', 'A1', 'A0', 'C15', 'C14', 'C13', 'VB']
USED = {'A9', 'A10', 'B6', 'B0', 'B12', 'B13', 'B10', 'B11', 'A7', 'A5', 'A4', 'A1', 'A0', '5V', 'G', '3V3'}

BX0, BX1, TOP, STEP = 6.9, 9.3, 10.0, 0.42
NET = {'5V': '#d62728', '3V3': '#ff7f0e', 'GND': '#222222'}

fig, ax = plt.subplots(figsize=(17, 12.8))
ax.set_xlim(-0.2, 17.2); ax.set_ylim(-0.8, 13.0); ax.axis('off')


def pin_y(i):
    return TOP - STEP * i


def board_pin(name, side):
    lst = LEFT if side == 'L' else RIGHT
    i = lst.index(name)
    return ((BX0 if side == 'L' else BX1), pin_y(i))


# ---------------- Blue Pill ----------------
ax.add_patch(FancyBboxPatch((BX0, pin_y(19) - 0.35), BX1 - BX0, pin_y(0) - pin_y(19) + 0.9,
                            boxstyle='round,pad=0,rounding_size=0.12', fc='#1f5fa8', ec='black', lw=1.5))
ax.add_patch(Rectangle((7.7, TOP + 0.3), 0.8, 0.35, fc='#c0c0c0', ec='black'))
ax.text(8.1, TOP + 0.42, 'USB', ha='center', va='center', fontsize=8)
ax.text(8.1, 6.2, 'BLUE\nPILL', ha='center', va='center', color='white', fontsize=12, fontweight='bold')
ax.text(8.1, 5.3, 'STM32\nF103C8', ha='center', color='white', fontsize=8.5)
for side, lst in (('L', LEFT), ('R', RIGHT)):
    for i, n in enumerate(lst):
        x = BX0 + 0.18 if side == 'L' else BX1 - 0.18
        used = n in USED
        ax.add_patch(plt.Circle((x, pin_y(i)), 0.1, fc='#ffd400' if used else '#dddddd', ec='black', lw=0.6))
        ax.text(x + (0.2 if side == 'L' else -0.2), pin_y(i), n, ha='left' if side == 'L' else 'right',
                va='center', fontsize=8, color='white', fontweight='bold' if used else 'normal')


# ---------------- Module ----------------
def module(x, y, w, title, pins, side, fc, note=None):
    """pins: list of (label, net or None). side: chân ở cạnh 'L' hoặc 'R' của module. Trả về dict label->(x,y)."""
    h = 0.42 * len(pins) + 0.7
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle='round,pad=0,rounding_size=0.1', fc=fc, ec='black', lw=1.2))
    ax.text(x + w / 2, y + h - 0.3, title, ha='center', va='center', fontsize=10.5, fontweight='bold')
    if note:
        ax.text(x + w / 2, y - 0.25, note, ha='center', va='top', fontsize=8.3, style='italic')
    pos = {}
    for k, (lab, net) in enumerate(pins):
        py = y + h - 0.75 - 0.42 * k
        px = x if side == 'L' else x + w
        ax.add_patch(plt.Circle((px, py), 0.07, fc='white', ec='black', lw=0.8))
        ax.text(px + (0.15 if side == 'L' else -0.15), py, lab, ha='left' if side == 'L' else 'right', va='center', fontsize=8.5)
        pos[lab] = (px, py)
        if net:
            net_flag(px, py, net, -1 if side == 'L' else 1)
    return pos


def net_flag(x, y, net, d):
    """Nhãn nguồn: các chân cùng nhãn được nối chung."""
    x2 = x + d * 0.45
    ax.plot([x, x2], [y, y], color=NET[net], lw=2)
    ax.text(x2 + d * 0.06, y, net, ha='left' if d > 0 else 'right', va='center', fontsize=8.5,
            color='white', fontweight='bold', bbox=dict(boxstyle='round,pad=0.15', fc=NET[net], ec='none'))


def wire(p, q, color, xmid, label=None):
    (x1, y1), (x2, y2) = p, q
    ax.plot([x1, xmid, xmid, x2], [y1, y1, y2, y2], color=color, lw=2.2, solid_capstyle='round')
    for (x, y) in (p, q):
        ax.add_patch(plt.Circle((x, y), 0.06, fc=color, ec=color))
    if label:   # nhãn đặt sát chân Blue Pill, phía trên dây
        d = 1 if x2 > x1 else -1
        ax.text(x2 - d * 0.15, y2 + 0.1, label, fontsize=8, color=color, va='bottom',
                ha='right' if d > 0 else 'left', fontweight='bold')


# Nhãn nguồn trên Blue Pill
net_flag(*board_pin('5V', 'L'), '5V', -1)
net_flag(*board_pin('G', 'L'), 'GND', -1)
net_flag(*board_pin('3V3', 'L'), '3V3', -1)

# USB-TTL (trái trên)
usb = module(1.0, 6.4, 2.6, 'Mạch USB-TTL', [('RX', None), ('TX', None), ('GND', 'GND'), ('5V/3V3', None)], 'R', '#e8e8e8',
             note='chân 5V/3V3 để trống (Blue Pill cấp nguồn riêng)')
wire(usb['RX'], board_pin('A9', 'L'), '#2ca02c', 5.2, 'A9 = USART1 TX')
wire(usb['TX'], board_pin('A10', 'L'), '#17becf', 5.7, 'A10 = USART1 RX')

# HC-SR04 (trái dưới)
srf = module(1.0, 0.9, 2.6, 'Siêu âm HC-SR04', [('VCC', '5V'), ('Trig', None), ('Echo', None), ('GND', 'GND')], 'R', '#cfe2f3',
             note='4 chân: VCC, Trig, Echo, GND')
wire(srf['Echo'], board_pin('B6', 'L'), '#9467bd', 5.0, 'B6 = TIM4_CH1 (chịu 5 V)')
# Trig: đi vòng dưới bo mạch sang chân B0 bên phải
tx, ty = srf['Trig']
bx, by = board_pin('B0', 'R')
ax.plot([tx, 4.4, 4.4, 10.6, 10.6, bx], [ty, ty, -0.25, -0.25, by, by], color='#8c564b', lw=2.2)
ax.add_patch(plt.Circle((tx, ty), 0.06, fc='#8c564b')); ax.add_patch(plt.Circle((bx, by), 0.06, fc='#8c564b'))
ax.text(bx + 0.15, by + 0.1, 'B0 = Trig', fontsize=8, color='#8c564b', va='bottom', fontweight='bold')

# Còi chip (trái giữa): I/O -> B1, đi vòng dưới bo mạch như dây Trig
bz = module(1.0, 3.95, 2.6, 'Còi chủ động 2 chân', [('Còi +', '5V'), ('Còi −', None)], 'R', '#ead1dc',
            note='không cần transistor: (−) nối PB12 + PB13')
zx, zy = bz['Còi −']
for pin, xv in (('B12', 4.75), ('B13', 4.6)):
    bx, by = board_pin(pin, 'L')
    ax.plot([zx, xv, xv, bx], [zy, zy, by, by], color='#c2185b', lw=2.2)
    ax.add_patch(plt.Circle((bx, by), 0.06, fc='#c2185b'))
ax.add_patch(plt.Circle((zx, zy), 0.06, fc='#c2185b'))

# OLED SSD1306 I2C (trái trên cùng): SCL -> B10, SDA -> B11, dây đi vòng phía trên bo mạch
ol = module(1.0, 9.4, 2.6, 'OLED SSD1306 (I2C)', [('GND', 'GND'), ('VCC', '3V3'), ('SCL', None), ('SDA', None)], 'R', '#d0e0e3',
            note='hiện tải CPU % + đồ thị 60 s')
for (lab, pin, xl, ytop, xr, col) in (('SCL', 'B10', 5.9, 11.95, 10.4, '#0b5394'), ('SDA', 'B11', 6.2, 11.75, 10.15, '#38761d')):
    sx, sy = ol[lab]
    px_, py_ = board_pin(pin, 'R')
    ax.plot([sx, xl, xl, xr, xr, px_], [sy, sy, ytop, ytop, py_, py_], color=col, lw=2.2)
    ax.add_patch(plt.Circle((sx, sy), 0.06, fc=col)); ax.add_patch(plt.Circle((px_, py_), 0.06, fc=col))
    ax.text(px_ + 0.15, py_ + 0.08, lab, fontsize=8, color=col, va='bottom', fontweight='bold')

# MAX7219 (phải trên)
mx = module(13.2, 6.5, 3.0, 'Ma trận LED MAX7219', [('VCC', '5V'), ('GND', 'GND'), ('DIN', None), ('CS', None), ('CLK', None)], 'L', '#f4cccc',
            note='nguồn 5 V, tín hiệu 3,3 V vẫn nhận được')
wire(mx['DIN'], board_pin('A7', 'R'), '#e377c2', 11.2, 'A7 = SPI1 MOSI')
wire(mx['CS'], board_pin('A4', 'R'), '#bcbd22', 11.7, 'A4 = CS')
wire(mx['CLK'], board_pin('A5', 'R'), '#1f77b4', 12.2, 'A5 = SPI1 SCK')

# Hồng ngoại (phải giữa)
ir = module(13.2, 2.75, 3.0, 'Hồng ngoại vật cản', [('VCC', '3V3'), ('GND', 'GND'), ('OUT', None)], 'L', '#d9ead3',
            note='OUT = 0 khi có vật (EXTI1)')
wire(ir['OUT'], board_pin('A1', 'R'), '#ff7f0e', 11.9, 'A1')

# Quang trở (phải dưới)
ldr = module(13.2, 0.0, 3.0, 'Quang trở LM393', [('VCC', '3V3'), ('GND', 'GND'), ('DO', None)], 'L', '#fff2cc',
             note='DO = 1 khi tối (EXTI0)')
wire(ldr['DO'], board_pin('A0', 'R'), '#7f7f7f', 11.4, 'A0')

ax.text(8.6, 12.75, 'SƠ ĐỒ CẮM CHÂN – HỆ THỐNG HỖ TRỢ ĐỖ XE (FreeRTOS trên STM32F103C8)', ha='center',
        fontsize=14, fontweight='bold')
ax.text(8.6, 12.35, 'Các chân mang cùng nhãn nguồn (5V, 3V3, GND) nối chung với chân tương ứng của Blue Pill. '
        'Tất cả GND phải nối chung. Chân vàng = chân được dùng.', fontsize=9, style='italic', ha='center')

fig.tight_layout()
fig.savefig(OUT, dpi=170)
print('saved', os.path.abspath(OUT))
