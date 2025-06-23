/*
Keyball44 - マウスレイヤー自動切替 + クリック復帰 + フラッシュ + 呼吸エフェクト
*/

#include QMK_KEYBOARD_H
#include "lib/keyball/keyball.h" // ★★★ この行を追加しました ★★★

// --- 機能設定 ---
#define MOUSE_LAYER 3                   // マウス操作レイヤーの番号
#define AML_ACTIVATE_THRESHOLD 5        // マウスレイヤーを発動するマウス移動のしきい値
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000  // マウスレイヤーを維持する時間（ミリ秒、30秒）
#define MOUSE_CLICK_REVERT_TIME 500     // クリック後に通常レイヤーに戻る時間（ミリ秒）
#define LED_FLASH_TIME 200              // クリック時にLEDがフラッシュする時間（ミリ秒）

// --- 状態管理用変数 ---
static bool mouse_layer_active = false;
static uint16_t last_mouse_activity_timer = 0;
static bool mouse_click_detected = false;
static uint16_t mouse_click_timer = 0;

static bool led_flash_active = false;
static uint16_t led_flash_timer = 0;

report_mouse_t last_report = {0};

// ===================================
// ■ キーマップ定義
// ===================================
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // レイヤー0：通常キー
    [0] = LAYOUT_universal(
        KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_DEL,
        KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, S(KC_7),
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_INT1,
                          KC_LALT, KC_LGUI, KC_GRV, LT(1,KC_SPC), LT(3,KC_ENT), KC_BSPC, LT(2,KC_TAB), RCTL_T(KC_LNG2), KC_RALT, KC_PSCR
    ),
    // レイヤー1：数字キー＋カーソル操作
    [1] = LAYOUT_universal(
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,
        _______, _______, _______, KC_UP,   KC_ENT,  KC_DEL,  KC_PGUP, KC_BTN1, KC_MS_U, KC_BTN2, KC_BTN3, KC_F12,
        _______, _______, KC_LEFT, KC_DOWN, KC_RGHT, KC_BSPC, KC_PGDN, KC_MS_L, KC_MS_D, KC_MS_R, _______, _______,
                          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______
    ),
    // レイヤー2：記号入力専用
    [2] = LAYOUT_universal(
        _______, S(KC_QUOT), KC_7,    KC_8,    KC_9,    S(KC_8), S(KC_9), S(KC_1), S(KC_6), KC_LBRC, S(KC_4), _______,
        _______, S(KC_SCLN), KC_4,    KC_5,    KC_6,    KC_RBRC, KC_NUHS, KC_MINS, S(KC_EQL), S(KC_3), KC_QUOT, S(KC_2),
        _______, S(KC_MINS), KC_1,    KC_2,    KC_3,    S(KC_RBRC),S(KC_NUHS),S(KC_INT1),KC_EQL, S(KC_LBRC),S(KC_SLSH),S(KC_INT3),
                          KC_0,    KC_DOT,  _______, _______, _______, KC_DEL,  _______, _______, _______, _______
    ),
    // レイヤー3：マウス操作専用（トラックボール連動）
    [3] = LAYOUT_universal(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, KC_BTN1, KC_BTN2, KC_BTN3, _______, _______, _______, KC_WH_U, KC_MS_U, KC_WH_D, _______, _______,
        _______, KC_MS_L, KC_MS_D, KC_MS_R, KC_BTN4, KC_BTN5, _______, KC_WH_L, KC_MS_D, KC_WH_R, _______, _______,
                          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______
    ),
    // レイヤー4：RGBカラー設定・スクロールモード等の設定専用レイヤー
    [4] = LAYOUT_universal(
        RGB_TOG, _______, _______, _______, _______, _______, RGB_M_P, RGB_M_B, RGB_M_R, RGB_M_SW, RGB_M_SN, RGB_M_K,
        RGB_MOD, RGB_HUI, RGB_SAI, RGB_VAI, _______, _______, RGB_M_X, RGB_M_G, RGB_M_T, RGB_M_TW, _______, _______,
        RGB_RMOD,RGB_HUD, RGB_SAD, RGB_VAD, _______, _______, _______, _______, _______, _______, _______, QK_BOOT,
                          QK_BOOT, _______, _______, _______, _______, _______, _______, _______, _______, QK_BOOT
    )
};

