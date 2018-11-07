mkdir -vp `dirname build/javascript/jscpucfg.o`
gcc -o build/javascript/jscpucfg.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jscpucfg.c
mkdir -vp `dirname bin/jscpucfg`
gcc -o bin/jscpucfg build/javascript/jscpucfg.o
mkdir -vp `dirname build/javascript/jsautocfg.h`
jscpucfg > build/javascript/jsautocfg.h
mkdir -vp `dirname bin/bin2inc`
gcc source/bin2inc/bin2inc.c -o bin/bin2inc
mkdir -vp `dirname build/spider/scripts/object-keys-patch.c`
bin2inc "`basename source/spider/scripts/object-keys-patch.js`" < source/spider/scripts/object-keys-patch.js > build/spider/scripts/object-keys-patch.c
mkdir -vp `dirname build/javascript/jsapi.o`
gcc -o build/javascript/jsapi.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsapi.c
mkdir -vp `dirname build/javascript/jsarena.o`
gcc -o build/javascript/jsarena.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsarena.c
mkdir -vp `dirname build/javascript/jsarray.o`
gcc -o build/javascript/jsarray.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsarray.c
mkdir -vp `dirname build/javascript/jsatom.o`
gcc -o build/javascript/jsatom.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsatom.c
mkdir -vp `dirname build/javascript/jsbool.o`
gcc -o build/javascript/jsbool.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsbool.c
mkdir -vp `dirname build/javascript/jscntxt.o`
gcc -o build/javascript/jscntxt.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jscntxt.c
mkdir -vp `dirname build/javascript/jsdate.o`
gcc -o build/javascript/jsdate.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsdate.c
mkdir -vp `dirname build/javascript/jsdbgapi.o`
gcc -o build/javascript/jsdbgapi.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsdbgapi.c
mkdir -vp `dirname build/javascript/jsdhash.o`
gcc -o build/javascript/jsdhash.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsdhash.c
mkdir -vp `dirname build/javascript/jsdtoa.o`
gcc -o build/javascript/jsdtoa.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsdtoa.c
mkdir -vp `dirname build/javascript/jsemit.o`
gcc -o build/javascript/jsemit.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsemit.c
mkdir -vp `dirname build/javascript/jsexn.o`
gcc -o build/javascript/jsexn.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsexn.c
mkdir -vp `dirname build/javascript/jsfile.o`
gcc -o build/javascript/jsfile.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsfile.c
mkdir -vp `dirname build/javascript/jsfun.o`
gcc -o build/javascript/jsfun.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsfun.c
mkdir -vp `dirname build/javascript/jsgc.o`
gcc -o build/javascript/jsgc.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsgc.c
mkdir -vp `dirname build/javascript/jshash.o`
gcc -o build/javascript/jshash.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jshash.c
mkdir -vp `dirname build/javascript/jsinterp.o`
gcc -o build/javascript/jsinterp.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsinterp.c
mkdir -vp `dirname build/javascript/jsinvoke.o`
gcc -o build/javascript/jsinvoke.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsinvoke.c
mkdir -vp `dirname build/javascript/jsiter.o`
gcc -o build/javascript/jsiter.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsiter.c
mkdir -vp `dirname build/javascript/jslock.o`
gcc -o build/javascript/jslock.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jslock.c
mkdir -vp `dirname build/javascript/jslog2.o`
gcc -o build/javascript/jslog2.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jslog2.c
mkdir -vp `dirname build/javascript/jslong.o`
gcc -o build/javascript/jslong.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jslong.c
mkdir -vp `dirname build/javascript/jsmath.o`
gcc -o build/javascript/jsmath.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsmath.c
mkdir -vp `dirname build/javascript/jsnum.o`
gcc -o build/javascript/jsnum.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsnum.c
mkdir -vp `dirname build/javascript/jsobj.o`
gcc -o build/javascript/jsobj.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsobj.c
mkdir -vp `dirname build/javascript/jsopcode.o`
gcc -o build/javascript/jsopcode.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsopcode.c
mkdir -vp `dirname build/javascript/jsparse.o`
gcc -o build/javascript/jsparse.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsparse.c
mkdir -vp `dirname build/javascript/jsprf.o`
gcc -o build/javascript/jsprf.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsprf.c
mkdir -vp `dirname build/javascript/jsregexp.o`
gcc -o build/javascript/jsregexp.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsregexp.c
mkdir -vp `dirname build/javascript/jskwgen.o`
gcc -o build/javascript/jskwgen.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jskwgen.c
mkdir -vp `dirname bin/jskwgen`
gcc -o bin/jskwgen -lm build/javascript/jskwgen.o
mkdir -vp `dirname build/javascript/jsautokw.h`
jskwgen build/javascript/jsautokw.h
mkdir -vp `dirname build/javascript/jsscan.o`
gcc -o build/javascript/jsscan.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsscan.c
mkdir -vp `dirname build/javascript/jsscope.o`
gcc -o build/javascript/jsscope.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsscope.c
mkdir -vp `dirname build/javascript/jsscript.o`
gcc -o build/javascript/jsscript.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsscript.c
mkdir -vp `dirname build/javascript/jsstr.o`
gcc -o build/javascript/jsstr.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsstr.c
mkdir -vp `dirname build/javascript/jsutil.o`
gcc -o build/javascript/jsutil.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsutil.c
mkdir -vp `dirname build/javascript/jsxdrapi.o`
gcc -o build/javascript/jsxdrapi.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsxdrapi.c
mkdir -vp `dirname build/javascript/jsxml.o`
gcc -o build/javascript/jsxml.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/jsxml.c
mkdir -vp `dirname build/javascript/prmjtime.o`
gcc -o build/javascript/prmjtime.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -Ibuild/javascript -I/usr/include/nspr source/javascript/prmjtime.c
mkdir -vp `dirname build/javascript/libspider.a`
ar rv build/javascript/libspider.a build/javascript/jsapi.o build/javascript/jsarena.o build/javascript/jsarray.o build/javascript/jsatom.o build/javascript/jsbool.o build/javascript/jscntxt.o build/javascript/jsdate.o build/javascript/jsdbgapi.o build/javascript/jsdhash.o build/javascript/jsdtoa.o build/javascript/jsemit.o build/javascript/jsexn.o build/javascript/jsfile.o build/javascript/jsfun.o build/javascript/jsgc.o build/javascript/jshash.o build/javascript/jsinterp.o build/javascript/jsinvoke.o build/javascript/jsiter.o build/javascript/jslock.o build/javascript/jslog2.o build/javascript/jslong.o build/javascript/jsmath.o build/javascript/jsnum.o build/javascript/jsobj.o build/javascript/jsopcode.o build/javascript/jsparse.o build/javascript/jsprf.o build/javascript/jsregexp.o build/javascript/jsscan.o build/javascript/jsscope.o build/javascript/jsscript.o build/javascript/jsstr.o build/javascript/jsutil.o build/javascript/jsxdrapi.o build/javascript/jsxml.o build/javascript/prmjtime.o
mkdir -vp dist/lib dist/include
cp -vu build/javascript/libspider.a dist/lib
cp -vu source/javascript/*.{h,tbl,msg} build/javascript/*.h dist/include
mkdir -vp `dirname build/spider/spider.o`
gcc -o build/spider/spider.o -c -Wall -Wno-format  -DXP_UNIX -DSVR4 -DSYSV -D_DEFAULT_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_THREADSAFE -DEDITLINE -Idist/include -Ibuild/spider/scripts -I/usr/include/nspr source/spider/spider.c
mkdir -vp `dirname build/spider/spider`
gcc -o build/spider/spider build/spider/spider.o  build/javascript/libspider.a -lm -L/usr/lib -lplds4 -lplc4 -lnspr4 -lreadline
mkdir -vp dist/bin
cp -vu build/spider/spider dist/bin
strip dist/bin/spider
