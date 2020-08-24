/* This is full screen window manager, based on a previous work of mine, which it
 * was written in SLang programming language as a slang module.

 * Initially that code was derived from dminiwm, see at:
 * https://github.com/moetunes/dminiwm
 * so please see LICENSE.
 *
 * Some code in this unit, is also from the same author and specifically from the
 * bipolarbar project at:
 * https://github.com/moetunes/bipolarbar
 *
 * Many thanks for all.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <X11/XKBlib.h>

#include <errno.h>

#include "c.h"
#include <libwm.h>

#define CLEANMASK(mask) (mask & ~($my(numlockmask) | LockMask))

private void wm_maprequest (XEvent *);
private void wm_keypress (XEvent *);
private void wm_unmapnotify (XEvent *);
private void wm_configurerequest (XEvent *);
private void wm_destroynotify (XEvent *);
private void wm_unmapnotify (XEvent *);

static void (*events[LASTEvent])(XEvent *e) = {
  [KeyPress] = wm_keypress,
  [MapRequest] = wm_maprequest,
  [UnmapNotify] = wm_unmapnotify,
  [DestroyNotify] = wm_destroynotify,
  [ConfigureRequest] = wm_configurerequest
};

NewType (client,
  client_t
    *next,
    *prev;

  Window win;

  int
    x,
    y,
    order,
    width,
    height;
);

NewType (desktop,
  int
    numwins,
    mode,
    growth;

  client_t
    *head,
    *current,
    *transient;
);

NewType (OnMap,
  OnMap_t *next;
  char *class;
  int
    desk,
    follow;
);

NewType (positional,
  positional_t *next;
  char *class;

  int
    x,
    y,
    width,
    height;
);

NewType (wm_key,
  wm_key_t *next;
  int modifier;
  KeySym keysym;
);

static const char *def_font =
  "-misc-fixed-medium-r-*-*-32-*-*-*-*-*-*-*";

NewType (font,
  XFontStruct *font;          /* font structure */
  XFontSet fontset;           /* fontset structure */
  int height;                 /* height of the font */
  int width;
  uint fh;                    /* Y coordinate to draw characters */
  int ascent;
  int descent;
);

NewProp (wm,
  Window root;
  Display *dpy;

  desktop_t *desktops;
  wm_key_t  *keys;
  OnMap_t   *onmap;
  positional_t *positional;

  Class (wm) *__Me__;

  client_t
    *head,
    *current,
    *transient;

  Atom
    *protocols,
    wm_delete_window,
    protos;

  int
    sh,
    sw,
    bdw,
    mode,
    screen,
    growth,
    desknum,
    numwins,
    bool_quit,
    win_focus,
    win_unfocus,
    numlockmask,
    current_desktop,
    previous_desktop;

  XWindowAttributes attr;

  font_t *font;

  OnStartup_cb OnStartup;
  OnKeypress_cb OnKeypress;
);

static Class (wm) *WM;

int (*xerrorxlib)(Display *, XErrorEvent *);

private int xerror (Display *dpy, XErrorEvent *ee) {
  if (ee->error_code is BadWindow or (ee->request_code is X_SetInputFocus and ee->error_code is BadMatch)
      or (ee->request_code is X_PolyText8 and ee->error_code is BadDrawable)
      or (ee->request_code is X_PolyFillRectangle and ee->error_code is BadDrawable)
      or (ee->request_code is X_PolySegment and ee->error_code is BadDrawable)
      or (ee->request_code is X_ConfigureWindow and ee->error_code is BadMatch)
      or (ee->request_code is X_GrabKey and ee->error_code is BadAccess)
      or (ee->request_code is X_CopyArea and ee->error_code is BadDrawable))
    return 0;

  if (ee->error_code is BadAccess)
    exit (1);

  return xerrorxlib (dpy, ee);
}

private void wm_keypress (XEvent *e) {
  wm_T *this = WM;
  KeySym keysym;
  XKeyEvent *ev = &e->xkey;

  keysym = XkbKeycodeToKeysym ($my(dpy), (KeyCode)ev->keycode, 0, 0);

  wm_key_t *k;

  for (k = $my(keys); k; k = k->next)
    if (keysym is k->keysym and CLEANMASK(k->modifier) is (int) CLEANMASK(ev->state)) {
      char *kstr = XKeysymToString (k->keysym);
      if (NULL isnot $my(OnKeypress))
        $my(OnKeypress) (this, k->modifier, kstr);
      break;
   }
}

