// Copyright Nezametdinov E. Ildus 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <sys/select.h>
#include <sys/wait.h>

#include <assert.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <errno.h>

typedef struct {
    char* group_names[XkbNumKbdGroups];
} xkm_keyboard_desc;

static_assert(XkbNumKbdGroups == 4, "");

// Keyboard description initialization/destruction functions.
//

static xkm_keyboard_desc
xkm_initialize_keyboard_desc(Display* dpy);

static void
xkm_free_keyboard_desc(xkm_keyboard_desc* x);

//
// Implementation of the keyboard description initialization/destruction
// functions.
//

xkm_keyboard_desc
xkm_initialize_keyboard_desc(Display* dpy) {
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

    xkm_keyboard_desc r = {};
    for(size_t i = 0; i < XkbNumKbdGroups; ++i) {
        if(kb_desc->names->groups[i] == 0) {
            break;
        }

        r.group_names[i] = XGetAtomName(dpy, kb_desc->names->groups[i]);
    }

    XkbFreeKeyboard(kb_desc, 0, True);
    return r;
}

void
xkm_free_keyboard_desc(xkm_keyboard_desc* x) {
    for(size_t i = 0; i < XkbNumKbdGroups; ++i) {
        XFree(x->group_names[i]);
    }

    *x = (xkm_keyboard_desc){};
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
        static unsigned long const mask = XkbGroupStateMask | XkbGroupBaseMask |
                                          XkbGroupLatchMask | XkbGroupLockMask;
        if(!XkbSelectEventDetails(
               dpy, XkbUseCoreKbd, XkbStateNotify, mask, mask)) {
            fprintf(stderr, "%s: %s\n", __func__, "Failed to set event mask");
            return EXIT_FAILURE;
        }
    }

    // Obtain keyboard description.
    xkm_keyboard_desc kb_desc = xkm_initialize_keyboard_desc(dpy);

    // Receive and handle events.
    unsigned long long i = 0;
    for(XkbEvent event;;) {
        XNextEvent(dpy, &event.core);
        if(XFilterEvent(&event.core, None)) {
            continue;
        }

        if(event.core.type != xkb_event_code) {
            continue;
        }

        // Process the event.
        switch(event.any.xkb_type) {
            case XkbMapNotify:
                /* fall-through */
            case XkbNewKeyboardNotify:
                xkm_free_keyboard_desc(&kb_desc);
                kb_desc = xkm_initialize_keyboard_desc(dpy);
                break;

            case XkbStateNotify: {
                char* group_name =
                    kb_desc.group_names[event.state.locked_group];
                if(group_name != NULL) {
                    fprintf(stdout, "XkbStateNotify %lld, group: %s\n", i++,
                            group_name);
                } else {
                    fprintf(stdout, "XkbStateNotify %lld, group: %d\n", i++,
                            (int)event.state.locked_group);
                }
                break;
            }

            default:
                break;
        }
    }

    return EXIT_SUCCESS;
}
