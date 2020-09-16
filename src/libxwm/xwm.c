/* Sample utility to initialize the library and which serves as a starting point
   and demonstration, so it is merely an example.

   Some functions are Linux only.

   Most functions assume basic utilities, that are installed by default in
   almost all distributions and do not represent personal preferences.
 */

/* Keybindings as they were implemented in this application.
   Mod4 = windows key
   Mod1 = alt key

   Mod4 + ([0-9] and [F1-F8]) goes to desktops [0-9] and [10-17] respectively
   Mod4 + (c) spawns the terminal 
   Mod4 + (tab and q) cycles through the [previous|next] generated windows in the
                      current desktop
   Mod4 + (`) return to the previous focused desktop
   Mod4 + (left and right) cycles through the [previous|next] desktop in the order
                           they were created
   Mod4 + (l) set the backlight to the maximum brightness (Linux only)

   Mod4 + shift + (q) quits the window manager
   Mod4 + shift + (k) kills the current client

   Mod1 + (F3) alsamixer
   Mod1 + (F4) htop process viewer

   Mod4 + Mod1 + (left, right, up and down) resizes window in stacking mode
   Mod4 + Mod1 + (m) puts the system in sleep (Linux only)
   Mod4 + Mod1 + (l) set the backlight to the minimum brightness (Linux only)

   Mod4 + control + (left, right, up and down) moves window in stacking mode
 */

/* Demonstration:
  Mod4 + 1 will focus to the first desktop and open this unit with vim if itsn't running
  Mod4 + 2 will creates two windows in this directory
  Mod4 + 5 will focus to the fifth desktop and open the lua interactive interpreter if itsn't running
  Mod4 + 6 will focus to the sixth desktop and open the mutt email client if itsn't running
  Mod4 + 7 will open two windows in the /tmp directory
  Mod4 + 8 will focus to the eighth desktop and open the elinks text browser if itsn't running
  ...
  With every change in a desktop, a terminal will be created if the desktop is empty of windows

  All the desktop modes are set to fullscreen mode, except one and Mod4 + F4 will
  give the focus to that desktop.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <string.h>

#include <X11/Xlib.h>
#include <errno.h>

#include "c.h"
#include <libxwm.h>

#define DESKNUM       18

/* urxvt[c] because it understands the -cd (change directory) option and simplify
   the examples. "urxvtd -q -f -o &" in the top of .xinitrc if using urxvtc client */
#define TERM          "urxvt"
#define SHELL         "bash"
#define EDITOR        "vim"
#define TXT_BROWSER   "elinks"
#define MAILER        "mutt"
#define MAILER_ARGS   "-y"
#define PLANG_SHELL   "lua"
#define TMP_DIR       "/tmp"
#define HOME_DIR      "/home/" USER
#define THIS_FILE     THISDIR "/xwm.c"

#define WIN_KEY         (Mod4Mask)
#define ALT_KEY         (Mod1Mask)
#define WIN_SHIFT_KEY   (Mod4Mask|ShiftMask)
#define WIN_ALT_KEY     (Mod4Mask|Mod1Mask)
#define WIN_CONTROL_KEY (Mod4Mask|ControlMask)

// the compiler flags include "-Wunused-macros" and those are not being used
// #define ALT_CONTROL_KEY (Mod1Mask|ControlMask)
// #define CONTROL_KEY     (ControlMask)

#define STR_EQ       0 is strcmp
#define FOLLOW       1

static xwm_T *XWM;

#define Xwm XWM->self

NewType (xwm,
  char *backlight_dir;
  size_t backlight_dirlen;
  int max_brightness;

  char *sysname;
);

mutable public void __alloc_error_handler__ (int err, size_t size,
                           char *file, const char *func, int line) {
  fprintf (stderr, "MEMORY_ALLOCATION_ERROR\n");
  fprintf (stderr, "File: %s\nFunction: %s\nLine: %d\n", file, func, line);
  fprintf (stderr, "Size: %zd\n", size);

  if (err is INTEGEROVERFLOW_ERROR)
    fprintf (stderr, "Error: Integer Overflow Error\n");
  else
    fprintf (stderr, "Error: Not Enouch Memory\n");

  exit (1);
}

private int get_passwd_cb (xwm_T *this, char *buffer, int num_chars,
                                         int keycode, KeySym keysym) {
  (void) keycode; (void) keysym;

  if (NULL is this->user_string)
    this->user_string =  (char *) Alloc (512);

  int idx = this->user_int;
  if (-1 is idx)
    idx = 0;

  if (num_chars is 1) {
    if (buffer[0] is '\r')
      return 0;

     this->user_string[idx++] = buffer[0];
  } else {
    strncpy (this->user_string + idx, buffer, num_chars);
    idx += num_chars;
   }

  this->user_string[idx] = '\0';
  this->user_int = idx;
  return 1;
}