private void wm_configurerequest (XEvent *e) {
  wm_T *this = WM;
  XConfigureRequestEvent *ev = &e->xconfigurerequest;
  XWindowChanges wc;

  wc.x = ev->x;
  wc.y = ev->y;
  wc.width = (ev->width < $my(sw) - $my(bdw)) ? ev->width : $my(sw) + $my(bdw);
  wc.height = (ev->height < $my(sh) - $my(bdw)) ? ev->height : $my(sh) + $my(bdw);
  wc.border_width = 0;
  wc.sibling = ev->above;
  wc.stack_mode = ev->detail;
  XConfigureWindow ($my(dpy), ev->window, ev->value_mask, &wc);
  XSync ($my(dpy), False);
}

private void wm_maprequest (XEvent *e) {
  wm_T *this = WM;
  XMapRequestEvent *ev = &e->xmaprequest;

  XGetWindowAttributes ($my(dpy), ev->window, &$my(attr));

  if ($my(attr).override_redirect) return;

  client_t *c;

  for (c = $my(head); c; c = c->next)
    if (ev->window is c->win) {
      XMapWindow ($my(dpy), ev->window);
      return;
    }

  Window trans = None;

  if (XGetTransientForHint ($my(dpy), ev->window, &trans) and trans isnot None) {
    self(add_window, ev->window, 1, NULL);

    if (($my(attr).y + $my(attr).height) > $my(sh))
      XMoveResizeWindow ($my(dpy), ev->window, $my(attr).x, 0, $my(attr).width , $my(attr).height - 10);

    XSetWindowBorderWidth ($my(dpy), ev->window, $my(bdw));
    XSetWindowBorder ($my(dpy), ev->window, $my(win_focus));
    XMapWindow ($my(dpy), ev->window);
    self(update_current);
    return;
  }

  if (FULLSCREEN_MODE is $my(mode) and $my(current) isnot NULL)
    XUnmapWindow ($my(dpy), $my(current)->win);

  int j = 0;
  int tmp = $my(current_desktop);

  OnMap_t *o;
  XClassHint ch = {0};

  if (XGetClassHint ($my(dpy), ev->window, &ch))
    for (o = $my(onmap); o; o = o->next)
      if ((strcmp (ch.res_class, o->class) is 0) or
          (strcmp (ch.res_name, o->class) is 0)) {
        self(save_desktop, tmp);
        self(select_desktop, o->desk - 1);

        for (c = $my(head); c; c = c->next)
          if (ev->window is c->win)
            ++j;

        if (j < 1)
          self(add_window, ev->window, 0, NULL);

        if (tmp is o->desk - 1) {
          self(tile);
          XMapWindow ($my(dpy), ev->window);
          self(update_current);
        } else
           self(select_desktop, tmp);

        if (o->follow and o->desk - 1 isnot $my(current_desktop)) {
          int desk = o->desk - 1;
          self(change_desktop, desk);
        }

        if (ch.res_class)
          XFree (ch.res_class);

        if (ch.res_name)
          XFree (ch.res_name);

        return;
      }

  if (ch.res_class)
    XFree (ch.res_class);

  if (ch.res_name)
    XFree (ch.res_name);

  self(add_window, ev->window, 0, NULL);

  if (FULLSCREEN_MODE is $my(mode))
    self(tile);
  else
    XMapWindow ($my(dpy), ev->window);

  self(update_current);
}

private void wm_destroynotify (XEvent *e) {
  wm_T *this = WM;
  int tmp = $my(current_desktop);
  client_t *c;
  XDestroyWindowEvent *ev = &e->xdestroywindow;

  self(save_desktop, tmp);

  for (int i = $my(current_desktop); i < $my(current_desktop) + $my(desknum); ++i) {
    self(select_desktop, i % $my(desknum));

    for (c = $my(head); c; c = c->next)
      if (ev->window is c->win) {
        self(remove_window, ev->window, 0, 0);
        self(select_desktop, tmp);
        return;
      }

    if ($my(transient) isnot NULL)
      for (c = $my(transient); c; c = c->next)
        if (ev->window is c->win) {
          self(remove_window, ev->window, 0, 1);
          self(select_desktop, tmp);
          return;
        }
  }

  self(select_desktop, tmp);
}

private void wm_unmapnotify (XEvent *e) {
  wm_T *this = WM;
  XUnmapEvent *ev = &e->xunmap;
  client_t *c;

  if (ev->send_event is 1)
    for (c = $my(head); c; c = c->next)
      if (ev->window is c->win) {
        self(remove_window, ev->window, 1, 0);
        return;
      }
}

