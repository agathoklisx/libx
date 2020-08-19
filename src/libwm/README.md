This is a library that implements a tiny fullscreen window manager for X windows.

Usage:
```sh
  make

  # This builds and installs, the libwm library, the wm executable and the required
  # header.
  # The default installed hierarchy (lib,include,bin), is the sys directory one level
  # lower to the cloned distribution. To change that, use the SYSDIR variable when
  # invoking make, or modify the Makefile. There is no specific install target.
```
```C
This code has been tested and can be compiled with gcc, clang and the tcc C compilers,
without a warning and while turning the DEBUG flags on.

Testing with valgrind for memory leaks, and though there are many reports that are
coming from the X library, at the end reports that all the allocated resources has
been deallocated properly.

This library exposes a wm_T * structure, which holds all the required information,
and it can be initialized with:

  wm_T *this = __init_wm__ ();

It implements a fullscreen and a stacking mode. The original source had a couple
modes more, but since they have never being used, that code has been removed with
time, so see at the original source to expand the functionality:

  https://github.com/moetunes/dminiwm

In fullscreen mode the windows reserve all the required screen pixels and can not
be overlap with each other, they can not be splitted and they can not be resized.

The stacking mode has almost the same behavior, except that can be resized and can
be moved, plus they draw a very thin line with a different color to indicate the
focus.

All the operations are controlled with the keyboard. There is no code to handle
mouse events, but the original code includes this functionality.

The wm.c unit, it serves as an example using the library from C.

It reserves 18 desktops/workspaces.

Desktops they do not have relation with each other.

In one such desktop (which can be unlimited), they can be spawned unlimited clients.

One desktop can be set in one of those two modes. The default is fullscreen mode.
Note that the mode of a desktop can not influence the others.

The user of the library should be set the keys to control the operations, because
the library doesn't sets anything by its own.

It can also associate a client with a desktop, by using the class name.

But the function names and the methods of the structure, describe the usage. Also
the code at the wm.c unit is self explainable. Start from the main function and
walk to the top.

Thanks:

To the first original fullscreen manager and the root of all that has been spawned
this culture, to the legendary ratpoison.

To the most advanced of all window managers, the venerable fvwm[2].

To the musca and wmii projects, and to all the suckless land.

To the dminiwm project.

To those that has been offered code and any little bit of help in the evolution.

The license is the same with dminiwm and it is included with the distribution.
```
