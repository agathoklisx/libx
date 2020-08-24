THIS_LIBNAME := wm

API := 0
REV := 1

THIS_LIBVERS  :=  $(API)$(REV)

THIS_HEADER     = lib$(THIS_LIBNAME).h
THIS_LIBSRC     = lib$(THIS_LIBNAME).c
LIBNAME         = $(THIS_LIBNAME)-$(THIS_LIBVERS)
THIS_LIB        = lib$(LIBNAME)

CC            := gcc
CC_STD        := -std=c11

SYSKERNEL     := $(shell uname -s)
SYSARCH       := $(shell uname -m)
SYS           := $(SYSKERNEL)-$(SYSARCH)

THISDIR    := $(shell (cd . && pwd))
SYSDIR     := $(THISDIR)/../../../sys
SYSLIBDIR   = $(SYSDIR)/lib
SYSBINDIR   = $(SYSDIR)/bin
SYSINCDIR   = $(SYSDIR)/include

THIS_SHAREDLIB := $(SYSLIBDIR)/$(THIS_LIB).so
#----------------------------------------------------------#
BASE_FLAGS  := -O2 -march=native -fvisibility=hidden -fdollars-in-identifiers

DEBUG_FLAGS := -g -Wextra -Wshadow -Wall -Wunused-function -Wunused-macros -Wno-override-init

# -Wno-override-init: is for designated initializers and both compilers complain.
# it is included in extra in gcc, but clang needs this unconditionally
ifneq ($(DEBUG), 1)
  ifeq (-$(CC), -clang)
    BASE_FLAGS  +=  -Wno-override-init
  endif
endif

FLAGS         := $(BASE_FLAGS)

CHECK_PTR_BOUNDS_FLAGS := -mmpx -fcheck-pointer-bounds

CHECK_PTR_BOUNDS := 0

ifneq ($(CHECK_PTR_BOUNDS), 0)
  FLAGS += $(CHECK_PTR_BOUNDS_FLAGS)
endif

ifeq (-$(CC), -gcc)
  FLAGS += -Wimplicit-fallthrough=0
endif

DEBUG := 0

ifneq ($(DEBUG), 0)
  FLAGS += $(DEBUG_FLAGS)
endif
#----------------------------------------------------------#
LIBFLAGS := -I$(SYSINCDIR) $(FLAGS) -lX11 -shared -fPIC

all: app-shared

prereq: Env

shared-lib: prereq header $(THIS_SHAREDLIB)
$(THIS_SHAREDLIB):
	$(CC) -x c $(THIS_LIBSRC) $(LIBFLAGS) $(SHARED_CFLAGS) -o $(THIS_SHAREDLIB)
	@$(LN_S) -vf $(THIS_SHAREDLIB) $(SYSLIBDIR)/lib$(THIS_LIBNAME).so

header: clean-header $(SYSINCDIR)/$(THIS_HEADER)
$(SYSINCDIR)/$(THIS_HEADER):
	@$(CP) -v $(THIS_HEADER) $(SYSINCDIR)

#----------------------------------------------------------#
APPNAME := wm

THIS_APPSRC  := $(APPNAME).c
THIS_APPVERS := $(API)$(REV)

APPSHARED    := $(APPNAME)-$(THIS_APPVERS)_shared
SYSAPPSHARED := $(SYSBINDIR)/$(APPSHARED)

#----------------------------------------------------------#
APPFLAGS := -I$(SYSINCDIR) -L$(SYSLIBDIR) $(FLAGS) -lwm

app-all: app-shared

USER = $(shell whoami)

app-shared: shared-lib
	$(CC) -x c $(THIS_APPSRC) $(APPFLAGS) -DTHISDIR='"$(THISDIR)"' -DUSER='"$(USER)"' -o $(APPSHARED)
	@$(INSTALL) -v $(APPSHARED) $(SYSBINDIR)
	@$(RM) $(APPSHARED)

aga: shared-lib
	$(CC) -x c aga.c $(APPFLAGS) -DTHISDIR='"$(THISDIR)"' -DUSER='"$(USER)"' -o $(APPSHARED)
	@$(INSTALL) -v $(APPSHARED) $(SYSBINDIR)
	@$(RM) $(APPSHARED)

#----------------------------------------------------------#
clean: clean-header clean-shared clean-app-shared

clean-shared:
	@$(TEST) ! -f $(THIS_SHAREDLIB) || $(RM) $(THIS_SHAREDLIB)

clean-app: clean-app-shared

clean-app-shared:
	@$(TEST) ! -f $(SYSAPPSHARED) || $(RM) $(SYSAPPSHARED)

clean-header:
	@$(TEST) ! -f $(SYSINCDIR)/$(THIS_HEADER)  || $(RM) $(SYSINCDIR)/$(THIS_HEADER)

#----------------------------------------------------------#
AR = ar rs
TEST = test
RM = rm -f
RM_R = rm -r -f
RMDIR = rmdir
MKDIR = mkdir
MKDIR_P = $(MKDIR) -p
CP = cp
CP_R = $(CP) -r
LN_S = ln -s
INSTALL = install
MAKE = make
#----------------------------------------------------------#

Env: makeenv checkenv
makeenv:
	@$(TEST) -d $(SYSBINDIR)  || $(MKDIR_P) $(SYSBINDIR)
	@$(TEST) -d $(SYSLIBDIR)  || $(MKDIR_P) $(SYSLIBDIR)
	@$(TEST) -d $(SYSINCDIR)  || $(MKDIR_P) $(SYSINCDIR)

checkenv:
	@$(TEST) -w $(SYSBINDIR)  || exit 1
	@$(TEST) -w $(SYSLIBDIR)  || exit 1
	@$(TEST) -w $(SYSINCDIR)  || exit 1
