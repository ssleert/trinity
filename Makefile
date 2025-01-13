DEBUG      ?= 1
STATIC     ?= 0

CC         ?= gcc-14
CFLAGS     ?= -std=c99 -Wall -Wextra -Wpedantic \
              -Wformat=2 -Wno-unused-parameter -Wshadow \
              -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
              -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
              -Wno-format-nonliteral \
							-DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_OMIT_DEPRECATED -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_USE_ALLOCA -DSQLITE_OMIT_AUTOINIT # -Wno-incompatible-pointer-types-discards-qualifiers 
ifeq ($(CC),gcc)
  CFLAGS   += -Wjump-misses-init -Wlogical-op
endif

CFLAGS_RELEASE = $(CFLAGS) -Ofast -DNDEBUG -DNTRACE -march=native
ifeq ($(CC),gcc-14)
  CFLAGS_RELEASE += -flto
endif
ifeq ($(STATIC),1)
	CFLAGS_RELEASE += -static
endif

CFLAGS_DEBUG   = $(CFLAGS) -O0 -g3 -fstack-protector -ftrapv -fwrapv
CFLAGS_DEBUG  += -fsanitize=address,undefined

SRCDIR     ?= src
OBJDIR     ?= obj

PROG        = trinity

CFILES      = $(shell ls $(SRCDIR)/*.c)
COBJS       = ${CFILES:.c=.o}
COBJS      := $(subst $(SRCDIR), $(OBJDIR), $(COBJS))

ifeq ($(DEBUG),1)
	_CFLAGS := $(CFLAGS_DEBUG)
else
	_CFLAGS := $(CFLAGS_RELEASE)
endif

all: prepare $(PROG)
prepare: $(OBJDIR)

$(PROG): $(COBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(_CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	$(CC) $(_CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(PROG) $(OBJDIR)

.PHONY: all install uninstall clean
