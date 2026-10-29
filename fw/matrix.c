#include "matrix.h"
#include "class/hid/hid.h"
#include "config.h"
#include "led.h"
#include "lighting.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>

static uint const COL_TO_PIN[NCOL] = {1,  0,  47, 46, 45, 44, 43, 42, 41, 40, 39,
                                      38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28};

static uint const ROW_TO_PIN[NROW] = {17, 18, 19, 20, 21, 22};

#define FN_KEY_COL 12
#define FN_KEY_ROW 5
#define DEBOUNCE_DELAY 5000

typedef struct __attribute__((packed)) {
    bool raw : 1;
    bool debounced : 1;
    uint32_t last_change;
} key_state_t;

static key_state_t KEY_STATES[NROW][NCOL] = {};

void matrix_init(void) {
    for (uint i = 0; i < NROW; ++i) {
        uint pin = ROW_TO_PIN[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
    }

    for (uint i = 0; i < NCOL; ++i) {
        uint pin = COL_TO_PIN[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);
    }
}

void matrix_query(matrix_result_t *res) {
    uint32_t now = time_us_32();

    gpio_put(COL_TO_PIN[FN_KEY_COL], 1);
    sleep_us(10);
    bool const fn = gpio_get(ROW_TO_PIN[FN_KEY_ROW]);
    gpio_put(COL_TO_PIN[FN_KEY_COL], 0);

    for (uint col = 0; col < NCOL; ++col) {
        gpio_put(COL_TO_PIN[col], 1);
        sleep_us(10);

        for (uint row = 0; row < NROW; ++row) {
            bool pressed = gpio_get(ROW_TO_PIN[row]);
            key_state_t *state = &KEY_STATES[row][col];

            if (pressed != state->raw) {
                state->raw = pressed;
                state->last_change = now;
            }

            if (state->raw != state->debounced && now - state->last_change >= DEBOUNCE_DELAY) {
                state->debounced = state->raw;
            }

            if (state->debounced) {
                matrix_result_add(res, matrix_keycode(row, col, fn));
            }
        }

        gpio_put(COL_TO_PIN[col], 0);
    }
}

void matrix_result_reset(matrix_result_t *res) {
    memset(res, 0, sizeof(matrix_result_t));
}

void matrix_result_add(matrix_result_t *res, uint16_t kc) {
    if (!kc) {
        return;
    }

    if (kc <= 0xFF) {
        if (kc >= HID_KEY_CONTROL_LEFT && kc <= HID_KEY_GUI_RIGHT) {
            res->modifiers |= 1 << (kc - HID_KEY_CONTROL_LEFT);
            return;
        }

        if (res->narray < NKEYARRAY) {
            res->array[res->narray++] = kc;
        } else {
            res->rollover = true;
        }

        res->bitmap[kc >> 3] |= 1 << (kc & 7);
        return;
    }

    switch (kc & KEY_FN_MASK) {
    case KEY_MACRO_MASK:
        res->macros |= 1 << (kc & KEY_MACRO_BITS);
        break;

    case KEY_MULTIMEDIA_MASK:
        if (res->nmultimedia < NMULTIMEDIA) {
            res->multimedia[res->nmultimedia++] = kc & KEY_MULTIMEDIA_BITS;
        }
        break;

    case KEY_MISC_MASK:
        switch (kc) {
        case KEY_WINLOCK:
            syntaxkbd_config.flag_winlock ^= 1;
            led_refresh();
            config_store();
            break;

        case KEY_LIGHTING_TOGGLE:
            syntaxkbd_config.flag_lighting ^= 1;
            lighting_dispatch(&((lcmd_t){.type = LCMD_RESTART}));
            lighting_wait();
            config_store();
            break;
        }
        break;

    default:
        printf("Ignoring unrecognized keycode %04X.\n", kc);
        break;
    }
}

static uint8_t matrix_normal[NROW][NCOL] = {
    {
        0,
        HID_KEY_ESCAPE,
        0,
        HID_KEY_F1,
        HID_KEY_F2,
        HID_KEY_F3,
        HID_KEY_F4,
        0,
        HID_KEY_F5,
        HID_KEY_F6,
        HID_KEY_F7,
        HID_KEY_F8,
        HID_KEY_F9,
        HID_KEY_F10,
        HID_KEY_F11,
        HID_KEY_PRINT_SCREEN,
        HID_KEY_SCROLL_LOCK,
        HID_KEY_PAUSE,
        0,
        0,
        0,
        0,
    },
    {
        0,
        HID_KEY_GRAVE,
        HID_KEY_1,
        HID_KEY_2,
        HID_KEY_3,
        HID_KEY_4,
        HID_KEY_5,
        HID_KEY_6,
        HID_KEY_7,
        HID_KEY_8,
        HID_KEY_9,
        HID_KEY_0,
        HID_KEY_MINUS,
        HID_KEY_EQUAL,
        HID_KEY_F12,
        HID_KEY_INSERT,
        HID_KEY_HOME,
        HID_KEY_PAGE_UP,
        HID_KEY_NUM_LOCK,
        HID_KEY_KEYPAD_DIVIDE,
        HID_KEY_KEYPAD_MULTIPLY,
        HID_KEY_KEYPAD_SUBTRACT,
    },
    {
        0,
        HID_KEY_TAB,
        HID_KEY_Q,
        HID_KEY_W,
        HID_KEY_E,
        HID_KEY_R,
        HID_KEY_T,
        HID_KEY_Y,
        HID_KEY_U,
        HID_KEY_I,
        HID_KEY_O,
        HID_KEY_P,
        HID_KEY_BRACKET_LEFT,
        HID_KEY_BRACKET_RIGHT,
        HID_KEY_BACKSPACE,
        HID_KEY_DELETE,
        HID_KEY_END,
        HID_KEY_PAGE_DOWN,
        HID_KEY_KEYPAD_7,
        HID_KEY_KEYPAD_8,
        HID_KEY_KEYPAD_9,
        HID_KEY_KEYPAD_ADD,
    },
    {
        0,
        HID_KEY_CAPS_LOCK,
        HID_KEY_A,
        HID_KEY_S,
        HID_KEY_D,
        HID_KEY_F,
        HID_KEY_G,
        HID_KEY_H,
        HID_KEY_J,
        HID_KEY_K,
        HID_KEY_L,
        HID_KEY_SEMICOLON,
        HID_KEY_APOSTROPHE,
        HID_KEY_EUROPE_1,
        HID_KEY_ENTER,
        0,
        0,
        0,
        HID_KEY_KEYPAD_4,
        HID_KEY_KEYPAD_5,
        HID_KEY_KEYPAD_6,
        0,
    },
    {
        0,
        HID_KEY_SHIFT_LEFT,
        HID_KEY_EUROPE_2,
        HID_KEY_Z,
        HID_KEY_X,
        HID_KEY_C,
        HID_KEY_V,
        HID_KEY_B,
        HID_KEY_N,
        HID_KEY_M,
        HID_KEY_COMMA,
        HID_KEY_PERIOD,
        HID_KEY_SLASH,
        HID_KEY_SHIFT_RIGHT,
        0,
        0,
        HID_KEY_ARROW_UP,
        0,
        HID_KEY_KEYPAD_1,
        HID_KEY_KEYPAD_2,
        HID_KEY_KEYPAD_3,
        HID_KEY_KEYPAD_ENTER,
    },
    {
        0,
        HID_KEY_CONTROL_LEFT,
        HID_KEY_GUI_LEFT,
        HID_KEY_ALT_LEFT,
        0,
        0,
        0,
        HID_KEY_SPACE,
        0,
        0,
        0,
        HID_KEY_ALT_RIGHT,
        0,
        HID_KEY_APPLICATION,
        HID_KEY_CONTROL_RIGHT,
        HID_KEY_ARROW_LEFT,
        HID_KEY_ARROW_DOWN,
        HID_KEY_ARROW_RIGHT,
        HID_KEY_KEYPAD_0,
        0,
        HID_KEY_KEYPAD_COMMA,
        0,
    },
};

uint16_t matrix_keycode(uint row, uint col, bool fn) {
    uint8_t kc = matrix_normal[row][col];

    if (fn) {
        switch (kc) {
        case HID_KEY_F1:
        case HID_KEY_F2:
        case HID_KEY_F3:
        case HID_KEY_F4:
        case HID_KEY_F5:
        case HID_KEY_F6:
        case HID_KEY_F7:
        case HID_KEY_F8:
        case HID_KEY_F9:
        case HID_KEY_F10:
        case HID_KEY_F11:
            return syntaxkbd_config.fkey_function[kc - HID_KEY_F1];

        case HID_KEY_GUI_LEFT:
        case HID_KEY_GUI_RIGHT:
            return KEY_WINLOCK;

        case HID_KEY_PRINT_SCREEN:
            return KEY_LIGHTING_TOGGLE;
        }

        return HID_KEY_NONE;
    }

    return kc;
}

#ifdef SYNTAXKBD_DEBUG
char const *conv_table[256] = {
    [HID_KEY_NONE] = "(none)",
    [HID_KEY_A] = "A",
    [HID_KEY_B] = "B",
    [HID_KEY_C] = "C",
    [HID_KEY_D] = "D",
    [HID_KEY_E] = "E",
    [HID_KEY_F] = "F",
    [HID_KEY_G] = "G",
    [HID_KEY_H] = "H",
    [HID_KEY_I] = "I",
    [HID_KEY_J] = "J",
    [HID_KEY_K] = "K",
    [HID_KEY_L] = "L",
    [HID_KEY_M] = "M",
    [HID_KEY_N] = "N",
    [HID_KEY_O] = "O",
    [HID_KEY_P] = "P",
    [HID_KEY_Q] = "Q",
    [HID_KEY_R] = "R",
    [HID_KEY_S] = "S",
    [HID_KEY_T] = "T",
    [HID_KEY_U] = "U",
    [HID_KEY_V] = "V",
    [HID_KEY_W] = "W",
    [HID_KEY_X] = "X",
    [HID_KEY_Y] = "Y",
    [HID_KEY_Z] = "Z",
    [HID_KEY_1] = "1",
    [HID_KEY_2] = "2",
    [HID_KEY_3] = "3",
    [HID_KEY_4] = "4",
    [HID_KEY_5] = "5",
    [HID_KEY_6] = "6",
    [HID_KEY_7] = "7",
    [HID_KEY_8] = "8",
    [HID_KEY_9] = "9",
    [HID_KEY_0] = "0",
    [HID_KEY_ENTER] = "Enter",
    [HID_KEY_ESCAPE] = "Escape",
    [HID_KEY_BACKSPACE] = "Backspace",
    [HID_KEY_TAB] = "Tab",
    [HID_KEY_SPACE] = "Space",
    [HID_KEY_MINUS] = "-",
    [HID_KEY_EQUAL] = "=",
    [HID_KEY_BRACKET_LEFT] = "[",
    [HID_KEY_BRACKET_RIGHT] = "]",
    [HID_KEY_BACKSLASH] = "\\",
    [HID_KEY_EUROPE_1] = "Europe1",
    [HID_KEY_SEMICOLON] = ";",
    [HID_KEY_APOSTROPHE] = "'",
    [HID_KEY_GRAVE] = "`",
    [HID_KEY_COMMA] = ",",
    [HID_KEY_PERIOD] = ".",
    [HID_KEY_SLASH] = "/",
    [HID_KEY_CAPS_LOCK] = "Caps",
    [HID_KEY_F1] = "F1",
    [HID_KEY_F2] = "F2",
    [HID_KEY_F3] = "F3",
    [HID_KEY_F4] = "F4",
    [HID_KEY_F5] = "F5",
    [HID_KEY_F6] = "F6",
    [HID_KEY_F7] = "F7",
    [HID_KEY_F8] = "F8",
    [HID_KEY_F9] = "F9",
    [HID_KEY_F10] = "F10",
    [HID_KEY_F11] = "F11",
    [HID_KEY_F12] = "F12",
    [HID_KEY_PRINT_SCREEN] = "PrtSc",
    [HID_KEY_SCROLL_LOCK] = "ScrLk",
    [HID_KEY_PAUSE] = "Pause",
    [HID_KEY_INSERT] = "Insert",
    [HID_KEY_HOME] = "Home",
    [HID_KEY_PAGE_UP] = "PageUp",
    [HID_KEY_DELETE] = "Delete",
    [HID_KEY_END] = "End",
    [HID_KEY_PAGE_DOWN] = "PageDown",
    [HID_KEY_ARROW_RIGHT] = "Right",
    [HID_KEY_ARROW_LEFT] = "Left",
    [HID_KEY_ARROW_DOWN] = "Down",
    [HID_KEY_ARROW_UP] = "Up",
    [HID_KEY_NUM_LOCK] = "NumLock",
    [HID_KEY_KEYPAD_DIVIDE] = "KP/",
    [HID_KEY_KEYPAD_MULTIPLY] = "KP*",
    [HID_KEY_KEYPAD_SUBTRACT] = "KP-",
    [HID_KEY_KEYPAD_ADD] = "KP+",
    [HID_KEY_KEYPAD_ENTER] = "KPEnter",
    [HID_KEY_KEYPAD_1] = "KP1",
    [HID_KEY_KEYPAD_2] = "KP2",
    [HID_KEY_KEYPAD_3] = "KP3",
    [HID_KEY_KEYPAD_4] = "KP4",
    [HID_KEY_KEYPAD_5] = "KP5",
    [HID_KEY_KEYPAD_6] = "KP6",
    [HID_KEY_KEYPAD_7] = "KP7",
    [HID_KEY_KEYPAD_8] = "KP8",
    [HID_KEY_KEYPAD_9] = "KP9",
    [HID_KEY_KEYPAD_0] = "KP0",
    [HID_KEY_KEYPAD_DECIMAL] = "KP.",
    [HID_KEY_EUROPE_2] = "Europe2",
    [HID_KEY_APPLICATION] = "Application",
    [HID_KEY_POWER] = "Power",
    [HID_KEY_KEYPAD_EQUAL] = "KP=",
    [HID_KEY_F13] = "F13",
    [HID_KEY_F14] = "F14",
    [HID_KEY_F15] = "F15",
    [HID_KEY_F16] = "F16",
    [HID_KEY_F17] = "F17",
    [HID_KEY_F18] = "F18",
    [HID_KEY_F19] = "F19",
    [HID_KEY_F20] = "F20",
    [HID_KEY_F21] = "F21",
    [HID_KEY_F22] = "F22",
    [HID_KEY_F23] = "F23",
    [HID_KEY_F24] = "F24",
    [HID_KEY_EXECUTE] = "Execute",
    [HID_KEY_HELP] = "Help",
    [HID_KEY_MENU] = "Menu",
    [HID_KEY_SELECT] = "Select",
    [HID_KEY_STOP] = "Stop",
    [HID_KEY_AGAIN] = "Again",
    [HID_KEY_UNDO] = "Undo",
    [HID_KEY_CUT] = "Cut",
    [HID_KEY_COPY] = "Copy",
    [HID_KEY_PASTE] = "Paste",
    [HID_KEY_FIND] = "Find",
    [HID_KEY_MUTE] = "Mute",
    [HID_KEY_VOLUME_UP] = "Vol+",
    [HID_KEY_VOLUME_DOWN] = "Vol-",
    [HID_KEY_LOCKING_CAPS_LOCK] = "LockCaps",
    [HID_KEY_LOCKING_NUM_LOCK] = "LockNumLock",
    [HID_KEY_LOCKING_SCROLL_LOCK] = "LockScrLk",
    [HID_KEY_KEYPAD_COMMA] = "KP,",
    [HID_KEY_KEYPAD_EQUAL_SIGN] = "KP=",
    [HID_KEY_KANJI1] = "Kanji1",
    [HID_KEY_KANJI2] = "Kanji2",
    [HID_KEY_KANJI3] = "Kanji3",
    [HID_KEY_KANJI4] = "Kanji4",
    [HID_KEY_KANJI5] = "Kanji5",
    [HID_KEY_KANJI6] = "Kanji6",
    [HID_KEY_KANJI7] = "Kanji7",
    [HID_KEY_KANJI8] = "Kanji8",
    [HID_KEY_KANJI9] = "Kanji9",
    [HID_KEY_LANG1] = "Lang1",
    [HID_KEY_LANG2] = "Lang2",
    [HID_KEY_LANG3] = "Lang3",
    [HID_KEY_LANG4] = "Lang4",
    [HID_KEY_LANG5] = "Lang5",
    [HID_KEY_LANG6] = "Lang6",
    [HID_KEY_LANG7] = "Lang7",
    [HID_KEY_LANG8] = "Lang8",
    [HID_KEY_LANG9] = "Lang9",
    [HID_KEY_ALTERNATE_ERASE] = "AltErase",
    [HID_KEY_SYSREQ_ATTENTION] = "SysReq",
    [HID_KEY_CANCEL] = "Cancel",
    [HID_KEY_CLEAR] = "Clear",
    [HID_KEY_PRIOR] = "Prior",
    [HID_KEY_RETURN] = "Return",
    [HID_KEY_SEPARATOR] = "Separator",
    [HID_KEY_OUT] = "Out",
    [HID_KEY_OPER] = "Oper",
    [HID_KEY_CLEAR_AGAIN] = "ClearAgain",
    [HID_KEY_CRSEL_PROPS] = "CrSel",
    [HID_KEY_EXSEL] = "Exsel",
    [HID_KEY_KEYPAD_00] = "KP00",
    [HID_KEY_KEYPAD_000] = "KP000",
    [HID_KEY_THOUSANDS_SEPARATOR] = "1000Sep",
    [HID_KEY_DECIMAL_SEPARATOR] = "DecSep",
    [HID_KEY_CURRENCY_UNIT] = "Currency",
    [HID_KEY_CURRENCY_SUBUNIT] = "CurrencySub",
    [HID_KEY_KEYPAD_LEFT_PARENTHESIS] = "KP(",
    [HID_KEY_KEYPAD_RIGHT_PARENTHESIS] = "KP)",
    [HID_KEY_KEYPAD_LEFT_BRACE] = "KP[",
    [HID_KEY_KEYPAD_RIGHT_BRACE] = "KP]",
    [HID_KEY_KEYPAD_TAB] = "KPTab",
    [HID_KEY_KEYPAD_BACKSPACE] = "KPBackspace",
    [HID_KEY_KEYPAD_A] = "KPA",
    [HID_KEY_KEYPAD_B] = "KPB",
    [HID_KEY_KEYPAD_C] = "KPC",
    [HID_KEY_KEYPAD_D] = "KPD",
    [HID_KEY_KEYPAD_E] = "KPE",
    [HID_KEY_KEYPAD_F] = "KPF",
    [HID_KEY_KEYPAD_XOR] = "KPXOR",
    [HID_KEY_KEYPAD_CARET] = "KP^",
    [HID_KEY_KEYPAD_PERCENT] = "KP%",
    [HID_KEY_KEYPAD_LESS_THAN] = "KP<",
    [HID_KEY_KEYPAD_GREATER_THAN] = "KP>",
    [HID_KEY_KEYPAD_AMPERSAND] = "KP&",
    [HID_KEY_KEYPAD_DOUBLE_AMPERSAND] = "KP&&",
    [HID_KEY_KEYPAD_VERTICAL_BAR] = "KP|",
    [HID_KEY_KEYPAD_DOUBLE_VERTICAL_BAR] = "KP||",
    [HID_KEY_KEYPAD_COLON] = "KP:",
    [HID_KEY_KEYPAD_HASH] = "KP#",
    [HID_KEY_KEYPAD_SPACE] = "KPSpace",
    [HID_KEY_KEYPAD_AT] = "KP@",
    [HID_KEY_KEYPAD_EXCLAMATION] = "KP!",
    [HID_KEY_KEYPAD_MEMORY_STORE] = "KPMemStore",
    [HID_KEY_KEYPAD_MEMORY_RECALL] = "KPMemRecall",
    [HID_KEY_KEYPAD_MEMORY_CLEAR] = "KPMemClear",
    [HID_KEY_KEYPAD_MEMORY_ADD] = "KPMem+",
    [HID_KEY_KEYPAD_MEMORY_SUBTRACT] = "KPMem-",
    [HID_KEY_KEYPAD_MEMORY_MULTIPLY] = "KPMem*",
    [HID_KEY_KEYPAD_MEMORY_DIVIDE] = "KPMem/",
    [HID_KEY_KEYPAD_PLUS_MINUS] = "K+-",
    [HID_KEY_KEYPAD_CLEAR] = "KPClear",
    [HID_KEY_KEYPAD_CLEAR_ENTRY] = "KPClearEntry",
    [HID_KEY_KEYPAD_BINARY] = "KPBin",
    [HID_KEY_KEYPAD_OCTAL] = "KPOct",
    [HID_KEY_KEYPAD_DECIMAL_2] = "KPDec2",
    [HID_KEY_KEYPAD_HEXADECIMAL] = "KPHex",
    [HID_KEY_CONTROL_LEFT] = "LCtrl",
    [HID_KEY_SHIFT_LEFT] = "LShift",
    [HID_KEY_ALT_LEFT] = "LAlt",
    [HID_KEY_GUI_LEFT] = "LGUI",
    [HID_KEY_CONTROL_RIGHT] = "RCtrl",
    [HID_KEY_SHIFT_RIGHT] = "RShift",
    [HID_KEY_ALT_RIGHT] = "RAlt",
    [HID_KEY_GUI_RIGHT] = "RGUI",
};

char const *matrix_keyname(uint16_t kc) {
    if (kc <= 0xFF) {
        return conv_table[kc];
    }

    return 0;
}
#endif
