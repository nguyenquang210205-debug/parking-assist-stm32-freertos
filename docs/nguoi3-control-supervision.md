# Nguoi 3 - ControlTask, ConsoleTask, FSM va giam sat

Pham vi: du an `parking_assist` tren STM32F103C8, FreeRTOS. Tai lieu nay ghi lai phan code va nhung kiem tra de review theo quy trinh Git cua nhom.

## Luong du lieu

1. `SonarTask` gui `EV_DIST` (mm) va `SensorTask` gui `EV_IR`, `EV_LIGHT` vao `g_qEvents`.
2. `ControlTask` doc `g_qEvents`, chay `pk_fsm_on_distance()` / `pk_fsm_on_ir()`, nhan `EV_CMD_SET` / `EV_CMD_RESET`.
3. `ControlTask` ghi `g_qParkView` (mailbox 1 phan tu) bang `xQueueOverwrite()`; `DisplayTask`, `OledTask`, `ConsoleTask`, `SonarTask` chi peek de xem trang thai.
4. `ConsoleTask` nhan dong lenh UART, gui command qua queue, lay thong ke CPU/stack/heap tu `cpu_monitor`, dat bit READY de cac task bat dau.
5. `OledTask` doc mailbox va thong ke CPU moi 0,5 giay, cap nhat bieu do 60 mau va truyen SSD1306 qua I2C2.

## May trang thai (FSM)

| Trang thai | Dieu kien | Trang thai tiep theo |
|---|---|---|
| EMPTY | Hong ngoai co xe | APPROACH |
| APPROACH | Do duoc `d <= stop_mm` | STOP |
| APPROACH | Mat IR va chua vao vung gan > 15 s | EMPTY |
| STOP | `d > stop_mm + 20 mm` | APPROACH |
| STOP | **Lien tuc** `d <= stop_mm` du `park_hold_ms` | PARKED |
| PARKED | Cach xa `stop_mm + 80 mm` du 0,5 s | LEAVING |
| LEAVING | Hong ngoai thay xe di qua roi het thay | EMPTY |
| LEAVING | Hong ngoai khong thay xe du `empty_timeout_ms` | EMPTY |
| LEAVING | Xe quay lai `d <= stop_mm` | STOP |

Chinh sua trong ban nay:
- Khi mat echo hoac khoang cach nhich ra ngoai ngung STOP (du chua ra khoi vung tre), bo dem xac nhan do **duoc khoi dong lai**. Do xe lien tuc du 4 giay moi hien P.
- Moi lan vao lai PARKED, xoa co `away` cua luot truoc de tranh nhan nham xe roi di ngay lap tuc.
- Khi nhap `set hold` / `set empty`, doi giay sang mili giay co kiem tra tran 16 bit: khong chap nhan gia tri >= 66 giay. `hold` con bi gioi han 1..30 giay boi `pk_settings_valid`.
- Lenh `set`/`reset` thong bao neu khong day duoc event vao queue thay vi im lang bo lenh.
- Parser UART kiem tra do dai gia tri so truoc phep nhan: chuoi so qua lon khong bi tran `uint32_t`.

## Kiem thu

Trong thu muc goc `parking_assist`:

```bash
make test
```

Danh muc: kiem tra FSM day du, dao quanh vach dung, mat echo trong luc STOP, `set hold/empty` khong tran, quay lai STOP/da do, chong nhay so, do thi CPU, LED va coi. `make test` chay tren may tinh bang GCC; **khong thay the** kiem tra firmware tren Keil/STM32.

## Quy trinh Git de dua len repo du an dung

Chay lenh sau trong **repository Parking Assist cua nhom**, khi truong nhom da tao `develop`:

```bash
git checkout develop
git pull origin develop
git checkout -b feature/control-supervision
# Chep cac file da thay doi tu ban nay vao repository dung
git add app/inc/parking_fsm.h app/src/parking_fsm.c app/src/control_task.c app/src/console_task.c test/test_main.c docs/nguoi3-control-supervision.md
git commit -m "fix(control): validate settings and enforce continuous parking hold"
git push -u origin feature/control-supervision
```

Tao Pull Request **feature/control-supervision -> develop**, nho thanh vien khac review. Khong commit truc tiep vao `main`, khong tu chinh `main.c`, va khong dua `build/`, `Debug/`, `Release/` vao Git.
