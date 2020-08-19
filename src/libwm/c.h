#ifndef C_H
#define C_H

#define public __attribute__((visibility ("default")))
#define private __attribute__((visibility ("hidden")))
#define mutable __attribute__((__weak__))
#define __fallthrough__  __attribute__ ((fallthrough))
//#define __unused__  __attribute__ ((unused))

#define bytelen strlen
#define is    ==
#define isnot !=
#define and   &&
#define or    ||
#define ifnot(__expr__) if (0 == (__expr__))
#define loop(num_) for (int $i = 0; $i < (num_); $i++)
#define forever for (;;)
#define OK     0
#define NOTOK -1

typedef void (*AllocErrorHandlerF) (int, size_t, char *, const char *, int);

AllocErrorHandlerF AllocErrorHandler;

#define __REALLOC__ realloc
#define __CALLOC__  calloc

/* reallocarray:
 * $OpenBSD: reallocarray.c,v 1.1 2014/05/08 21:43:49 deraadt Exp $
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 */

#define INTEGEROVERFLOW_ERROR  -1002

#define MUL_NO_OVERFLOW ((size_t) 1 << (sizeof (size_t) * 4))
#define MEM_IS_INT_OVERFLOW(nmemb, ssize)                             \
 (((nmemb) >= MUL_NO_OVERFLOW || (ssize) >= MUL_NO_OVERFLOW) &&       \
  (nmemb) > 0 && SIZE_MAX / (nmemb) < (ssize))

#define Alloc(size) ({                                                \
  void *ptr__ = NULL;                                                 \
  if (MEM_IS_INT_OVERFLOW (1, (size))) {                              \
    errno = INTEGEROVERFLOW_ERROR;                                    \
    AllocErrorHandler (errno, (size),  __FILE__, __func__, __LINE__); \
  } else {                                                            \
    if (NULL == (ptr__ = __CALLOC__ (1, (size))))                     \
      AllocErrorHandler (errno, (size), __FILE__, __func__, __LINE__);\
    }                                                                 \
  ptr__;                                                              \
  })

#define Realloc(ptr, size) ({                                         \
  void *ptr__ = NULL;                                                 \
  if (MEM_IS_INT_OVERFLOW (1, (size))) {                              \
    errno = INTEGEROVERFLOW_ERROR;                                    \
    AllocErrorHandler (errno, (size),  __FILE__, __func__, __LINE__); \
  } else {                                                            \
    if (NULL == (ptr__ = __REALLOC__ ((ptr), (size))))                \
      AllocErrorHandler (errno, (size), __FILE__, __func__, __LINE__);\
    }                                                                 \
  ptr__;                                                              \
  })

#define __Me__  Me
#define __this__ this
#define __thisp__ thisp
#define __prop__ prop
#define __self__ self
#define __root__ root
#define __parent__ parent
#define __current__ current
#define Type(__type__) __type__ ## _t
#define DeclareType(__t__) typedef struct Type(__t__) Type(__t__)
#define AllocType(__t__) Alloc (sizeof (Type(__t__)))
#define NewType(__t__, ...) DeclareType(__t__); struct Type(__t__) {__VA_ARGS__}
#define Prop(__p__) __p__ ## prop_t
#define DeclareProp(__p__) typedef struct Prop(__p__) Prop(__p__)
#define AllocProp(__p__) Alloc (sizeof (Prop(__p__)))
#define NewProp(__p__, ...) DeclareProp(__p__); struct Prop(__p__) {__VA_ARGS__}
#define $myprop __this__->__prop__
#define $my(__p__) __this__->__prop__->__p__
#define $self(__f__) $myprop->__Me__->self.__f__
#define $selfp(__f__) (*__thisp__)->prop->__Me__->self.__f__
#define self(__f__, ...) $self(__f__)(__this__, ##__VA_ARGS__)
#define selfp(__f__, ...) $selfp(__f__)(__thisp__, ##__VA_ARGS__)
#define My(__C__) $my(__C__)->self
//#define $from(__v__) __v__->__prop__
#define $from(__v__, __p__) __v__->__prop__->__p__
#define $mycur(__v__) __this__->__current__->__v__
#define $myparents(__p__) __this__->__prop__->__parent__->__prop__->__p__
#define $myroots(__p__) __this__->__prop__->__root__->__prop__->__p__
#define Self(__name__) __name__ ## self_t
#define SubSelf(__super__, __name__) __super__ ## __name__ ## self_t
#define DeclareSelf(__name__) typedef struct Self (__name__) Self (__name__ )
#define NewSelf(__name__, ...)  DeclareSelf (__name__); \
  struct __name__ ## self_t {__VA_ARGS__}
#define NewSubSelf(__super__, __name__, ...) DeclareSelf (__super__ ## __name__); \
  struct __super__ ## __name__ ## self_t {__VA_ARGS__}
#define SelfInit(__name__, ...) (Self (__name__)) {__VA_ARGS__}
#define SubSelfInit(__super__, __name__, ...) SelfInit (__super__ ## __name__, __VA_ARGS__)
#define Class(__Type__) __Type__ ## _T
#define DeclareClass(__C__) typedef struct Class(__C__) Class(__C__)
#define NewClass(__C__, ...) DeclareClass(__C__); struct Class(__C__) {__VA_ARGS__}
#define ClassInit(__C__, ...) (Class (__C__)) {__VA_ARGS__}
#define AllocClass(__C__) ({                           \
  Class(__C__) *__c__ = Alloc (sizeof (Class(__C__))); \
  __c__->__prop__ = AllocProp(__C__);                  \
  __c__;                                               \
})

#define AllocSelf(__S__) ({                            \
  Self (__S__) *__s__ = Alloc (sizeof (Self(__S__)));  \
  __s__;                                               \
})

#endif /* C_H */
