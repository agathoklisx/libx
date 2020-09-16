#ifndef LIBXWM_H
#define LIBXWM_H 1

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

DeclareClass (xwm);
DeclareProp (xwm);
DeclareType (xwm_key);
DeclareType (client);
DeclareType (desktop);
DeclareType (OnMap);
DeclareType (positional);

typedef void (*OnKeypress_cb) (xwm_T *, int, char *);
typedef void (*OnStartup_cb) (xwm_T *);
typedef int  (*Input_cb) (xwm_T *, char *, int, int, KeySym);

NewSubSelf (xwm, get,
  char **(*desk_class_names) (xwm_T *, int);

  void (*font) (xwm_T *);

  int
    (*desknum) (xwm_T *),
    (*desk_numwin) (xwm_T *, int),
    (*current_desktop) (xwm_T *),
    (*previous_desktop) (xwm_T *);

  unsigned long (*color) (xwm_T *, const char *);
);

NewSubSelf (xwm, set,
  void
    (*key) (xwm_T *, char *, int),
    (*mode) (xwm_T *, int, int),
    (*onmap) (xwm_T *, char *, int, int),
    (*desktops) (xwm_T *, int),
    (*positional) (xwm_T *, char *, int, int, int, int),
    (*on_startup_cb) (xwm_T *, OnStartup_cb),
    (*on_keypress_cb) (xwm_T *, OnKeypress_cb);
);

NewSubSelf (xwm, release,
  void
    (*font) (xwm_T *),
    (*keys) (xwm_T *),
    (*onmap) (xwm_T *),
    (*desktops) (xwm_T *),
    (*positional) (xwm_T *);
);

NewSubSelf (xwm, resize,
  void
    (*stack) (xwm_T *, int),
    (*side_way) (xwm_T *, int);
);

NewSubSelf (xwm, move,
  void
    (*stack) (xwm_T *, int),
    (*side_way) (xwm_T *, int);
);

NewSelf (xwm,
  SubSelf (xwm, get) get;
  SubSelf (xwm, set) set;
  SubSelf (xwm, move) move;
  SubSelf (xwm, resize) resize;
  SubSelf (xwm, release) release;

  int (*xfree) (void *);

  void
    (*init) (xwm_T *, int),
    (*tile) (xwm_T *),
    (*quit) (xwm_T *),
    (*spawn) (xwm_T *, char **),
    (*grabkeys) (xwm_T *),
    (*prev_win) (xwm_T *),
    (*next_win) (xwm_T *),
    (*add_window) (xwm_T *, Window, int, client_t *),
    (*kill_client) (xwm_T *),
    (*change_mode) (xwm_T *, int),
    (*save_desktop) (xwm_T *, int),
    (*input_window) (xwm_T *, char *, Input_cb),
    (*remove_window) (xwm_T *, Window, int, int),
    (*change_desktop) (xwm_T *, int),
    (*update_current) (xwm_T *),
    (*select_desktop) (xwm_T *, int),
    (*kill_client_now) (xwm_T *, Window),
    (*client_to_desktop) (xwm_T *, int),
    (*follow_client_to_desktop) (xwm_T *, int);

  int
    (*startx) (xwm_T *);
);

NewClass (xwm,
  Self (xwm) self;
  Prop (xwm) *prop;

  void *user_obj;
  char *user_string;
  int   user_int;
);

public xwm_T *__init_xwm__ (void);
public void  __deinit_xwm__ (xwm_T **);

#endif /* LIBWM_H */
