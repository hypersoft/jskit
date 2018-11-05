ifdef HYPER_MAKE

# Making any changes to this file, will rebuild all files this file builds.
# so if you change any file names in this file, RUN MAKE CLEAN FIRST!
DEBUGGING != if test -e build/debug; then echo true; else echo false; fi

# if you add directories here, make sure the deeper tree paths are listed before the shallow tree
# paths or you will get leftover directories in make clean
ALL_BUILD_DIRECTORIES = bin build/{libjs,jskit/scripts,jskit}

ifeq (all,$(TARGET))
# everything we build depends on these directories
.not_existing_directory_error != mkdir -p $(ALL_BUILD_DIRECTORIES)
endif

# if you want more defines, you can define them in file: build/defines
PROJECT_DEFINES = \
	XP_UNIX \
	SVR4 \
	SYSV \
	_DEFAULT_SOURCE \
	POSIX_SOURCE \
	HAVE_LOCALTIME_R \
	HAVE_VA_COPY \
	VA_COPY=va_copy \
	PIC \
	JS_THREADSAFE

ifeq (true,$(DEBUGGING))
DEBUG_FLAGS = -g3
PROJECT_DEFINES += DEBUG
endif

# not all .c files in the source/libjs directory are targets so this list object list is hand made.
BUILD_LIBRARY = \
                 \
	build/libjs/jsapi.o build/libjs/jsarena.o build/libjs/jsarray.o build/libjs/jsatom.o \
	build/libjs/jsbool.o build/libjs/jscntxt.o build/libjs/jsdate.o build/libjs/jsdbgapi.o \
	build/libjs/jsdhash.o build/libjs/jsdtoa.o build/libjs/jsemit.o build/libjs/jsexn.o \
	build/libjs/jsfile.o build/libjs/jsfun.o build/libjs/jsgc.o build/libjs/jshash.o \
	build/libjs/jsinterp.o build/libjs/jsinvoke.o build/libjs/jsiter.o build/libjs/jslock.o \
	build/libjs/jslog2.o build/libjs/jslong.o build/libjs/jsmath.o build/libjs/jsnum.o \
	build/libjs/jsobj.o build/libjs/jsopcode.o build/libjs/jsparse.o build/libjs/jsprf.o \
	build/libjs/jsregexp.o build/libjs/jsscan.o build/libjs/jsscope.o build/libjs/jsscript.o \
	build/libjs/jsstr.o build/libjs/jsutil.o build/libjs/jsxdrapi.o build/libjs/jsxml.o \
	build/libjs/prmjtime.o

BUILD_JS_CPUCFG_H = build/libjs/jsautocfg.h
BUILD_JS_KEYWORDS_H = build/libjs/jsautokw.h
BUILD_JS_KEYWORDS = build/libjs/jskwgen.o
BUILD_JS_CPUCFG = build/libjs/jscpucfg.o
BUILD_JS_AUTO_TOOLS = $(BUILD_JS_KEYWORDS) $(BUILD_JS_CPUCFG)
BUILD_JS_KIT = build/jskit/jskit.o