private void wm_input_window (wm_T *this, char *header, Input_cb cb) {
  int cur_desktop = self(get.current_desktop);
  self(change_desktop, $my(desknum) - 1);
  self(get.font);

  Window root = DefaultRootWindow ($my(dpy));
  int screen  = XDefaultScreen ($my(dpy));
  int width   = XDisplayWidth ($my(dpy), screen);
  int height  = $my(font)->height;
  int y       = 0;

  $my(font)->fh = ((height - $my(font)->height) / 2) + $my(font)->ascent;

  XGCValues values;
  values.background = self(get.color, colorblack);
  values.foreground = self(get.color, colorwhite);
  values.line_width = 4;
  values.line_style = LineSolid;

  GC gc;
  if ($my(font)->fontset)
    gc = XCreateGC ($my(dpy), root, GCBackground|GCForeground|GCLineWidth|GCLineStyle, &values);
  else {
    values.font = $my(font)->font->fid;
    gc = XCreateGC ($my(dpy), root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
  }

  Drawable winbar;
  winbar = XCreatePixmap ($my(dpy), root, width, height, DefaultDepth ($my(dpy), screen));
  XFillRectangle ($my(dpy), winbar, gc, 0, 0, width, height);

  Window win = XCreateSimpleWindow($my(dpy), root, 0, y, width, height, 0,
      values.foreground, values.background);

  XSetWindowAttributes attr;
  attr.override_redirect = True;
  XChangeWindowAttributes ($my(dpy), win, CWOverrideRedirect, &attr);

  XStoreName ($my(dpy), win, "InputWindow");
  XSelectInput ($my(dpy), win, ExposureMask | KeyPressMask);
  XMapWindow ($my(dpy), win);
  XSetInputFocus ($my(dpy), win, RevertToParent, CurrentTime);
  XRaiseWindow ($my(dpy), win);
  XFlush ($my(dpy));

  Atom WM_DELETE_WINDOW = XInternAtom ($my(dpy), "WM_DELETE_WINDOW", True);
  XSetWMProtocols ($my(dpy), win, &WM_DELETE_WINDOW, 1);

  XEvent ev;
  KeySym keysym;
  int retval = 0;
  char buffer[] = "    ";
  int buf_len = 4;
  int num_chars = 0;

  while (1) {
    XNextEvent ($my(dpy), &ev);
    switch (ev.type) {
      case Expose:
        if (header isnot NULL) {
          if ($my(font)->fontset)
            XmbDrawString ($my(dpy), win, $my(font)->fontset,
                 gc, 1, $my(font)->fh, header, bytelen (header));
          else
            XDrawString($my(dpy), win, gc, 1, 4, header, bytelen(header));
        }
        break;

      case ClientMessage:
        if (ev.xclient.data.l[0] is (int) WM_DELETE_WINDOW)
          goto theend;

        break;

      case KeyPress:
        keysym = XLookupKeysym(&(ev.xkey), 0);
        num_chars = XLookupString(&(ev.xkey), buffer, buf_len, &keysym, NULL);
        retval = cb (this, buffer, num_chars, ev.xkey.keycode, keysym);
        if (1 is retval) continue;
        if (0 is retval) goto theend;
        break;
    }
  }

theend:
  XDestroyWindow ($my(dpy), win);
  self(change_desktop, cur_desktop);
}

private char **wm_get_desk_class_names (wm_T *this, int desk) {
  if (desk < 0 or desk >= $my(desknum) - 1) return NULL;

  ifnot ($my(desktops)[desk].numwins)
    return NULL;

  int num_wins = $my(desktops)[desk].numwins;

  char **classes = Alloc ((num_wins * sizeof (char *)) + 1);

  client_t *c;
  XClassHint hint;

  int idx = 0;
  for (c = $my(desktops)[desk].head; c; c = c->next) {
    if (XGetClassHint ($my(dpy), c->win, &hint)) {
      if (hint.res_name)
        //classes[idx] = strdup (hint.res_name);
        // will be free'd by the caller,
        // as this way, we avoid a rellocation
        classes[idx++] = hint.res_name;

      if (hint.res_class)
        XFree (hint.res_class);
    }
  }

  classes[idx] = NULL;
  return classes;
}

private void wm_get_font (wm_T *this) {
  if (NULL is $my(dpy)) return;

  ifnot (NULL is $my(font)) return;

  $my(font) = AllocType (font);

  char
    *def,
    **missing = NULL;

  int n;

  if (bytelen (def_font) > 0)
    $my(font)->fontset = XCreateFontSet ($my(dpy), (char *)def_font,
        &missing, &n, &def);

  if (missing) XFreeStringList (missing);

  if ($my(font)->fontset) {
    XFontStruct **xfonts;
    char **font_names;
    $my(font)->ascent = $my(font)->descent = 0;
    n = XFontsOfFontSet ($my(font)->fontset, &xfonts, &font_names);
    for (int i = 0; i < n; ++i) {
      if ($my(font)->ascent < (*xfonts)->ascent)
        $my(font)->ascent = (*xfonts)->ascent;
      if ($my(font)->descent < (*xfonts)->descent)
        $my(font)->descent = (*xfonts)->descent;
      ++xfonts;
    }

    $my(font)->width = XmbTextEscapement ($my(font)->fontset,
       " ", 1);
  } else {
    if (0 is ($my(font)->font = XLoadQueryFont ($my(dpy), def_font)))
      $my(font)->font = XLoadQueryFont ($my(dpy), "fixed");

    $my(font)->ascent = $my(font)->font->ascent;
    $my(font)->descent = $my(font)->font->descent;
    $my(font)->width = XTextWidth ($my(font)->font, " ", 1);
  }

  $my(font)->height = $my(font)->ascent + $my(font)->descent;
}

private unsigned long wm_get_color (wm_T *this, const char *color) {
  XColor c;
  Colormap map = DefaultColormap ($my(dpy), $my(screen));

  ifnot (XAllocNamedColor ($my(dpy), map, color, &c, &c))
    fprintf (stderr, "WM: Error parsing color\n");

  return c.pixel;
}

private int wm_get_current_desktop (wm_T *this) {
  return $my(current_desktop);
}

private int wm_get_desk_numwin (wm_T *this, int idx) {
  if (0 > idx or idx >= $my(desknum)) return 0;
  return $my(desktops)[idx].numwins;
}

private int wm_get_desknum (wm_T *this) {
  return $my(desknum) - 1;
}

private int wm_get_previous_desktop (wm_T *this) {
  return $my(previous_desktop);
}

private void wm_sigchld_handler (int);
private void wm_sigchld_handler (int sig) {
  if (signal (sig, wm_sigchld_handler) is SIG_ERR) {
    fprintf (stderr, "Can't install SIGCHLD handler\n");
    exit (1);
  }

  while (0 < waitpid (-1, NULL, WNOHANG));
}

private void wm_change_desktop (wm_T *this, int desk) {
  if (desk is $my(current_desktop)) return;
  int tmp = $my(current_desktop);

  self(save_desktop, $my(current_desktop));
  $my(previous_desktop) = $my(current_desktop);

  self(select_desktop, desk);

  client_t *c;
  if ($my(head) isnot NULL) {
    if (STACK_MODE is $my(mode))
      for (c = $my(head); c; c = c->next)
        XMapWindow ($my(dpy), c->win);

    self(tile);
  }

  if ($my(transient) isnot NULL)
    for (c = $my(transient); c; c = c->next)
      XMapWindow ($my(dpy), c->win);

  self(select_desktop, tmp);

  if ($my(transient) isnot NULL)
    for (c = $my(transient); c; c = c->next)
       XUnmapWindow ($my(dpy), c->win);

  if ($my(head) isnot NULL)
    for (c = $my(head); c; c = c->next)
      XUnmapWindow ($my(dpy), c->win);

  self(select_desktop, desk);
  self(update_current);
}

private void wm_follow_client_to_desktop  (wm_T *this, int desk) {
  self(client_to_desktop, desk);
  self(change_desktop, desk);
}

private void wm_client_to_desktop  (wm_T *this, int desk) {
  if (desk is $my(current_desktop) or $my(current) is NULL)
    return;

  client_t *tmp = $my(current);
  int tmp2 = $my(current_desktop);

  self(remove_window, $my(current)->win, 1, 0);
  self(select_desktop, desk);

  self(add_window, tmp->win, 0, tmp);
  self(save_desktop, desk);
  self(select_desktop, tmp2);
}

private void wm_change_mode (wm_T *this, int mode) {
  if ($my(mode) is mode) return;

  client_t *c;

  $my(growth) = 0;

  if (FULLSCREEN_MODE is $my(mode) and $my(current) isnot NULL and $my(head)->next isnot NULL) {
    XUnmapWindow ($my(dpy), $my(current)->win);

    for (c = $my(head); c; c = c->next)
      XMapWindow ($my(dpy), c->win);
  }

  $my(mode) = mode;

  if (FULLSCREEN_MODE is $my(mode) and $my(current) isnot NULL and $my(head)->next isnot NULL)
    for (c = $my(head); c; c = c->next)
      XUnmapWindow ($my(dpy), c->win);

  self(tile);
  self(update_current);
}

private void wm_save_desktop (wm_T *this, int i) {
  $my(desktops)[i].numwins = $my(numwins);
  $my(desktops)[i].mode = $my(mode);
  $my(desktops)[i].growth = $my(growth);
  $my(desktops)[i].head = $my(head);
  $my(desktops)[i].current = $my(current);
  $my(desktops)[i].transient = $my(transient);
}

private void wm_next_win (wm_T *this) {
  if ($my(numwins) < 2) return;

  $my(current) = ($my(current)->next is NULL) ? $my(head) : $my(current)->next;

  if (FULLSCREEN_MODE is $my(mode))
    self(tile);

  self(update_current);
}

private void wm_prev_win (wm_T *this) {
  if ($my(numwins) < 2) return;

  client_t *c;

  if ($my(current)->prev is NULL)
    for (c = $my(head); c->next; c = c->next);
  else
    c = $my(current)->prev;

  $my(current) = c;

  if (FULLSCREEN_MODE is $my(mode))
    self(tile);

  self(update_current);
}

private void wm_resize_side_way (wm_T *this, int inc) {
  if ($my(mode) is STACK_MODE and $my(current) isnot NULL) {
    $my(current)->width += inc;
    XMoveResizeWindow ($my(dpy), $my(current)->win, $my(current)->x, $my(current)->y,
        $my(current)->width + inc, $my(current)->height);
  }
}

private void wm_resize_stack (wm_T *this, int inc) {
  if ($my(mode) is STACK_MODE and $my(current) isnot NULL) {
    $my(current)->height += inc;
    XMoveResizeWindow ($my(dpy), $my(current)->win, $my(current)->x, $my(current)->y,
        $my(current)->width, $my(current)->height + inc);
  }
}

private void wm_move_stack (wm_T *this, int inc) {
  if ($my(mode) is STACK_MODE and $my(current) isnot NULL) {
    $my(current)->y += inc;
    XMoveResizeWindow ($my(dpy), $my(current)->win, $my(current)->x, $my(current)->y, $my(current)->width, $my(current)->height);
  }
}

private void wm_move_side_way (wm_T *this, int inc) {
  if ($my(mode) is STACK_MODE and $my(current) isnot NULL) {
    $my(current)->x += inc;
    XMoveResizeWindow ($my(dpy), $my(current)->win, $my(current)->x, $my(current)->y, $my(current)->width, $my(current)->height);
  }
}

private void wm_add_window (wm_T *this, Window w, int tw, client_t *cl) {
  client_t
    *c,
    *t,
    *dummy = $my(head);

  if (cl isnot NULL)
    c = cl;
  else
    c = Alloc (sizeof (client_t));

  if (tw is 0 and cl is NULL) {
    XClassHint ch = {0};
    int j = 0;
    positional_t *p;

    if (XGetClassHint ($my(dpy), w, &ch)) {
      for (p = $my(positional); p; p = p->next)
        if ((strcmp (ch.res_class, p->class) is 0) or
            (strcmp (ch.res_name, p->class) is 0)) {
          XMoveResizeWindow ($my(dpy), w, p->x, p->y, p->width, p->height);
          ++j;
          break;
        }

      if (ch.res_class)
        XFree (ch.res_class);

      if (ch.res_name)
        XFree (ch.res_name);
    }

    if (j < 1) {
      XGetWindowAttributes ($my(dpy), w, &$my(attr));
      XMoveResizeWindow ($my(dpy), w, $my(attr).x, $my(attr).y,
          $my(attr).width + 80, $my(attr).height - 12);
      // XMoveWindow ($my(dpy), w, $my(sw) / 2 - ($my(attr).width / 2), $my(sh) / 2 - ($my(attr).height / 2));
    }

    XGetWindowAttributes ($my(dpy), w, &$my(attr));
    c->x = $my(attr).x;
    c->y = $my(attr).y;
    c->width = $my(attr).width;
    c->height = $my(attr).height;
  }

  c->win = w;
  c->order = 0;

  if (tw is 1)
    dummy = $my(transient);

  for (t = dummy; t; t = t->next)
    ++t->order;

  if (dummy is NULL) {
    c->next = NULL;
    c->prev = NULL;
    dummy = c;
  } else {
    c->prev = NULL;
    c->next = dummy;
    c->next->prev = c;
    dummy = c;
  }

  if (tw is 1) {
    $my(transient) = dummy;
    self(save_desktop, $my(current_desktop));
    return;
  } else
    $my(head) = dummy;

  $my(current) = c;
  $my(numwins) += 1;
  $my(growth) = ($my(growth) > 0) ? $my(growth) * ($my(numwins) - 1) / $my(numwins) : 0;
  self(save_desktop, $my(current_desktop));
}

private void wm_update_current (wm_T *this) {
  if ($my(head) is NULL) return;

  int border = (($my(head)->next is NULL and $my(mode) is FULLSCREEN_MODE) or
     ($my(mode) is FULLSCREEN_MODE)) ? 0 : $my(bdw);

  client_t *c, *d;
  for (c = $my(head); c->next; c = c->next);

  for (d = c; d; d = d->prev) {
    XSetWindowBorderWidth ($my(dpy), d->win, border);

    if (d isnot $my(current)) {
      if (d->order < $my(current)->order)
        ++d->order;

      XSetWindowBorder ($my(dpy), d->win, $my(win_unfocus));
    } else {
      XSetWindowBorder ($my(dpy), d->win, $my(win_focus));
      XSetInputFocus ($my(dpy), d->win, RevertToParent, CurrentTime);
      XRaiseWindow ($my(dpy), d->win);
    }
  }

  $my(current)->order = 0;

  if ($my(transient) isnot NULL) {
    for (c = $my(transient); c->next; c = c->next);

    for (d = c; d; d = d->prev)
      XRaiseWindow ($my(dpy), d->win);

    XSetInputFocus ($my(dpy), $my(transient)->win, RevertToParent, CurrentTime);
  }

  XSync ($my(dpy), False);
}

private void wm_remove_window (wm_T *this, Window w, int dr, int tw) {
  client_t
    *c,
    *t,
    *dummy;

  dummy = (tw is 1) ? $my(transient) : $my(head);

  for (c = dummy; c; c = c->next) {
    if (c->win is w) {
      if (c->prev is NULL and c->next is NULL)
        dummy = NULL;
      else if (c->prev is NULL)  {
        dummy = c->next;
        c->next->prev = NULL;
      } else if (c->next is NULL)
        c->prev->next = NULL;
      else {
        c->prev->next = c->next;
        c->next->prev = c->prev;
      }

      break;
    }
  }

  if (tw is 1) {
    $my(transient) = dummy;
    free (c);
    self(save_desktop, $my(current_desktop));
    self(update_current);
    return;
  } else {
    $my(head) = dummy;
    XUngrabButton ($my(dpy), AnyButton, AnyModifier, c->win);
    XUnmapWindow ($my(dpy), c->win);

    $my(numwins)--;

    if ($my(head) isnot NULL)  {
      for (t = $my(head); t; t = t->next) {
        if (t->order > c->order)
          --t->order;

        if (t->order is 0)
          $my(current) = t;
      }
    } else
      $my(current) = NULL;

    if (dr is 0)
      free (c);

    if ($my(numwins) < 3)
      $my(growth) = 0;

    self(save_desktop, $my(current_desktop));

    if (FULLSCREEN_MODE is $my(mode))
      self(tile);

    self(update_current);
    return;
  }

}

private void wm_tile (wm_T *this) {
  if ($my(head) is NULL) return;

  client_t *c;

  if (FULLSCREEN_MODE is $my(mode) and $my(head) isnot NULL and $my(head)->next is NULL)  {
    XMapWindow ($my(dpy), $my(current)->win);
    XMoveResizeWindow ($my(dpy), $my(head)->win, 0, 0, $my(sw) + $my(bdw), $my(sh) + $my(bdw));
  } else  {
    switch ($my(mode)) {
      case FULLSCREEN_MODE:
        XMoveResizeWindow ($my(dpy), $my(current)->win, 0, 0, $my(sw) + $my(bdw), $my(sh) + $my(bdw));
        XMapWindow ($my(dpy), $my(current)->win);
        break;
      case STACK_MODE:
        for (c = $my(head); c; c = c->next)
          XMoveResizeWindow ($my(dpy), c->win, c->x, c->y, c->width, c->height);
        break;
    }
  }
}

private void wm_kill_client (wm_T *this) {
  if ($my(head) is NULL) return;

  self(kill_client_now, $my(current)->win);
  self(remove_window, $my(current)->win, 0, 0);
}

private void wm_kill_client_now (wm_T *this, Window w) {
  int n, i;
  XEvent ev;

  if (XGetWMProtocols ($my(dpy), w, &$my(protocols), &n) isnot 0) {
    for (i = n; i >= 0; --i)
      if ($my(protocols)[i] is $my(wm_delete_window)) {
        ev.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.message_type = $my(protos);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = $my(wm_delete_window);
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent ($my(dpy), w, False, NoEventMask, &ev);
      }
  } else
    XKillClient ($my(dpy), w);

  XFree ($my(protocols));
}

private void wm_quit (wm_T *this) {
  client_t *c;

  for (int i = 0; i < $my(desknum); ++i) {
    if ($my(desktops)[i].head isnot NULL)
      self(select_desktop, i);
    else
      continue;

    for (c = $my(head); c; c = c->next)
      self(kill_client_now, c->win);
  }

  XClearWindow ($my(dpy), $my(root));
  XUngrabKey ($my(dpy), AnyKey, AnyModifier, $my(root));
  XSync ($my(dpy), False);
  XSetInputFocus ($my(dpy), $my(root), RevertToPointerRoot, CurrentTime);
  $my(bool_quit) = 1;
}

private void wm_spawn (wm_T *this, char **command) {
  if (fork () == 0) {
    if (fork () == 0) {
      if ($my(dpy))
        close (ConnectionNumber ($my(dpy)));

      setsid ();

      execvp (command[0], command);
    }

    exit (0);
  }
}

private void wm_select_desktop (wm_T *this, int i) {
  $my(numwins) = $my(desktops)[i].numwins;
  $my(mode) = $my(desktops)[i].mode;
  $my(growth) = $my(desktops)[i].growth;
  $my(head) = $my(desktops)[i].head;
  $my(current) = $my(desktops)[i].current;
  $my(transient) = $my(desktops)[i].transient;
  $my(current_desktop) = i;
}

private void wm_grabkeys (wm_T *this) {
  $my(numlockmask) = 0;
  XModifierKeymap *modmap = XGetModifierMapping ($my(dpy));

  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < modmap->max_keypermod; ++j) {
      if (modmap->modifiermap[i * modmap->max_keypermod + j] is XKeysymToKeycode ($my(dpy), XK_Num_Lock))
        $my(numlockmask) = (1 << i);
    }
  }

  XFreeModifiermap (modmap);

  XUngrabKey ($my(dpy), AnyKey, AnyModifier, $my(root));

  KeyCode code;
  wm_key_t *k;

  for (k = $my(keys); k; k = k->next) {
    code = XKeysymToKeycode ($my(dpy), k->keysym);
    XGrabKey ($my(dpy), code, k->modifier, $my(root), True, GrabModeAsync, GrabModeAsync);
    XGrabKey ($my(dpy), code, k->modifier | LockMask, $my(root), True, GrabModeAsync, GrabModeAsync);
    XGrabKey ($my(dpy), code, k->modifier | $my(numlockmask), $my(root), True, GrabModeAsync, GrabModeAsync);
    XGrabKey ($my(dpy), code, k->modifier | $my(numlockmask) | LockMask, $my(root), True, GrabModeAsync, GrabModeAsync);
    }
}

