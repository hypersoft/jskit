ifdef HYPER_MAKE

# Making any changes to this file, will rebuild all files this file builds.
# so if you change any file names in this file, RUN MAKE CLEAN FIRST!
DEBUGGING != if test -e build/debug; then echo true; else echo false; fi

# if you add directories here, make sure the deeper tree paths are listed before the shallow tree
# paths or you will get leftover directories in make clean
ALL_BUILD_DIRECTORIES = bin build/{javascript,spider/scripts,spider}

ifneq (clean,$(TARGET))
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
BUILD_STRIP_SPIDER = : debug-build not stripped
else
BUILD_STRIP_SPIDER = strip dist/bin/$(shell basename $(BUILD_SPIDER_PROGRAM))
endif

# not all .c files in the source/javascript directory are targets so this list object list is hand made.
BUILD_LIBRARY = \
                 \
	build/javascript/jsapi.o build/javascript/jsarena.o build/javascript/jsarray.o build/javascript/jsatom.o \
	build/javascript/jsbool.o build/javascript/jscntxt.o build/javascript/jsdate.o build/javascript/jsdbgapi.o \
	build/javascript/jsdhash.o build/javascript/jsdtoa.o build/javascript/jsemit.o build/javascript/jsexn.o \
	build/javascript/jsfile.o build/javascript/jsfun.o build/javascript/jsgc.o build/javascript/jshash.o \
	build/javascript/jsinterp.o build/javascript/jsinvoke.o build/javascript/jsiter.o build/javascript/jslock.o \
	build/javascript/jslog2.o build/javascript/jslong.o build/javascript/jsmath.o build/javascript/jsnum.o \
	build/javascript/jsobj.o build/javascript/jsopcode.o build/javascript/jsparse.o build/javascript/jsprf.o \
	build/javascript/jsregexp.o build/javascript/jsscan.o build/javascript/jsscope.o build/javascript/jsscript.o \
	build/javascript/jsstr.o build/javascript/jsutil.o build/javascript/jsxdrapi.o build/javascript/jsxml.o \
	build/javascript/prmjtime.o

BUILD_JS_CPUCFG_H = build/javascript/jsautocfg.h
BUILD_JS_KEYWORDS_H = build/javascript/jsautokw.h
BUILD_JS_KEYWORDS = build/javascript/jskwgen.o
BUILD_JS_CPUCFG = build/javascript/jscpucfg.o
BUILD_JS_AUTO_TOOLS = $(BUILD_JS_KEYWORDS) $(BUILD_JS_CPUCFG)
BUILD_SPIDER = build/spider/spider.o

# all js kit scripts are automatic targets
BUILD_SPIDER_SCRIPTS != echo source/spider/scripts/*.js | sed -E -e 's/\.js/.c/g' -e s/source/build/g

# automatic target generators
BUILD_JS_KEYWORDS_PROGRAM = bin/jskwgen
BUILD_JS_CPUCFG_PROGRAM = bin/jscpucfg
BUILD_BIN2INC_PROGRAM = bin/bin2inc

BUILD_AUTO_PROGRAMS = $(BUILD_BIN2INC_PROGRAM) $(BUILD_JS_CPUCFG_PROGRAM) $(BUILD_JS_KEYWORDS_PROGRAM)

# production files
BUILD_JS_LIBRARY_ARCHIVE = build/javascript/libspider.a
BUILD_SPIDER_PROGRAM = build/spider/spider

# object files
ALL_BUILT_OBJECTS = $(BUILD_BIN2INC_PROGRAM) \
  $(BUILD_LIBRARY) $(BUILD_JS_CPUCFG_H) $(BUILD_JS_KEYWORDS_H) \
	$(BUILD_JS_AUTO_TOOLS) $(BUILD_SPIDER_SCRIPTS) $(BUILD_SPIDER)

# program files
ALL_BUILT_PROGRAMS = \
	$(BUILD_AUTO_PROGRAMS) $(BUILD_JS_LIBRARY_ARCHIVE) $(BUILD_SPIDER_PROGRAM) 

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
build/javascript/jsscan.o: $(BUILD_JS_KEYWORDS_H)

# declare that all build objects depend on autocpucfg, because source/javascript/jsapi.h requires it.
# doing this other ways could create problems, such as during make -j ...
$(BUILD_SPIDER) $(BUILD_LIBRARY): $(BUILD_JS_CPUCFG_H)

build/javascript/%.o: source/javascript/%.c
	gcc -o $@ -c -Wall -Wno-format $(DEBUG_FLAGS) $(PROJECT_DEFINES) -Ibuild/javascript $(NSPR_CFLAGS) $<

build/spider/%.o: source/spider/%.c
	gcc -o $@ -c -Wall -Wno-format $(DEBUG_FLAGS) $(PROJECT_DEFINES) -DEDITLINE -Isource/javascript -Ibuild/javascript -Ibuild/spider/scripts $(NSPR_CFLAGS) $<

$(BUILD_BIN2INC_PROGRAM): source/bin2inc/bin2inc.c
	gcc $< -o $@

bin/jscpucfg: build/javascript/jscpucfg.o
	gcc -o $@ $<

bin/jskwgen: build/javascript/jskwgen.o
	gcc -o $@ -lm $<

$(BUILD_JS_CPUCFG_H): bin/jscpucfg
	jscpucfg > $@

$(BUILD_JS_KEYWORDS_H): bin/jskwgen
	jskwgen $@

$(BUILD_JS_LIBRARY_ARCHIVE): $(BUILD_LIBRARY)
	ar rv $@ $(BUILD_LIBRARY)

$(BUILD_SPIDER): $(BUILD_SPIDER_SCRIPTS)
$(BUILD_SPIDER): $(shell find source/spider -maxdepth 1 -type f -name '*.c')

build/spider/scripts/%.c: source/spider/scripts/%.js bin/bin2inc
	bin2inc "`basename $<`" < $< > $@

$(BUILD_SPIDER_PROGRAM): $(BUILD_SPIDER) $(BUILD_JS_LIBRARY_ARCHIVE)
	gcc -o $@ $< $(DEBUG_FLAGS) $(BUILD_JS_LIBRARY_ARCHIVE) $(NSPR_LIBS) -lreadline

clean:
	-@rm -vf $(ALL_BUILT_OBJECTS) $(ALL_BUILT_PROGRAMS)
	-@rm -vf build/spider/scripts/*.c # just in case any leftovers from a file rename; as we don't manage those files from within this file
	-@rm -vfd $(ALL_BUILD_DIRECTORIES)

lib-dist: $(BUILD_JS_LIBRARY_ARCHIVE)
	@mkdir -vp dist/lib dist/include
	@cp -vu $(BUILD_JS_LIBRARY_ARCHIVE) dist/lib
	@cp -vu source/javascript/*.h build/javascript/*.h dist/include

bin-dist: $(BUILD_SPIDER_PROGRAM)
	@mkdir -vp dist/bin
	@cp -vu $(BUILD_SPIDER_PROGRAM) dist/bin
	$(BUILD_STRIP_SPIDER)

dist: lib-dist bin-dist

debug: clean
	@mkdir -p build;
	@touch build/debug

release: clean
	@rm -f build/debug

else

all:

slow:
	@TARGET=all HYPER_MAKE=1 PATH=$(shell realpath .)/bin:$(PATH) make --no-print-directory all

%:
	@TARGET=$@ HYPER_MAKE=1 PATH=$(shell realpath .)/bin:$(PATH) make --no-print-directory -j $@

endif
