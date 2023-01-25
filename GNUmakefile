MAKE=make
SHELL=/bin/sh
OBJDIR=obj
DRIVER_BIN=driver
PROOF=
STRFUNCS=
INSTALL=install -c
INSTALL_DIR=../v21.7_bin
OPTIMIZE=-O2 -fomit-frame-pointer -fstrength-reduce
CPP=gcc -E
CFLAGS=
CC=gcc
YACC=bison -d -y
RANLIB=ranlib

# YOU DO NOT NEED TO CONFIGURE ANYTHING IN THIS FILE.
#
# RUN THE SHELL SCRIPT ./build.MudOS to generate the Makefiles, and follow
# it's instructions.
#
############################################################################
#
# **** TARGETS AND THEIR CORRECT USAGE ****
#
# COMPILATION TARGETS:
# 
# all:		compile all the files
#
# install:	make all, then move the files to the correct directories
#
# remake: 	remove the object files and generated files, and recompile
# 		     NO reconfiguration is done, etc.
#
# depend:	automatically create dependency info
#
# Makefiles:	update Makefile.in and GNUmakefile.in from Makefile.master
#		(only necessary after making changes to Makefile.master)
#
# 'CLEAN' TARGETS:
#
# neat: 	remove object files and generated files (used by remake)
#
# clean:	in addition to neat, also remove .orig and .rej files,
#		cores, lint files, emacs backups, tag files, yacc debug
#		files, generated Makefiles, generated binaries, and
#		generated dependency info
#
# spotless:	make clean, then remove ALL CONFIGURATION AND CUSTOMIZATION
#		     useful for creating distributions
#
# ---- REALLY COMPLEX OPTIONS YOU PROBABLY DON'T WANT TO TOUCH -----
#
# NeXT: link with MallocDebug if you have a NeXT with NeXTOS 2.1 or later and
# you wish to search for memory leaks (see /NextDeveloper/Apps/MallocDebug).
# Note: linking with MallocDebug will cause the virtual size of the
# driver process to reach appoximately 40MB however the amount of real memory
# used will remain close to normal.
#EXTRALIBS=-lMallocDebug -lsys_s
#
# ---- DO NOT EDIT ANYTHING BELOW HERE UNLESS YOU KNOW ALOT ABOUT HOW
#      MUDOS WORKS INTERNALLY ----

OURLIBS=packages/packages.a mudlib/mudlib.a

OVERRIDES=$(MAKEOVERRIDES)

SRC=grammar.tab.c lex.c main.c rc.c interpret.c simulate.c file.c object.c \
  backend.c array.c mapping.c comm.c ed.c regexp.c swap.c buffer.c crc32.c \
  malloc.c mallocwrapper.c class.c efuns_main.c efuns_port.c \
  call_out.c otable.c dumpstat.c stralloc.c hash.c \
  port.c reclaim.c parse.c simul_efun.c sprintf.c program.c \
  compiler.c avltree.c icode.c trees.c generate.c scratchpad.c \
  socket_efuns.c socket_ctrl.c qsort.c eoperators.c socket_err.c md.c \
  strstr.c disassembler.c binaries.c ualarm.c $(STRFUNCS) \
  replace_program.c functab_tree.c ccode.c cfuns.c compile_file.c

all: cc.h files main_build

main_build: $(DRIVER_BIN) addr_server

VPATH = $(OBJDIR)

OBJ=$(addprefix $(OBJDIR)/,$(subst .c,.o,$(SRC)))

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(OPTIMIZE) -o $@ -c $<

$(OBJDIR)/lex.o: lex.c preprocess.c cc.h grammar.tab.c

$(OBJDIR)/grammar.tab.o: grammar.tab.c opcodes.h

which_makefile:
	echo MakeIsGNU

grammar.tab.c: grammar.y
	$(YACC) grammar.y
	-rm -f grammar.tab.*
	sed "s/y.tab.c/grammar.tab.c/g" y.tab.c  > grammar.tab.c
	-mv y.tab.h grammar.tab.h

$(DRIVER_BIN): $(OBJ)
	$(MAKE) -C packages 'CC=$(CC)' 'CFLAGS=$(CFLAGS) $(OPTIMIZE)' 'OBJDIR=../$(OBJDIR)' 'RANLIB=$(RANLIB)'
	$(MAKE) -C mudlib 'CC=$(CC)' 'CFLAGS=$(CFLAGS) $(OPTIMIZE)' 'OBJDIR=../$(OBJDIR)' 'RANLIB=$(RANLIB)'
	-mv -f $(DRIVER_BIN) $(DRIVER_BIN).old
	$(PROOF) $(CC) $(CFLAGS) $(OPTIMIZE) $(OBJ) -o $(DRIVER_BIN) $(OURLIBS) `cat system_libs`