private void wm_event_loop (wm_T *this) {
  XEvent ev;

  while (0 is $my(bool_quit) and 0 is XNextEvent ($my(dpy), &ev))
    if (events[ev.type])
      events[ev.type](&ev);
}

private int wm_startx (wm_T *this) {
  wm_sigchld_handler (SIGCHLD);

  if (NULL is ($my(dpy) = XOpenDisplay (NULL))) {
    fprintf (stderr, "Cannot open display\n");
    return NOTOK;
  }

  $my(screen) = DefaultScreen ($my(dpy));
  $my(root) = RootWindow ($my(dpy), $my(screen));
  $my(bdw) = BORDER_WIDTH;
  $my(sw) = XDisplayWidth ($my(dpy), $my(screen)) - $my(bdw);
  $my(sh) = XDisplayHeight ($my(dpy), $my(screen)) - $my(bdw);
  $my(win_focus) = self(get.color, FOCUS);
  $my(win_unfocus) = self(get.color, UNFOCUS);

  if (NULL is setlocale (LC_ALL, ""))
    fprintf (stderr, "failed to set locale\n");

  self(grabkeys);
  self(select_desktop, 0);

  $my(wm_delete_window) = XInternAtom ($my(dpy), "WM_DELETE_WINDOW", False);
  $my(protos) = XInternAtom ($my(dpy), "WM_PROTOCOLS", False);
  XSelectInput ($my(dpy), $my(root), SubstructureNotifyMask|SubstructureRedirectMask);

  $my(bool_quit) = 0;

  ifnot (NULL is $my(OnStartup))
    $my(OnStartup) (this);

  wm_event_loop (this);

  XCloseDisplay ($my(dpy));

  return OK;
}

