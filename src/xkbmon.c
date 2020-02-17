// Copyright Nezametdinov E. Ildus 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <assert.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>

#include <unistd.h>
#include <errno.h>

// Unicode-related definitions.
//

enum { XKM_MAX_UTF8_ENCODED_CODE_POINT_SIZE = 4 };

enum {
    XKM_UTF8_DECODING_INCOMPLETE = 0xFFFFFFFE,
    XKM_UTF8_DECODING_ERROR = 0xFFFFFFFF
};

typedef struct {
    char32_t x;
    size_t n;
} xkm_utf8_decoding_result;

// Keyboard description-related definitions.
//

enum { XKM_MAX_GROUP_NAME_SIZE = 2 * XKM_MAX_UTF8_ENCODED_CODE_POINT_SIZE + 1 };

typedef char xkm_group_name[XKM_MAX_GROUP_NAME_SIZE];

typedef struct {
    unsigned current_group;
    xkm_group_name group_names[XkbNumKbdGroups];
} xkm_keyboard_desc;

// Unicode-related utility functions.
//

static xkm_utf8_decoding_result
xkm_utf8_decode(unsigned char const* string, size_t n);

// Keyboard description-related utility functions.
//

static xkm_keyboard_desc
xkm_obtain_keyboard_desc(Display* dpy);

static void
xkm_print_keyboard_desc(xkm_keyboard_desc const* kb_desc);

//
// Implementation of the Unicode-related utility functions.
//

