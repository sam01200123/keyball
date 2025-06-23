# MCU name
MCU = RP2040

# Bootloader selection
BOOTLOADER = rp2040

# Build Options
SPLIT_KEYBOARD = yes
RGBLIGHT_ENABLE = yes
OLED_ENABLE = yes
EXTRAKEY_ENABLE = yes
MOUSEKEY_ENABLE = yes
COMMAND_ENABLE = no

# .hファイル内のレイアウト定義エラーを回避する
VALIDATE_LAYOUTS = no

# ライブラリソースの追加
SRC += lib/keyball/keyball.c