private void wm_set_on_startup_cb (wm_T *this, OnStartup_cb cb) {
  $my(OnStartup) = cb;
}

private void wm_set_on_keypress_cb (wm_T *this, OnKeypress_cb cb) {
  $my(OnKeypress) = cb;
}

private void wm_set_onmap (wm_T *this, char *class, int desk, int follow) {
  OnMap_t *s = AllocType (OnMap);
  s->class = strdup (class);
  s->desk = desk;
  s->follow = follow;
  s->next = $my(onmap);
  $my(onmap) = s;
}

private void wm_set_positional (wm_T *this, char *class, int x, int y,
                                               int width, int height) {
  positional_t *s = AllocType (positional);
  s->class = strdup (class);
  s->x = x;
  s->y = y;
  s->width = width;
  s->height = height;
  s->next = $my(positional);
  $my(positional) = s;
}

private void wm_set_mode (wm_T *this, int desk, int mode) {
  desk--;
  if (desk < 0 or desk >= $my(desknum) - 1) return;
  $my(desktops)[desk].mode = mode;
}

private void wm_set_key (wm_T *this, char *keysym, int modifier) {
  wm_key_t *s = AllocType (wm_key);
  s->keysym = XStringToKeysym (keysym);
  s->modifier = modifier;
  s->next = $my(keys);
  $my(keys) = s;
}