xkm_utf8_decoding_result
xkm_utf8_decode(unsigned char const* string, size_t n) {
#define min_(a, b) (((a) < (b)) ? (a) : (b))
#define is_in_range_(x, a, b) (((x) >= (a)) && ((x) <= (b)))

    // Empty string is always incomplete.
    if((string == NULL) || (n == 0)) {
        return (xkm_utf8_decoding_result){.x = XKM_UTF8_DECODING_INCOMPLETE};
    }

    // Check if the string starts with an ASCII character.
    if(string[0] <= 0x7F) {
        return (xkm_utf8_decoding_result){.x = string[0], .n = 1};
    }

    // Check for disallowed first byte values (see the Table 3-7 in the Unicode
    // Standard for details).
    if(!is_in_range_(string[0], 0xC2, 0xF4)) {
        return (xkm_utf8_decoding_result){.x = XKM_UTF8_DECODING_ERROR, .n = 1};
    }

    static const struct entry {
        char32_t high;
        unsigned char ranges[3][2], n;
    } utf8_table[51] = {
        {0x00000080, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000000C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000100, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000140, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000180, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000001C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000200, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000240, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000280, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000002C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000300, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000340, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000380, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000003C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000400, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000440, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000480, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000004C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000500, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000540, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000580, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000005C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000600, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000640, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000680, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000006C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000700, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000740, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000780, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x000007C0, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 1},
        {0x00000000, {{0xA0, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00001000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00002000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00003000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00004000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00005000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00006000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00007000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00008000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00009000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x0000A000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x0000B000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x0000C000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x0000D000, {{0x80, 0x9F}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x0000E000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x0000F000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 2},
        {0x00000000, {{0x90, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 3},
        {0x00040000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 3},
        {0x00080000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 3},
        {0x000C0000, {{0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF}}, 3},
        {0x00100000, {{0x80, 0x8F}, {0x80, 0xBF}, {0x80, 0xBF}}, 3}};

    /* Decode the rest of the UTF-8 sequence. */ {
        struct entry const r = utf8_table[(*(string++)) - 0xC2];
        char32_t x = r.high, shift = r.n * 6;

        for(size_t i = 0, limit = min_(n - 1, r.n); i < limit; ++i) {
            if(!is_in_range_(string[i], r.ranges[i][0], r.ranges[i][1])) {
                return (xkm_utf8_decoding_result){
                    .x = XKM_UTF8_DECODING_ERROR, .n = (i + 1)};
            }

            shift -= 6;
            x |= ((char32_t)(string[i] & 0x3F)) << shift;
        }

        if(shift == 0) {
            return (xkm_utf8_decoding_result){.x = x, .n = (r.n + 1)};
        }
    }

#undef min_
#undef is_in_range_

    return (xkm_utf8_decoding_result){.x = XKM_UTF8_DECODING_INCOMPLETE};
}

//
// Implementation of the keyboard description-related utility functions.
//

xkm_keyboard_desc
xkm_obtain_keyboard_desc(Display* dpy) {
    // Obtain full keyboard description.
    XkbStateRec kb_state = {};
    if(XkbGetState(dpy, XkbUseCoreKbd, &kb_state) != Success) {
        fprintf(
            stderr, "%s: %s\n", __func__, "Failed to obtain keyboard state");
        exit(EXIT_FAILURE);
    }

    XkbDescPtr kb_desc = XkbAllocKeyboard();
    if(kb_desc == NULL) {
        fprintf(stderr, "%s: %s\n", __func__,
                "Failed to allocate a keyboard description");
        exit(EXIT_FAILURE);
    }

    if(XkbGetNames(dpy, XkbGroupNamesMask, kb_desc) != Success) {
        fprintf(stderr, "%s: %s\n", __func__, "Failed to obtain group names");
        exit(EXIT_FAILURE);
    }

    // Fill-in the group names.
    xkm_keyboard_desc r = {.current_group = kb_state.locked_group};
    for(size_t i = 0; i < XkbNumKbdGroups; ++i) {
        if(kb_desc->names->groups[i] == 0) {
            break;
        }

        // Get the full name.
        char* const group_name = XGetAtomName(dpy, kb_desc->names->groups[i]);
        size_t len = strlen(group_name);

        // Write the first two UTF-8-encoded characters from the group's full
        // name to the short group name's buffer.
        unsigned char* src_buf = (unsigned char*)group_name;
        unsigned char* dst_buf = (unsigned char*)r.group_names[i];

        for(size_t j = 0; (j < 2) && (len != 0); ++j) {
            xkm_utf8_decoding_result d = xkm_utf8_decode(src_buf, len);

            if(d.x == XKM_UTF8_DECODING_INCOMPLETE) {
                break;
            } else if(d.x == XKM_UTF8_DECODING_ERROR) {
                src_buf += d.n;
                len -= d.n;
                --j;

                continue;
            }

            memcpy(dst_buf, src_buf, d.n);

            src_buf += d.n;
            dst_buf += d.n;
            len -= d.n;
        }

        // Add the termination character to the resulting string.
        *dst_buf = '\0';

        // Free allocated memory.
        XFree(group_name);
    }

    XkbFreeKeyboard(kb_desc, 0, True);
    return r;
}

void
xkm_print_keyboard_desc(xkm_keyboard_desc const* kb_desc) {
    char const* group_name = kb_desc->group_names[kb_desc->current_group];
    if(group_name[0] != '\0') {
        fprintf(stdout, "%s\n", group_name);
    } else {
        fprintf(stdout, "G%d\n", (int)(kb_desc->current_group));
    }

    fflush(stdout);
}

//
// Program entry point.
//

int
main() {
    // Set locale-specific native environment for character handling functions.
    if(setlocale(LC_CTYPE, "") == NULL) {
        fprintf(stderr, "%s: %s\n", __func__,
                "Failed to set locale-specific native environment "
                "for character handling functions");
        return EXIT_FAILURE;
    }

    // Open a display and initialize XKB extension.
    int xkb_event_code = 0;
    Display* dpy =
        XkbOpenDisplay(NULL, &xkb_event_code, NULL, NULL, NULL, NULL);
    if(dpy == NULL) {
        fprintf(stderr, "%s: %s\n", __func__, "Failed to connect to X server");
        return EXIT_FAILURE;
    }

    /* Set event mask. */ {
        static unsigned long const map_notify_mask = XkbKeySymsMask;
        if(!XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbMapNotify,
                                  map_notify_mask, map_notify_mask)) {
            fprintf(stderr, "%s: %s\n", __func__, "Failed to set event mask");
            return EXIT_FAILURE;
        }

        static unsigned long const state_notify_mask =
            XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask |
            XkbGroupLockMask;
        if(!XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbStateNotify,
                                  state_notify_mask, state_notify_mask)) {
            fprintf(stderr, "%s: %s\n", __func__, "Failed to set event mask");
            return EXIT_FAILURE;
        }
    }

    // Obtain keyboard description.
    xkm_keyboard_desc kb_desc = xkm_obtain_keyboard_desc(dpy);
    xkm_print_keyboard_desc(&kb_desc);

    // Receive and handle events.
    unsigned long serial = 0;
    for(XkbEvent event;;) {
        XNextEvent(dpy, &event.core);
        if(XFilterEvent(&event.core, None)) {
            continue;
        }

        if(event.core.type != xkb_event_code) {
            continue;
        }

        switch(event.any.xkb_type) {
            case XkbMapNotify:
                // Note: prevent handling the same event by examining its serial
                // number.
                if(event.map.serial != serial) {
                    kb_desc = xkm_obtain_keyboard_desc(dpy);
                    xkm_print_keyboard_desc(&kb_desc);

                    serial = event.map.serial;
                }
                break;

            case XkbStateNotify:
                kb_desc.current_group = event.state.locked_group;
                xkm_print_keyboard_desc(&kb_desc);
                break;

            default:
                break;
        }
    }

    return EXIT_SUCCESS;
}
