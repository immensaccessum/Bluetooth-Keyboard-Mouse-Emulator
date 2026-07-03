# M5 Keyboard and Mouse Emulator

USB/BLE HID keyboard and mouse for **M5 Cardputer** and **Cardputer ADV**.

## Features

- USB or Bluetooth HID mode
- Mouse and keyboard mode (toggle with **G0**)
- Adjustable mouse speed (**Fn+F11** slower, **Fn+F12** faster)
- Settings saved in flash (USB/BT choice, mode, speed)
- Full keyboard mapping including Fn layer (F-keys, arrows, special keys)
- Reboot to launcher/menu: **Ctrl+Fn+Esc**

## Controls

| Action | Input |
|--------|-------|
| USB / Bluetooth at boot | `;` or `.` to switch, **Enter** to confirm |
| Mouse / Keyboard | **G0** (side button) |
| Mouse move | `,` `.` `;` `/` or arrow keys (Fn layer) |
| Mouse click | **Enter** left, `\` right |
| Mouse speed | **Fn+F11** / **Fn+F12** |
| Back to launcher | **Ctrl+Fn+Esc** (then **Enter** on boot screen) |

## Build & flash

```bash
pio run -t upload
```

Or flash `MouseKeyboard.bin` from releases with esptool / web flasher.

## Version

1.2 — settings, mouse speed, ADV keyboard mapping, launcher reboot shortcut