private void wm_set_desktops (wm_T *this, int num) {
  $my(desknum) = num + 1;

  self(release.desktops);

  $my(desktops) = Alloc (sizeof (desktop_t) * $my(desknum));

  for (int i = 0; i < $my(desknum); i++) {
    $my(desktops)[i].growth = 0;
    $my(desktops)[i].numwins = 0;
    $my(desktops)[i].head = NULL;
    $my(desktops)[i].current = NULL;
    $my(desktops)[i].transient = NULL;
    $my(desktops)[i].mode = DEFAULT_MODE;
  }
}

private void wm_release_font (wm_T *this) {
  if (NULL is $my(font)) return;
  free ($my(font));
}

private void wm_release_keys (wm_T *this) {
  if (NULL is $my(keys)) return;
  wm_key_t *s = $my(keys);
  while (s) {
    wm_key_t *next = s->next;
    free (s);
    s = next;
  }
}

private void wm_release_onmap (wm_T *this) {
  if (NULL is $my(onmap)) return;

  OnMap_t *s = $my(onmap);
  while (s) {
    OnMap_t *next = s->next;
    free (s->class);
    free (s);
    s = next;
  }
}

private void wm_release_positional (wm_T *this) {
  if (NULL is $my(positional)) return;
  positional_t *s = $my(positional);
  while (s) {
    positional_t *next = s->next;
    free (s->class);
    free (s);
    s = next;
  }
}

