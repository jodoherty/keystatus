/*
 * keystatus.c - Report keyboard status for i3blocks
 *
 * Copyright © 2024 James O'Doherty <james@theodohertyfamily.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>. 
 *
 * Example i3blocks config:
 *
 * [keystatus]
 * command=keystatus
 * label=KEY
 * interval=persist
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

struct ModifierEntry {
  int value;
  const char *latched;
  const char *locked;
};

struct ModifierEntry mods[] = {
  {1, "shift", "SHIFT"},
  {1<<2, "ctrl", "CTRL"},
  {1<<3, "alt", "ALT"},
  {1<<6, "super", "SUPER"},
  {0, NULL, NULL},
};

static Display *open_default_display(int *event_rtrn) {
  int error_retrn;
  int major_in_out = XkbMajorVersion;
  int minor_in_out = XkbMinorVersion;
  int reason_rtrn;
  Display *display = XkbOpenDisplay(NULL, event_rtrn, &error_retrn, &major_in_out, &minor_in_out, &reason_rtrn);
  if (display == NULL) {
    fprintf(stderr, "Failed to initialize Xkb extension for display %s\n", getenv("DISPLAY"));
    exit(1);
  }
  return display;
}

static void print_word(const char *word, bool *first) {
  if (!*first) {
    putc(' ', stdout);
  } else {
    *first = false;
  }
  printf("%s", word);
}

int main(int argc, char *argv[]) {
  int xkb_event_type;
  Display *display = open_default_display(&xkb_event_type);

  XkbDescPtr xkb = XkbGetKeyboard(display, XkbControlsMask, XkbUseCoreKbd);
  if (xkb == 0) {
    fprintf(stderr, "XkbGetKeyboard() failed for display '%s'\n", getenv("DISPLAY"));
    exit(1);
  }

  if (!XkbSelectEvents(display, XkbUseCoreKbd, XkbStateNotifyMask | XkbControlsNotifyMask | XkbAccessXNotifyMask, XkbStateNotifyMask | XkbControlsNotifyMask | XkbAccessXNotifyMask)) {
    fprintf(stderr, "XkbSelectEvents() failed\n");
    exit(1);
  }

  while (true) {
    bool is_first = true;
    /* Get modifier state */
    XkbStateRec state;
    XkbGetState(display, XkbUseCoreKbd, &state);
    for (int i=0; mods[i].value != 0; ++i) {
      if (state.latched_mods & mods[i].value) {
        print_word(mods[i].latched, &is_first);
      } else if (state.locked_mods & mods[i].value) {
        print_word(mods[i].locked, &is_first);
      }
    }
    /* Get accessibility state */
    Status status = XkbGetControls(display, XkbAllControlsMask, xkb);
    if (status != Success) {
      fprintf(stderr, "XkbGetControls() returned %d", status);
      exit(1);
    } if (xkb->ctrls->enabled_ctrls & XkbStickyKeysMask) {
      print_word("sticky", &is_first);
    }
    if (xkb->ctrls->enabled_ctrls & XkbAccessXKeysMask) {
      print_word("accessx", &is_first);
    }
    putc('\n', stdout);
    fflush(stdout);

    /* Wait for Xkb events */
    while (true) {
      XkbEvent e;
      XNextEvent(display, &e.core);
      if (e.type == xkb_event_type) {
        if (e.any.xkb_type == XkbStateNotify) {
          if (e.state.mods == state.mods &&
              e.state.base_mods == state.base_mods &&
              e.state.latched_mods == state.latched_mods &&
              e.state.locked_mods == state.locked_mods) {
            // No modifier state actually changed, so continue waiting
            continue;
          }
        }
        /* Consume any other events before we loop back around. */
        while (XPending(display)) {
          XNextEvent(display, &e.core);
        }
        break;
      }
    }
  }

  XCloseDisplay(display);
  return 0;
}