private void sys_backlight_linux (xwm_T *this, xwm_t *me, int v) {
  int value = (me->max_brightness * v) / 100;

  ifnot (NULL is this->user_string)
    free (this->user_string);

  this->user_string = NULL;
  this->user_int = -1;

  Xwm.input_window (this, "Passwd:", get_passwd_cb);
  size_t len = bytelen (this->user_string);
  ifnot (len) return;

  size_t comlen = len + me->backlight_dirlen + 1 + bytelen ("brightness") + 256;

  char com[comlen + 1];
  snprintf (com, comlen + 1,
      "printf \"%s\n\" | "
      "sudo -S -p \"\" "
      "/bin/sh -c \"/bin/printf \"%d\" >%s/brightness\"",
      this->user_string, value, me->backlight_dir);
  system (com);
}

private void sys_backlight (xwm_T *this, int v) {
  xwm_t *me = (xwm_t *) this->user_obj;
  if (NULL is me->backlight_dir or
      0 is me->backlight_dirlen or
      0 is me->max_brightness)
    return;

  if (STR_EQ ("Linux", me->sysname))
    sys_backlight_linux (this, me, v);
}

private void sys_to_memory_linux (xwm_T *this) {
  ifnot (NULL is this->user_string)
    free (this->user_string);

  this->user_string = NULL;
  this->user_int = -1;

  Xwm.input_window (this, "Passwd:", get_passwd_cb);
  size_t len = bytelen (this->user_string);
  ifnot (len) return;

  size_t comlen = len + bytelen ("/sys/power/state") + 256;

  char com[comlen + 1];
  snprintf (com, comlen + 1,
      "printf \"%s\n\" | "
      "sudo -S -p \"\" "
      "/bin/sh -c \"/bin/printf \"mem\" >/sys/power/state\"",
      this->user_string);
  system (com);
}

private void sys_to_memory (xwm_T *this) {
  xwm_t *me = (xwm_t *) this->user_obj;
  if (STR_EQ ("Linux", me->sysname))
    sys_to_memory_linux (this);
}

private void on_change_desktop (xwm_T *this, int desk_idx) {
  char **classnames = Xwm.get.desk_class_names (this, desk_idx);

  int idx = 0;

  if (1 is desk_idx) {
    if (NULL isnot classnames) {
      while (classnames[idx])
        if (STR_EQ (classnames[idx++], "EDITOR"))
          goto theend;
    }

    char *argv[] = {TERM, "-name", "EDITOR", "-e", EDITOR, THIS_FILE, NULL};
    Xwm.spawn (this, argv);
    goto theend;
  }

  if (desk_idx is 2) {
    int num = 0;
    if (NULL isnot classnames) {
      while (classnames[idx])
        if (STR_EQ (classnames[idx++], "XWM_DIR")) {
          num++;
          if (num is 2) goto theend;
        }
    }

    char *argv[] = {TERM, "-name", "XWM_DIR", "-cd", THISDIR, "-e", SHELL, NULL};
    Xwm.spawn (this, argv);
    if (++num < 2)
      Xwm.spawn (this, argv);

    goto theend;
  }

  if (desk_idx is 5) {
    if (NULL isnot classnames) {
      while (classnames[idx])
        if (STR_EQ (classnames[idx++], "PLANG_SHELL")) goto theend;
    }

    char *argv[] = {TERM, "-name", "PLANG_SHELL", "-e", PLANG_SHELL, NULL};
    Xwm.spawn (this, argv);
    goto theend;
  }

  if (desk_idx is 6) {
    if (NULL isnot classnames) {
      while (classnames[idx])
        if (STR_EQ (classnames[idx++], "MAIL")) goto theend;
    }

    char *argv[] = {TERM, "-name", "MAIL", "-e", MAILER, MAILER_ARGS, NULL};
    Xwm.spawn (this, argv);
    goto theend;
  }

  if (desk_idx is 7) {
    int num = 0;
    if (NULL isnot classnames) {
      while (classnames[idx])
        if (STR_EQ (classnames[idx++], "TMP")) {
          num++;
          if (num is 2) goto theend;
        }
    }

    char *argv[] = {TERM, "-name", "TMP", "-cd", TMP_DIR, "-e", SHELL, NULL};
    Xwm.spawn (this, argv);
    if (++num < 2)
      Xwm.spawn (this, argv);

    goto theend;
  }

  if (desk_idx is 8) {
    if (NULL isnot classnames) {
      while (classnames[idx])
        if (STR_EQ (classnames[idx++], "TXT_BROWSER")) goto theend;
    }

    char *argv[] = {TERM, "-name", "TXT_BROWSER", "-e", TXT_BROWSER,
       "www.linuxfromscratch.org", NULL};
    Xwm.spawn (this, argv);
    goto theend;
  }

  if (NULL is classnames) {
    char *argv[] = {TERM, "-cd", HOME_DIR, "-e", SHELL, NULL};
    Xwm.spawn (this, argv);
  }

theend:
  if (NULL is classnames) return;
  idx = 0;
  while (classnames[idx]) free (classnames[idx++]);
  free (classnames);
}