addr_server:  $(OBJDIR)/addr_server.o $(OBJDIR)/socket_ctrl.o $(OBJDIR)/port.o addr_server.h
	$(CC) $(CFLAGS) $(OPTIMIZE) $(OBJDIR)/socket_ctrl.o $(OBJDIR)/addr_server.o $(OBJDIR)/port.o \
	-o addr_server `cat system_libs`


remake: neat all

customize:
	-cp ../local_options .
	-cp ../system_libs .
	-cp ../configure.h .

depend: opcodes.h grammar.tab.c cc.h efunctions.h efun_defs.c configure.h
	-rm tmpdepend
	for i in *.c; do $(CC) -MM -DDEPEND $$i >>tmpdepend; done
	sed -e "s!^[^ ]!$(OBJDIR)/&!" <tmpdepend >Dependencies
	-rm tmpdepend

cc.h: GNUmakefile
	rm -f cc.h
	echo "/* this file automatically generated by the Makefile */" > cc.h
	echo '#define COMPILER "$(CC)"' >> cc.h
	echo '#define OPTIMIZE "$(OPTIMIZE)"' >> cc.h
	echo '#define CFLAGS   "$(CFLAGS) $(OPTIMIZE)"' >> cc.h
	echo '#define OBJDIR   "$(OBJDIR)"' >> cc.h

# the touches here are necessary to fix the modification times; link(2) does
# 'modify' a file
files: edit_source sysmalloc.c smalloc.c bsdmalloc.c debugmalloc.c wrappedmalloc.c options.h op_spec.c func_spec.c mudlib/Makefile.pre mudlib/GNUmakefile.pre packages/Makefile.pre packages/GNUmakefile.pre configure.h grammar.y.pre
	./edit_source -options -malloc -build_func_spec '$(CPP) $(CFLAGS)' \
	              -process grammar.y.pre
	./edit_source -process packages/Makefile.pre
	./edit_source -process packages/GNUmakefile.pre
	./edit_source -process mudlib/Makefile.pre
	./edit_source -process mudlib/GNUmakefile.pre
	./edit_source -build_efuns
	touch mallocwrapper.c
	touch malloc.c
	touch files

make_func.tab.c: make_func.y cc.h
	$(YACC) $(YFLAGS) make_func.y
	-rm -f make_func.tab.c
	mv y.tab.c make_func.tab.c

configure.h: edit_source
	./edit_source -configure

$(OBJDIR)/edit_source.o: edit_source.c preprocess.c cc.h

edit_source: $(OBJDIR)/edit_source.o $(OBJDIR)/hash.o $(OBJDIR)/make_func.tab.o
	$(CC) $(CFLAGS) $(OBJDIR)/edit_source.o $(OBJDIR)/hash.o $(OBJDIR)/make_func.tab.o -o edit_source

# don't optimize these two
$(OBJDIR)/edit_source.o: edit_source.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/make_func.tab.o: make_func.tab.c
	$(CC) $(CFLAGS) -o $@ -c $<

tags: $(SRC)
	ctags $(SRC)

TAGS: $(SRC)
	etags $(SRC)

install: all
	-mkdir $(INSTALL_DIR)
	$(INSTALL) $(DRIVER_BIN) $(INSTALL_DIR)
	$(INSTALL) addr_server $(INSTALL_DIR)

Makefiles: Makefile.in GNUmakefile.in

Makefile.in: edit_source Makefile.in.pre Makefile.master
	./edit_source -process Makefile.in.pre

GNUmakefile.in: edit_source GNUmakefile.in.pre Makefile.master
	./edit_source -process GNUmakefile.in.pre

nothing:

# remove local configuration
spotless: clean
	-rm -f configure.h local_options system_libs
	-rm -f options_incl.h

# get ready for recompile
neat:
	-(cd packages; $(MAKE) clean)
	-(cd mudlib; $(MAKE) clean)
	-rm -rf $(OBJDIR) *.o *.tab.c *.tab.h
	-mkdir $(OBJDIR)
	-rm -f efun_defs.c option_defs.c
	-rm -f opcodes.h efunctions.h opc.h efun_protos.h
	-rm -f malloc.c mallocwrapper.c
	-rm -f func_spec.cpp files
	-rm -f grammar.y comptest* a.out
	-rm -f packages/Makefile packages/GNUmakefile

# remove everything except configuration
clean: neat
	-rm -f Makefile.MudOS GNUmakefile.MudOS
	-rm -f cc.h edit_source
	-rm -f core y.output
	-rm -f *.orig *.rej
	-rm -f */*.orig */*.rej
	-rm -f *.ln tags */*~ *~ TAGS
	-rm -f $(DRIVER_BIN) $(DRIVER_BIN).old addr_server
	-rm -f Dependencies tmpdepend
	-touch Dependencies

include Dependencies
