#ifndef LIBWM_H
#define LIBWM_H 1

#define FULLSCREEN_MODE 0
#define STACK_MODE      1
#define DEFAULT_MODE FULLSCREEN_MODE

#define BORDER_WIDTH    1

#define FOCUS   "#664422" /* dkorange */
#define UNFOCUS "#004050" /* blueish */

#define colorblack  "#000000"
#define colorwhite  "#ffffff"
#define colorcyan   "#001020"
#define color2      "#002030"
#define color3      "#665522"
#define color4      "#898900"
#define color5      "#776644"
#define color6      "#887733"
#define color7      "#998866"
#define color8      "#999999"
#define color9      "#000055"  // &9

DeclareClass (wm);
DeclareProp (wm);
DeclareType (wm_key);
DeclareType (client);
DeclareType (desktop);
DeclareType (OnMap);
DeclareType (positional);

typedef void (*OnKeypress_cb) (wm_T *, int, char *);
typedef void (*OnStartup_cb) (wm_T *);
typedef int  (*Input_cb) (wm_T *, char *, int, int, KeySym);

NewSubSelf (wm, get,
  char **(*desk_class_names) (wm_T *, int);

  void (*font) (wm_T *);

  int
    (*desknum) (wm_T *),
    (*desk_numwin) (wm_T *, int),
    (*current_desktop) (wm_T *),
    (*previous_desktop) (wm_T *);

  unsigned long (*color) (wm_T *, const char *);
);

NewSubSelf (wm, set,
  void
    (*key) (wm_T *, char *, int),
    (*mode) (wm_T *, int, int),
    (*onmap) (wm_T *, char *, int, int),
    (*desktops) (wm_T *, int),
    (*positional) (wm_T *, char *, int, int, int, int),
    (*on_startup_cb) (wm_T *, OnStartup_cb),
    (*on_keypress_cb) (wm_T *, OnKeypress_cb);
);

NewSubSelf (wm, release,
  void
    (*font) (wm_T *),
    (*keys) (wm_T *),
    (*onmap) (wm_T *),
    (*desktops) (wm_T *),
    (*positional) (wm_T *);
);

NewSubSelf (wm, resize,
  void
    (*stack) (wm_T *, int),
    (*side_way) (wm_T *, int);
);

NewSubSelf (wm, move,
  void
    (*stack) (wm_T *, int),
    (*side_way) (wm_T *, int);
);

NewSelf (wm,
  SubSelf (wm, get) get;
  SubSelf (wm, set) set;
  SubSelf (wm, move) move;
  SubSelf (wm, resize) resize;
  SubSelf (wm, release) release;

  int (*xfree) (void *);

  void
    (*init) (wm_T *, int),
    (*tile) (wm_T *),
    (*quit) (wm_T *),
    (*spawn) (wm_T *, char **),
    (*grabkeys) (wm_T *),
    (*prev_win) (wm_T *),
    (*next_win) (wm_T *),
    (*add_window) (wm_T *, Window, int, client_t *),
    (*kill_client) (wm_T *),
    (*change_mode) (wm_T *, int),
    (*save_desktop) (wm_T *, int),
    (*input_window) (wm_T *, char *, Input_cb),
    (*remove_window) (wm_T *, Window, int, int),
    (*change_desktop) (wm_T *, int),
    (*update_current) (wm_T *),
    (*select_desktop) (wm_T *, int),
    (*kill_client_now) (wm_T *, Window),
    (*client_to_desktop) (wm_T *, int),
    (*follow_client_to_desktop) (wm_T *, int);

  int
    (*startx) (wm_T *);
);

NewClass (wm,
  Self (wm) self;
  Prop (wm) *prop;

  void *user_obj;
  char *user_string;
  int   user_int;
);

public wm_T *__init_wm__ (void);
public void  __deinit_wm__ (wm_T **);

#endif /* LIBWM_H */