private void on_keypress (xwm_T *this, int modifier, char *key) {
  int
    desk_idx,
    cur,
    num;

  if (modifier is WIN_KEY) {
    if (STR_EQ ("c", key)) {
      char *argv[] = {TERM, "-e", SHELL, NULL};
      Xwm.spawn (this, argv);
    } else if (STR_EQ ("Tab", key))
      Xwm.next_win (this);
    else if (STR_EQ ("q", key))
      Xwm.prev_win (this);
    else if (STR_EQ ("grave", key)) {
      int prev = Xwm.get.previous_desktop (this);
      Xwm.change_desktop (this, prev);
    } else if (STR_EQ ("Left", key)) {
      cur = Xwm.get.current_desktop (this);
      num = Xwm.get.desknum (this);
      desk_idx = (cur + num - 1) % num;
      Xwm.change_desktop (this, desk_idx);
    } else if (STR_EQ ("Right", key)) {
      cur = Xwm.get.current_desktop (this);
      num = Xwm.get.desknum (this);
      desk_idx = (cur + num + 1) % num;
      Xwm.change_desktop (this, desk_idx);
    } else if ('0' <= key[0] and key[0] <= '9') {
      desk_idx = key[0] - '0';
      Xwm.change_desktop (this, desk_idx);
      on_change_desktop (this, desk_idx);
    } else if ('F' is key[0] and ('1' <= key[1] and key[1] <= '8')) {
      desk_idx = key[1] - '0' + 9;
      Xwm.change_desktop (this, desk_idx);
      on_change_desktop (this, desk_idx);
    } else if (STR_EQ ("Return", key)) {
      char *argv[] = {"xterm", "-e", SHELL, NULL};
      Xwm.spawn (this, argv);
    } else if (STR_EQ ("l", key))
      sys_backlight (this, 100);

    return;
  }

  if (modifier is WIN_SHIFT_KEY) {
    if (STR_EQ ("q", key))
      Xwm.quit (this);
    else if (STR_EQ ("k", key))
      Xwm.kill_client (this);

    return;
  }

  if (modifier is ALT_KEY) {
    if (STR_EQ ("F4", key)) {
      char *argv[] = {TERM, "-name", "HTOP", "-e", "htop", NULL};
      Xwm.spawn (this, argv);
    } else if (STR_EQ ("F3", key)) {
      char *argv[] = {TERM, "-name", "ALSAMIXER", "-e", "alsamixer", NULL};
      Xwm.spawn (this, argv);
    } else if (STR_EQ ("F2", key)) {
      char *argv[] = {"chromium", NULL};
      Xwm.spawn (this, argv);
    }

    return;
  }

  if (modifier is WIN_ALT_KEY) {
    if (STR_EQ ("Right", key))
      Xwm.resize.side_way (this, 12);
    else if (STR_EQ ("Left", key))
      Xwm.resize.side_way (this, -12);
    else if (STR_EQ ("Up", key))
      Xwm.resize.stack (this, -12);
    else if (STR_EQ ("Down", key))
      Xwm.resize.stack (this, 12);
    else if (STR_EQ ("m", key))
      sys_to_memory (this);
    else if (STR_EQ ("l", key))
      sys_backlight (this, 1);

    return;
  }

  if (modifier is WIN_CONTROL_KEY) {
    if (STR_EQ ("Right", key))
      Xwm.move.side_way (this, 15);
    else if (STR_EQ ("Left", key))
      Xwm.move.side_way (this, -15);
    else if (STR_EQ ("Up", key))
      Xwm.move.stack (this, -15);
    else if (STR_EQ ("Down", key))
      Xwm.move.stack (this, 15);
    return;
  }
}

private void on_startup (xwm_T *this) {
  char *argv[] = {TERM, "-e", SHELL, NULL};
  Xwm.spawn (this, argv);
}

private int is_directory (char *dname) {
  struct stat st;
  if (NOTOK is stat (dname, &st)) return 0;
  return S_ISDIR (st.st_mode);
}

