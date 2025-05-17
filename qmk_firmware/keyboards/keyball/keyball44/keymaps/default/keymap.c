/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#include "quantum.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // keymap for default
  [0] = LAYOUT_universal(
    KC_ESC   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                        KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_DEL   ,
    KC_TAB   , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                        KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  , S(KC_7)  ,
    KC_LSFT  , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                                        KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , KC_INT1  ,
              KC_LALT,KC_LGUI,LCTL_T(KC_LANG2)     ,LT(1,KC_SPC),LT(3,KC_LANG2),                  KC_BSPC,LT(2,KC_ENT), RCTL_T(KC_LANG2),     KC_RALT  , KC_PSCR
  ),

  [1] = LAYOUT_universal(
    _______  ,  KC_F1   , KC_F2    , KC_F3   , KC_F4    , KC_F5    ,                                         KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   , KC_F11   ,
    _______  ,  _______ , _______  , KC_UP   , KC_ENT   , KC_DEL   ,                                         KC_PGUP  , KC_BTN1  , KC_UP    , KC_BTN2  , KC_BTN3  , KC_F12   ,
    _______  ,  _______ , KC_LEFT  , KC_DOWN , KC_RGHT  , KC_BSPC  ,                                         KC_PGDN  , KC_LEFT  , KC_DOWN  , KC_RGHT  , _______  , _______  ,
                  _______  , _______ , _______  ,         _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [2] = LAYOUT_universal(
    _______  ,S(KC_QUOT), KC_7     , KC_8    , KC_9     , S(KC_8)  ,                                         S(KC_9)  , S(KC_1)  , S(KC_6)  , KC_LBRC  , S(KC_4)  , _______  ,
    _______  ,S(KC_SCLN), KC_4     , KC_5    , KC_6     , KC_RBRC  ,                                         KC_NUHS  , KC_MINS  , S(KC_EQL), S(KC_3)  , KC_QUOT  , S(KC_2)  ,
    _______  ,S(KC_MINS), KC_1     , KC_2    , KC_3     ,S(KC_RBRC),                                        S(KC_NUHS),S(KC_INT1), KC_EQL   ,S(KC_LBRC),S(KC_SLSH),S(KC_INT3),
                  KC_0     , KC_DOT  , _______  ,         _______  , _______  ,                   KC_DEL   , _______  , _______       , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , _______  , _______  , _______  , _______  , _______  ,                                        RGB_M_P  , RGB_M_B  , RGB_M_R  , RGB_M_SW , RGB_M_SN , RGB_M_K  ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , _______  , SCRL_DVI ,                                        RGB_M_X  , RGB_M_G  , RGB_M_T  , RGB_M_TW , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , _______  , SCRL_DVD ,                                        CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , _______  , KBC_SAVE ,
                  RESET    , KBC_RST  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , KBC_RST  , RESET
  ),
};

// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    // Auto enable scroll mode when the highest layer is 3
    keyball_set_scroll_mode(get_highest_layer(state) == 3);
    return state;
}

#ifdef OLED_ENABLE

#    include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
}
#endif

#define MOUSE_LAYER 2
#define AML_ACTIVATE_THRESHOLD 5
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000  // 30秒

static bool mouse_layer_active = false;
static uint16_t mouse_click_timer = 0;
static bool mouse_click_detected = false;
static uint16_t last_mouse_activity_timer = 0;

report_mouse_t last_report = {0};

void pointing_device_task_user(report_mouse_t *mouse_report) {
    int16_t dx = mouse_report->x - last_report.x;
    int16_t dy = mouse_report->y - last_report.y;

    if (abs(dx) >= AML_ACTIVATE_THRESHOLD || abs(dy) >= AML_ACTIVATE_THRESHOLD) {
        if (!mouse_layer_active) {
            layer_on(MOUSE_LAYER);
            mouse_layer_active = true;
        }
        // 動かした時にタイマーリセット
        last_mouse_activity_timer = timer_read();
    }

    last_report = *mouse_report;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_MS_BTN1:
        case KC_MS_BTN2:
        case KC_MS_BTN3:
            if (record->event.pressed) {
                mouse_click_detected = true;
                mouse_click_timer = timer_read();
            }
            break;
    }
    return true;
}

void matrix_scan_user(void) {
    // クリック後500msでマウスレイヤーOFF
    if (mouse_click_detected) {
        if (timer_elapsed(mouse_click_timer) > 500) {
            layer_off(MOUSE_LAYER);
            mouse_layer_active = false;
            mouse_click_detected = false;
        }
    }

    // タイムアウトでマウスレイヤーOFF（30秒経過時）
    if (mouse_layer_active && !mouse_click_detected) {
        if (timer_elapsed(last_mouse_activity_timer) > AUTO_MOUSE_LAYER_KEEP_TIME) {
            layer_off(MOUSE_LAYER);
            mouse_layer_active = false;
        }
    }
}