# all js kit scripts are automatic targets
BUILD_JSKIT_SCRIPTS != echo source/jskit/scripts/*.js | sed -E -e 's/\.js/.c/g' -e s/source/build/g

# automatic target generators
BUILD_JS_KEYWORDS_PROGRAM = bin/jskwgen
BUILD_JS_CPUCFG_PROGRAM = bin/jscpucfg
BUILD_BIN2INC_PROGRAM = bin/bin2inc

# production files
BUILD_JS_LIBRARY_ARCHIVE = build/libjs/libjskit.a
BUILD_JSKIT_PROGRAM = build/jskit/jskit

# object files
ALL_BUILT_OBJECTS = $(BUILD_BIN2INC_PROGRAM) \
  $(BUILD_LIBRARY) $(BUILD_JS_CPUCFG_H) $(BUILD_JS_KEYWORDS_H) \
	$(BUILD_JS_AUTO_TOOLS) $(BUILD_JSKIT_SCRIPTS) $(BUILD_JS_KIT)

# program files
ALL_BUILT_PROGRAMS = \
	$(BUILD_JS_CPUCFG_PROGRAM) $(BUILD_JS_KEYWORDS_PROGRAM) $(BUILD_JS_LIBRARY_ARCHIVE) \
	$(BUILD_JSKIT_PROGRAM) $(BUILD_BIN2INC_PROGRAM)

NSPR_CFLAGS = -I/usr/include/nspr
NSPR_LIBS = -lm -L/usr/lib -lplds4 -lplc4 -lnspr4

PROJECT_EXTERN_DEFINES != test -e build/defines && cat build/define

ifneq (,$(PROJECT_EXTERN_DEFINES))
PROJECT_DEFINES += $(PROJECT_EXTERN_DEFINES)
endif

ifneq (,$(PROJECT_DEFINES))
PROJECT_DEFINES := $(addprefix -D,$(PROJECT_DEFINES))
endif

all: dist

# everything we build depends on this Makefile
$(ALL_BUILT_OBJECTS): Makefile

# the lexical scanner requires automatic configuration
build/libjs/jsscan.o: $(BUILD_JS_KEYWORDS_H)

# declare that all build objects depend on autocpucfg, because source/libjs/jsapi.h requires it.
# doing this other ways could create problems, such as during make -j ...
$(BUILD_JS_KIT) $(BUILD_LIBRARY): $(BUILD_JS_CPUCFG_H)

build/libjs/%.o: source/libjs/%.c
	gcc -o $@ -c -Wall -Wno-format $(DEBUG_FLAGS) $(PROJECT_DEFINES) -Ibuild/libjs $(NSPR_CFLAGS) $<

build/jskit/%.o: source/jskit/%.c
	gcc -o $@ -c -Wall -Wno-format $(DEBUG_FLAGS) $(PROJECT_DEFINES) -DEDITLINE -Isource/libjs -Ibuild/libjs -Ibuild/jskit/scripts $(NSPR_CFLAGS) $<

$(BUILD_BIN2INC_PROGRAM): source/bin2inc/bin2inc.c
	gcc $< -o $@

bin/jscpucfg: build/libjs/jscpucfg.o
	gcc -o $@ $<

bin/jskwgen: build/libjs/jskwgen.o
	gcc -o $@ -lm $<

$(BUILD_JS_CPUCFG_H): bin/jscpucfg
	jscpucfg > $@

$(BUILD_JS_KEYWORDS_H): bin/jskwgen
	jskwgen $@

$(BUILD_JS_LIBRARY_ARCHIVE): $(BUILD_LIBRARY)
	ar rv $@ $(BUILD_LIBRARY)

$(BUILD_JS_KIT): $(BUILD_JSKIT_SCRIPTS)

build/jskit/scripts/%.c: source/jskit/scripts/%.js bin/bin2inc
	bin2inc "`basename $<`" < $< > $@

$(BUILD_JSKIT_PROGRAM): $(BUILD_JS_KIT) $(BUILD_JS_LIBRARY_ARCHIVE)
	gcc -o $@ $< $(BUILD_JS_LIBRARY_ARCHIVE) $(NSPR_LIBS) -lreadline

clean:
	-@rm -vf $(ALL_BUILT_OBJECTS) $(ALL_BUILT_PROGRAMS)
	-@rm -vf build/jskit/scripts/*.c # just in case any leftovers from a file rename; as we don't manage those files from within this file
	-@rm -vfd $(ALL_BUILD_DIRECTORIES)

lib-dist: $(BUILD_JS_LIBRARY_ARCHIVE)
	@mkdir -vp dist/lib dist/include
	@cp -vu $(BUILD_JS_LIBRARY_ARCHIVE) dist/lib
	@cp -vu source/libjs/*.h build/libjs/*.h dist/include

bin-dist: $(BUILD_JSKIT_PROGRAM)
	@mkdir -vp dist/bin
	@cp -vu $(BUILD_JSKIT_PROGRAM) dist/bin
	@strip dist/bin/`basename $(BUILD_JSKIT_PROGRAM)`

dist: lib-dist bin-dist

else

all:

debug: clean
	@mkdir -p build;
	@touch build/debug
	@HYPER_MAKE=1 PATH=$(shell realpath .)/bin:$(PATH) make --no-print-directory -j all

release: clean
	@rm -f build/debug
	@HYPER_MAKE=1 PATH=$(shell realpath .)/bin:$(PATH) make --no-print-directory -j all

%:
	@TARGET=$@ HYPER_MAKE=1 PATH=$(shell realpath .)/bin:$(PATH) make --no-print-directory -j $@

endif