private void init_backlight_linux (xwm_t *this) {
  char dir[] = "/sys/class/backlight";

  ifnot (is_directory (dir)) return;

  DIR *dh = NULL;
  if (NULL is (dh = opendir (dir))) return;
  struct dirent *dp;

  size_t dirlen = bytelen (dir);
  size_t len = 0;

  while (1) {
    if (NULL is (dp = readdir (dh)))
      break;

    len = bytelen (dp->d_name);

    if (len < 3 and dp->d_name[0] is '.')
      if (len is 1 or dp->d_name[1] is '.')
        continue;

    if (dp->d_type is DT_DIR)
      continue;

    this->backlight_dirlen = len + dirlen + 1;
    this->backlight_dir = Alloc (this->backlight_dirlen + 1);
    snprintf (this->backlight_dir, this->backlight_dirlen + 1, "%s/%s", dir, dp->d_name);
    break;
  }

  closedir (dh);

  if (NULL is this->backlight_dir) return;
  len = this->backlight_dirlen + 1 + bytelen ("max_brightness");
  char mb[len + 1];
  snprintf (mb, len + 1, "%s/max_brightness", this->backlight_dir);
  FILE *fp = fopen (mb, "r");
  if (NULL is fp) return;
  char buf[32];
  char *s = fgets (buf, 32, fp);
  fclose (fp);
  if (NULL is s) return;
  len = bytelen (s);
  s[len - 1] = '\0';
  this->max_brightness = atoi (s);
}

private void init_backlight (xwm_t *this) {
  this->backlight_dirlen = 0;
  this->backlight_dir = NULL;
  this->max_brightness = 0;

  if (NULL isnot this->sysname)
    if (STR_EQ ("Linux", this->sysname))
      init_backlight_linux (this);
}

private xwm_t *init_this (void) {
  xwm_t *this = AllocType (xwm);

  struct utsname u;
  if (-1 is uname (&u))
    this->sysname = NULL;
  else
    this->sysname = strdup (u.sysname);

  init_backlight (this);

  return this;
}

private void deinit_this (xwm_t **thisp) {
  if (NULL is *thisp) return;
  xwm_t *this = *thisp;

  ifnot (NULL is this->sysname)
    free (this->sysname);

  ifnot (NULL is this->backlight_dir)
    free (this->backlight_dir);

  free (*thisp);
  *thisp = NULL;
}

int main (int argc, char **argv) {
  (void) argc; (void) argv;

  xwm_t *xwm = init_this ();

  xwm_T *this = __init_xwm__();
  XWM = this;
  this->user_obj = xwm;

  Xwm.init (this, DESKNUM);

  for (int i = 0; i < 10; i++) {
    char n[2]; n[0] = i + '0'; n[1] = '\0';
    Xwm.set.key (this, n, Mod4Mask);
  }

  for (int i = 1; i < 9; i++) {
    char n[3]; n[0] = 'F'; n[1] = i + '0'; n[2] = '\0';
    Xwm.set.key (this, n, Mod4Mask);
  }

  Xwm.set.key (this, "c",      Mod4Mask);
  Xwm.set.key (this, "q",      Mod4Mask);
  Xwm.set.key (this, "Tab",    Mod4Mask);
  Xwm.set.key (this, "Left",   Mod4Mask);
  Xwm.set.key (this, "Right",  Mod4Mask);
  Xwm.set.key (this, "grave",  Mod4Mask);
  Xwm.set.key (this, "Return", Mod4Mask);
  Xwm.set.key (this, "l",      Mod4Mask);

  Xwm.set.key (this, "q",      Mod4Mask|ShiftMask);
  Xwm.set.key (this, "k",      Mod4Mask|ShiftMask);

  Xwm.set.key (this, "F3",     Mod1Mask);
  Xwm.set.key (this, "F4",     Mod1Mask);
  Xwm.set.key (this, "F2",     Mod1Mask);

  Xwm.set.key (this, "Right",  Mod4Mask|Mod1Mask);
  Xwm.set.key (this, "Left",   Mod4Mask|Mod1Mask);
  Xwm.set.key (this, "Up",     Mod4Mask|Mod1Mask);
  Xwm.set.key (this, "Down",   Mod4Mask|Mod1Mask);
  Xwm.set.key (this, "m",      Mod4Mask|Mod1Mask);
  Xwm.set.key (this, "l",      Mod4Mask|Mod1Mask);

  Xwm.set.key (this, "Right",  Mod4Mask|ControlMask);
  Xwm.set.key (this, "Left",   Mod4Mask|ControlMask);
  Xwm.set.key (this, "Up",     Mod4Mask|ControlMask);
  Xwm.set.key (this, "Down",   Mod4Mask|ControlMask);

  Xwm.set.onmap (this, "HTOP",      13, FOLLOW);
  Xwm.set.onmap (this, "ALSAMIXER", 13, FOLLOW);
  Xwm.set.onmap (this, "Chromium",  12, FOLLOW);

//  Xwm.set.positional (this, "InputWindow", 1, 1, 1000, 50);

  Xwm.set.mode (this, 14, STACK_MODE);

  Xwm.set.on_startup_cb (this, on_startup);
  Xwm.set.on_keypress_cb (this, on_keypress);

  Xwm.startx (this);

  __deinit_xwm__ (&this);

  deinit_this (&xwm);

  return 0;
}