// ===================================
// ■ LED色更新関数
// ===================================
void update_layer_rgb_color(void) {
    if (!rgblight_is_enabled()) { return; }
    switch (get_highest_layer(layer_state)) {
        case 1: rgblight_sethsv_noeeprom(HSV_GREEN); break;
        case 2: rgblight_sethsv_noeeprom(HSV_WHITE); break;
        case 3: rgblight_sethsv_noeeprom(HSV_RED); break;
        case 4: rgblight_sethsv_noeeprom(HSV_ORANGE); break;
        default: rgblight_sethsv_noeeprom(HSV_BLUE); break;
    }
}

// ===================================
// ■ 呼吸エフェクト用の明るさ計算
// ===================================
uint8_t get_breathing_brightness(void) {
    uint16_t time = timer_read() % 3000; // 3秒周期
    if (time > 1500) {
        time = 3000 - time;
    }
    return (uint8_t)(time * 255 / 1500);
}

// ===================================
// ■ レイヤー変更時の処理
// ===================================
layer_state_t layer_state_set_user(layer_state_t state) {
    update_layer_rgb_color();
    keyball_set_scroll_mode(get_highest_layer(state) == 3);
    return state;
}

// ===================================
// ■ トラックボール動作時のレイヤー制御
// ===================================
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    if (abs(mouse_report.x) >= AML_ACTIVATE_THRESHOLD || abs(mouse_report.y) >= AML_ACTIVATE_THRESHOLD) {
        if (!mouse_layer_active) {
            mouse_layer_active = true;
            layer_on(MOUSE_LAYER);
        }
        last_mouse_activity_timer = timer_read();
    }
    last_report = mouse_report;
    return mouse_report;
}

// ===================================
// ■ クリック検出（復帰用）
// ===================================
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case KC_BTN1:
            case KC_BTN2:
            case KC_BTN3:
            case KC_BTN4:
            case KC_BTN5:
                if (mouse_layer_active) {
                    mouse_click_detected = true;
                    mouse_click_timer = timer_read();
                }
                // LEDフラッシュ
                led_flash_active = true;
                led_flash_timer = timer_read();
                rgblight_sethsv_noeeprom(HSV_WHITE);
                break;
        }
    }
    return true;
}

// ===================================
// ■ メインループ処理
// ===================================
void matrix_scan_user(void) {
    // クリック後、指定時間でレイヤーOFF
    if (mouse_click_detected && timer_elapsed(mouse_click_timer) > MOUSE_CLICK_REVERT_TIME) {
        layer_off(MOUSE_LAYER);
        mouse_layer_active = false;
        mouse_click_detected = false;
        update_layer_rgb_color();
    }

    // 無操作が指定時間続いたらレイヤーOFF
    if (mouse_layer_active && !mouse_click_detected && timer_elapsed(last_mouse_activity_timer) > AUTO_MOUSE_LAYER_KEEP_TIME) {
        layer_off(MOUSE_LAYER);
        mouse_layer_active = false;
        update_layer_rgb_color();
    }

    // LEDフラッシュ処理
    if (led_flash_active) {
        if (timer_elapsed(led_flash_timer) >= LED_FLASH_TIME) {
            led_flash_active = false;
            update_layer_rgb_color();
        }
        return;
    }

    // レイヤー1での呼吸エフェクト
    if (get_highest_layer(layer_state) == 1) {
        uint8_t breath_v = get_breathing_brightness();
        rgblight_sethsv_noeeprom(0, 255, breath_v); // 赤色の呼吸
    }
}

// ===================================
// ■ OLED表示
//