private void wm_release_desktops (wm_T *this) {
  if (NULL is $my(desktops)) return;
  for (int i = 0; i < $my(desknum); i++) {
    client_t *c = $my(desktops)[i].head;
    while (c) {
      client_t *next = c->next;
      free (c);
      c = next;
    }
  }

  free ($my(desktops));
  $my(desktops) = NULL;
}

private void wm_init (wm_T *this, int desknum) {
  XSetErrorHandler (xerror);
  self(set.desktops, desknum);
}

public Class (wm) *__init_wm__ (void) {
  Class (wm) *this = AllocClass (wm);

  *this = ClassInit (wm,
    .prop = this->prop,
    .user_obj = NULL,
    .user_string = NULL,
    .user_int = -1,
    .self = SelfInit (wm,
      .init = wm_init,
      .quit = wm_quit,
      .tile = wm_tile,
      .spawn = wm_spawn,
      .xfree = XFree,
      .startx = wm_startx,
      .grabkeys = wm_grabkeys,
      .prev_win = wm_prev_win,
      .next_win = wm_next_win,
      .add_window = wm_add_window,
      .kill_client = wm_kill_client,
      .change_mode = wm_change_mode,
      .save_desktop = wm_save_desktop,
      .input_window = wm_input_window,
      .remove_window = wm_remove_window,
      .change_desktop = wm_change_desktop,
      .update_current = wm_update_current,
      .select_desktop = wm_select_desktop,
      .kill_client_now = wm_kill_client_now,
      .client_to_desktop = wm_client_to_desktop,
      .follow_client_to_desktop = wm_follow_client_to_desktop,
      .get = SubSelfInit (wm, get,
        .font = wm_get_font,
        .color = wm_get_color,
        .desknum = wm_get_desknum,
        .desk_numwin = wm_get_desk_numwin,
        .current_desktop = wm_get_current_desktop,
        .previous_desktop = wm_get_previous_desktop,
        .desk_class_names = wm_get_desk_class_names
      ),
      .set = SubSelfInit (wm, set,
        .key = wm_set_key,
        .mode = wm_set_mode,
        .onmap = wm_set_onmap,
        .desktops = wm_set_desktops,
        .positional = wm_set_positional,
        .on_startup_cb = wm_set_on_startup_cb,
        .on_keypress_cb = wm_set_on_keypress_cb
      ),
      .move = SubSelfInit (wm, move,
        .stack = wm_move_stack,
        .side_way = wm_move_side_way
      ),
      .resize = SubSelfInit (wm, resize,
        .stack = wm_resize_stack,
        .side_way = wm_resize_side_way
      ),
      .release = SubSelfInit (wm, release,
        .font = wm_release_font,
        .keys = wm_release_keys,
        .onmap = wm_release_onmap,
        .desktops = wm_release_desktops,
        .positional = wm_release_positional
      )
    )
  );

  $my(dpy) = NULL;
  $my(onmap) = NULL;
  $my(keys) = NULL;
  $my(desktops) = NULL;
  $my(positional) = NULL;
  $my(OnKeypress) = NULL;

  $my( __Me__) = this;
  WM = this;
  return this;
}

public void __deinit_wm__ (Class (wm) **thisp) {
  if (NULL is *thisp) return;
  Class (wm) *this = *thisp;

  self(release.font);
  self(release.keys);
  self(release.onmap);
  self(release.desktops);
  self(release.positional);

  free ($myprop);
  free (this);
  *thisp = NULL;
}
