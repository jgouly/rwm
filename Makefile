
SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .
topdir = /home/jey/.rvm/rubies/rbx-master/include
hdrdir = $(topdir)
VPATH = $(srcdir):$(topdir):$(hdrdir)
exec_prefix = $(prefix)
install_prefix = $(DESTDIR)
prefix = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master
libdir = $(exec_prefix)/lib
bindir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/bin
includedir = $(prefix)/include
dvidir = $(docdir)
oldincludedir = $(DESTDIR)/usr/include
archdir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/site/i686-linux-gnu
rubylibdir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/site
sharedstatedir = $(prefix)/com
sbindir = $(exec_prefix)/sbin
sitedir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/site
datarootdir = $(prefix)/share
localedir = $(datarootdir)/locale
localstatedir = $(prefix)/var
sitelibdir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/site
htmldir = $(docdir)
pdfdir = $(docdir)
mandir = $(datarootdir)/man
sysconfdir = $(prefix)/etc
rubyhdrdir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/include
sitearchdir = $(DESTDIR)/home/jey/.rvm/rubies/rbx-master/site/i686-linux-gnu
libexecdir = $(exec_prefix)/libexec
infodir = $(datarootdir)/info
docdir = $(datarootdir)/doc/$(PACKAGE)
psdir = $(docdir)
datadir = $(datarootdir)

CC = clang
LIBRUBY = $(LIBRUBY_SO)
LIBRUBY_A = 
LIBRUBYARG_SHARED = 
LIBRUBYARG_STATIC = 

RUBY_EXTCONF_H = 
cflags   = 
optflags = 
debugflags = 
warnflags = 
CFLAGS   =  -fPIC -ggdb3 -O2 -fPIC 
INCFLAGS = -I. -I. -I/home/jey/.rvm/rubies/rbx-master/include -I.
DEFS     = 
CPPFLAGS =  
CXXFLAGS = $(CFLAGS) 
ldflags  = 
dldflags = 
archflag = 
DLDFLAGS = $(ldflags) $(dldflags) $(archflag)
LDSHARED = clang -shared
AR = ar
EXEEXT = 

RUBY_INSTALL_NAME = rbx
RUBY_SO_NAME = rubinius-1.2.5dev
arch = i686-linux-gnu
sitearch = i686-linux-gnu
ruby_version = 1.8
ruby = /home/jey/.rvm/rubies/rbx-master/bin/rbx
RUBY = $(ruby)
RM = rm -f
MAKEDIRS = mkdir -p
INSTALL = install -c
INSTALL_PROG = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 644
COPY = cp

#### End of system configuration section. ####

preload = 

libpath = . $(libdir)
LIBPATH =  -L. -L$(libdir)
DEFFILE = 

CLEANFILES = mkmf.log
DISTCLEANFILES = 

extout = 
extout_prefix = 
target_prefix = 
LOCAL_LIBS = 
LIBS = $(LIBRUBYARG_STATIC) -lX11   
SRCS = rwm.c
OBJS = rwm.o
TARGET = rwm
DLLIB = $(TARGET).so
EXTSTATIC = 
STATIC_LIB = 

BINDIR        = $(bindir)
RUBYCOMMONDIR = $(sitedir)$(target_prefix)
RUBYLIBDIR    = $(sitelibdir)$(target_prefix)
RUBYARCHDIR   = $(sitearchdir)$(target_prefix)

TARGET_SO     = $(DLLIB)
CLEANLIBS     = $(TARGET).so 
CLEANOBJS     = *.o  *.bak

all:    $(DLLIB)
static: $(STATIC_LIB)
.PHONY: all install static install-so install-rb
.PHONY: clean clean-so clean-rb

clean:
	@-$(RM) $(CLEANLIBS) $(CLEANOBJS) $(CLEANFILES)

distclean: clean
	@-$(RM) Makefile $(RUBY_EXTCONF_H) conftest.* mkmf.log
	@-$(RM) core ruby$(EXEEXT) *~ $(DISTCLEANFILES)

realclean: distclean
install: install-so install-rb

install-so: $(RUBYARCHDIR)
install-so: $(RUBYARCHDIR)/$(DLLIB)
$(RUBYARCHDIR)/$(DLLIB): $(DLLIB)
	$(INSTALL_PROG) $(DLLIB) $(RUBYARCHDIR)
install-rb: pre-install-rb install-rb-default
install-rb-default: pre-install-rb-default
pre-install-rb: Makefile
pre-install-rb-default: Makefile
$(RUBYARCHDIR):
	$(MAKEDIRS) $@

site-install: site-install-so site-install-rb
site-install-so: install-so
site-install-rb: install-rb

.SUFFIXES: .c .m .cc .cxx .cpp .C .o

.cc.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.cxx.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.cpp.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.C.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.c.o:
	$(CC) $(INCFLAGS) $(CPPFLAGS) $(CFLAGS) -c $<

$(DLLIB): $(OBJS) Makefile
	@-$(RM) $@
	$(LDSHARED) -o $@ $(OBJS) $(LIBPATH) $(DLDFLAGS) $(LOCAL_LIBS) $(LIBS)



$(OBJS): ruby.h defines.h
