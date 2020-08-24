This is a library that implements a tiny fullscreen window manager for the X window environment.

Usage:
```sh
  make

  # This builds and installs, the libwm library, the wm executable and the required
  # header.
  # The default installed hierarchy (lib,include,bin), is the sys directory one level
  # lower to the cloned distribution. To change that, use the SYSDIR variable when
  # invoking make, or modify the Makefile. There is no specific install target.
```

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

To the first original fullscreen window manager and the root of all that has been
spawned this culture, to the legendary ratpoison. It was introduced me to a wide
world view, and served me for 2 years.

To the most advanced of all window managers, the venerable fvwm[2]. Its configuration
schema, allows to suit the personal way to do things. It served me for two years as well.

To the musca and wmii projects, and to all the suckless land. They introduced simplicity
and realism in the new programmer minds.

To the dminiwm project that allowed to offer me (more than the necessary, and within
an amazing compact code), to build the first wm in SLang and use it for 6-7 years, iirc. 

To those that has been participated with any little bit piece to open source ecosystem
and to our code evolution.

The license is the same with dminiwm and it is included with the distribution.
(i wish for a time that the essence of the term, will be the natural way)

Bugs:

The only known is not strictly a bug, but an inconsistency. This happens, when the XCreateFontSet()
fails to create a fontset. This produced a segfault that got fixed by calling another X11
function to do the text drawing in the input_window() function. But the appearance of that window
is now different. Of cource there has to be a way to fix that, through a proper research in
the documents.

The reality is that there is a personal weakness to scan huge documents and with so many technical terms.
And in cases, like complicated systems like X, the real difficulty is to understand
the function relationship and the proper calling sequence, in other words
to undertand the logic. A failure to do this, it means that you might try
things sometimes blindly, praying that you got them right. Otherwise it is quite possible
to give up and to loose the confidence, which is a terrible thing to happen in a human being,
which desperaingly depends on the self confidence.

I don't claim that i understand all the details of the X window system and part of this code.
As my main intention was to work mostly with the structures and callbacks events,
to make it work in C exactly the way i wanted. And it works.

And that is why i really appreciate the dminiwm project.

And for those reasons the documentation is so important, as it is really an art.
For instance in SLang, John Davis doesn't explain less than it has to, and not more than it
should be. In eight years working with it, everything it was needed to know, it was there where
it was expected to be.

I believe that examples is probably the best way to master a system, and in this case
the main() function and the callbacks of the test utility, might be enough.
