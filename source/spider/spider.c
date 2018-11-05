
/*
 * JS shell.
 */
#include "jsstddef.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "jstypes.h"
#include "jsarena.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsparse.h"
#include "jsscope.h"
#include "jsscript.h"

#include <stdbool.h>

#include <nspr.h>

#include <stdint.h>
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYSTEM_CPU_BITS 32
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYSTEM_CPU_BITS 64
#else
/* wtf */
#endif

#include "object-keys-patch.c"

JSRuntime *rt;
JSContext *cx;

void JS_FreeNativeStrings(JSContext * cx, ...) {
    va_list args;
    va_start(args, cx);
		void * f;
		while ((f = va_arg(args, void*))) JS_free(cx, f);
		va_end(args);
}

#define STRINGIZE(x) #x
#define M180_GetCompilerString(x) STRINGIZE(x)

/**
 * This override ensures the final parameter to the procedure of the same name is null
 */

#define JS_ValueToNativeString(CX, JSVAL) ((char *) JS_EncodeString(CX, JS_ValueToString(CX, JSVAL)))
#define JS_FreeNativeString(CX, STRING) JS_free(CX, STRING)

#define JS_FreeNativeStrings(CX, ...) JS_FreeNativeStrings(CX, __VA_ARGS__, NULL)

/**
 * M180 Requires functions to have cx and vp named for these to work
 */
#define JS_ReturnValueEscape(VAL, ...) JS_FreeNativeStrings((JSContext *)cx, __VA_ARGS__); JS_ReturnValue(VAL)
#define JS_ReturnValue(VAL) JS_SET_RVAL((JSContext *)cx, vp, VAL); return JS_TRUE
#define JS_ReturnError() return JS_FALSE
#define JS_ReturnErrorEscape(...) JS_FreeNativeStrings((JSContext *)cx, __VA_ARGS__); JS_ReturnError()

/**
 * Report an error with 1 format parameter, free supplied strings and return from the procedure
*/ 
#define JS_ReturnException(FORMAT, ...) JS_ReportError((JSContext *)cx, FORMAT, __VA_ARGS__); JS_FreeNativeStrings((JSContext *)cx, __VA_ARGS__); JS_ReturnError()
/**
 * Report an error with 2 format parameters, free supplied strings and return from the procedure
*/ 
#define JS_ReturnExceptionEscape(FORMAT, MSG, ...) JS_ReportError((JSContext *)cx, FORMAT, MSG); JS_FreeNativeStrings((JSContext *)cx, __VA_ARGS__); JS_ReturnError()
/**
 * Report an error with 3 format parameters, free supplied strings and return from the procedure
*/ 
#define JS_ReturnExceptionEscape2(FORMAT, MSG, MSG2, ...) JS_ReportError((JSContext *)cx, FORMAT, MSG, MSG2); JS_FreeNativeStrings((JSContext *)cx, __VA_ARGS__); JS_ReturnError()

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*-------------------------------------------------------------------------*//**
 * @brief      Set if the inputs must be displayed or not.
 *
 * @param[in]  display  True for display, false for no display
 */
void displayInputs(bool display);

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

#include <windows.h>

void displayInputs(bool display) {
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if(hStdIn == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetStdHandle failed (error %lu)\n", GetLastError());
        return;
    }

    /* Get console mode */
    DWORD mode;
    if(!GetConsoleMode(hStdIn, &mode)) {
        fprintf(stderr, "GetConsoleMode failed (error %lu)\n", GetLastError());
        return;
    }

    if(display) {
        /* Add echo input to the mode */
        if(!SetConsoleMode(hStdIn, mode | ENABLE_ECHO_INPUT)) {
            fprintf(stderr, "SetConsoleMode failed (error %lu)\n", GetLastError());
            return;
        }
    }
    else {
        /* Remove echo input to the mode */
        if(!SetConsoleMode(hStdIn, mode & ~((DWORD) ENABLE_ECHO_INPUT))) {
            fprintf(stderr, "SetConsoleMode failed (error %lu)\n", GetLastError());
            return;
        }
    }
}

#else

#include <termios.h>

void displayInputs(bool display) {
    struct termios t;

    /* Get console mode */
    errno = 0;
    if (tcgetattr(STDIN_FILENO, &t)) {
        fprintf(stderr, "tcgetattr failed (%s)\n", strerror(errno));
        return;
    }

    /* Set console mode to echo or no echo */
    if (display) {
        t.c_lflag |= ECHO;
    } else {
        t.c_lflag &= ~((tcflag_t) ECHO);
    }
    errno = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
        fprintf(stderr, "tcsetattr failed (%s)\n", strerror(errno));
        return;
    }
}

#endif

#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>     /* for isatty() */
#endif

typedef enum JSShellExitCode {
    EXITCODE_RUNTIME_ERROR      = 3,
    EXITCODE_FILE_NOT_FOUND     = 4,
    EXITCODE_OUT_OF_MEMORY      = 5
} JSShellExitCode;

size_t gStackChunkSize = 8192;

/* Assume that we can not use more than 5e5 bytes of C stack by default. */
static size_t gMaxStackSize = 500000;
static jsuword gStackBase;

static size_t gScriptStackQuota = JS_DEFAULT_SCRIPT_STACK_QUOTA;

static JSBool gEnableBranchCallback = JS_FALSE;
static uint32 gBranchCount;
static uint32 gBranchLimit;

int gExitCode = 0;
JSBool gQuitting = JS_FALSE;
FILE *gErrFile = NULL;
FILE *gOutFile = NULL;

static JSBool reportWarnings = JS_TRUE;
static JSBool compileOnly = JS_FALSE;

typedef enum JSShellErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "jsshell.msg"
#undef MSG_DEF
    JSShellErr_Limit
#undef MSGDEF
} JSShellErrNum;

static const JSErrorFormatString *
my_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

#ifdef EDITLINE
JS_BEGIN_EXTERN_C
extern char     *readline(const char *prompt);
extern void     add_history(char *line);
JS_END_EXTERN_C
#endif

static JSBool
GetLine(JSContext *cx, char *bufp, FILE *file, const char *prompt) {
#ifdef EDITLINE
    /*
     * Use readline only if file is stdin, because there's no way to specify
     * another handle.  Are other filehandles interactive?
     */
    if (file == stdin) {
        char *linep = readline(prompt);
        if (!linep)
            return JS_FALSE;
        if (linep[0] != '\0')
            add_history(linep);
        strcpy(bufp, linep);
        JS_free(cx, linep);
        bufp += strlen(bufp);
        *bufp++ = '\n';
        *bufp = '\0';
    } else
#endif
    {
        char line[256];
        //fprintf(gOutFile, prompt);
        fflush(gOutFile);
        if (!fgets(line, sizeof line, file))
            return JS_FALSE;
        strcpy(bufp, line);
    }
    return JS_TRUE;
}

static JSBool
my_BranchCallback(JSContext *cx, JSScript *script)
{
    if (++gBranchCount == gBranchLimit) {
        if (script) {
            if (script->filename)
                fprintf(gErrFile, "%s:", script->filename);
            fprintf(gErrFile, "%u: script branch callback (%u callbacks)\n",
                    script->lineno, gBranchLimit);
        } else {
            fprintf(gErrFile, "native branch callback (%u callbacks)\n",
                    gBranchLimit);
        }
        gBranchCount = 0;
        return JS_FALSE;
    }
#ifdef JS_THREADSAFE
    if ((gBranchCount & 0xff) == 1) {
#endif
        if ((gBranchCount & 0x3fff) == 1)
            JS_MaybeGC(cx);
#ifdef JS_THREADSAFE
        else
            JS_YieldRequest(cx);
    }
#endif
    return JS_TRUE;
}

static void
SetContextOptions(JSContext *cx)
{
    jsuword stackLimit;

    if (gMaxStackSize == 0) {
        /*
         * Disable checking for stack overflow if limit is zero.
         */
        stackLimit = 0;
    } else {
#if JS_STACK_GROWTH_DIRECTION > 0
        stackLimit = gStackBase + gMaxStackSize;
#else
        stackLimit = gStackBase - gMaxStackSize;
#endif
    }
    JS_SetThreadStackLimit(cx, stackLimit);
    JS_SetScriptStackQuota(cx, gScriptStackQuota);
    if (gEnableBranchCallback) {
        JS_SetBranchCallback(cx, my_BranchCallback);
        JS_ToggleOptions(cx, JSOPTION_NATIVE_BRANCH_CALLBACK);
    }
}

static void
Process(JSContext *cx, JSObject *obj, char *filename, JSBool forceTTY)
{
    JSBool ok, hitEOF;
    JSScript *script;
    jsval result;
    JSString *str;
    char buffer[4096];
    char *bufp;
    int lineno;
    int startline;
    FILE *file;

    if (forceTTY || !filename || strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                                 JSSMSG_CANT_OPEN, filename, strerror(errno));
            gExitCode = EXITCODE_FILE_NOT_FOUND;
            return;
        }
    }

    SetContextOptions(cx);

    if (!forceTTY && !isatty(fileno(file))) {
        /*
         * It's not interactive - just execute it.
         *
         * Support the UNIX #! shell hack; gobble the first line if it starts
         * with '#'.  TODO - this isn't quite compatible with sharp variables,
         * as a legal js program (using sharp variables) might start with '#'.
         * But that would require multi-character lookahead.
         */
        int ch = fgetc(file);
        if (ch == '#') {
            while((ch = fgetc(file)) != EOF) {
                if (ch == '\n' || ch == '\r')
                    break;
            }
        }
        ungetc(ch, file);
        script = JS_CompileFileHandle(cx, obj, filename, file);
        if (script) {
            if (!compileOnly)
                (void)JS_ExecuteScript(cx, obj, script, &result);
            JS_DestroyScript(cx, script);
        }

        if (file != stdin)
            fclose(file);
        return;
    }

    /* It's an interactive filehandle; drop into read-eval-print loop. */
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        /*
         * Accumulate lines until we get a 'compilable unit' - one that either
         * generates an error (before running out of source) or that compiles
         * cleanly.  This should be whenever we get a complete statement that
         * coincides with the end of a line.
         */
        startline = lineno;
        do {
            if (!GetLine(cx, bufp, file, startline == lineno ? "spider: " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, obj, buffer, strlen(buffer)));

        /* Clear any pending exception from previous failed compiles.  */
        JS_ClearPendingException(cx);
        script = JS_CompileScript(cx, obj, buffer, strlen(buffer), "console",
                                  startline);
        if (script) {
            if (!compileOnly) {
                ok = JS_ExecuteScript(cx, obj, script, &result);
                if (ok && result != JSVAL_VOID) {
                    str = JS_ValueToString(cx, result);
                    if (str)
                        fprintf(gOutFile, "%s\n", JS_GetStringBytes(str));
                    else
                        ok = JS_FALSE;
                }
            }
            JS_DestroyScript(cx, script);
        }
    } while (!hitEOF && !gQuitting);
    fprintf(gOutFile, "\n");
    if (file != stdin)
        fclose(file);
    return;
}

extern JSClass global_class;

static int
ProcessArgs(JSContext *cx, JSObject *obj, char **argv, int argc)
{
    JSObject *argsObj;
    char *filename = NULL; //(argc)?argv[0]:NULL;
    //JSBool isInteractive = JS_TRUE;
    JSBool interactive = JS_TRUE;
    JSBool forceTTY = JS_FALSE;

    while (argc) {
        if (strcmp(argv[0], "-s" ) == 0 ||  strcmp(argv[0], "--shell-script") == 0) {
            filename = argv[1]; argc--; argv++;
            break;
        }
        if (strcmp(argv[0], "-i") == 0) {
            argc--; argv++;
            forceTTY = JS_TRUE;
            continue;
        }
        break;
    }
    /*
     * Create arguments early and define it to root it, so it's safe from any
     * GC calls nested below, and so it is available to -f <file> arguments.
     */
    argsObj = JS_NewArrayObject(cx, 0, NULL);
    if (!argsObj)
        return 1;
    if (!JS_DefineProperty(cx, obj, "parameter", OBJECT_TO_JSVAL(argsObj),
                           NULL, NULL, 0)) {
        return 1;
    }

    int i = 0;
    for (i = 0; i < argc; i++) {
        JSString *str = JS_NewStringCopyZ(cx, argv[i]);
        if (!str)
            return 1;
        if (!JS_DefineElement(cx, argsObj, i, STRING_TO_JSVAL(str),
                              NULL, NULL, JSPROP_ENUMERATE)) {
            return 1;
        }
    }

    if (filename) {
        Process(cx, obj, filename, forceTTY);
    } 
    
    else if (interactive == JS_TRUE) {
       Process(cx, obj, NULL, forceTTY);
    }
    return gExitCode;
}

JSErrorFormatString jsShell_ErrorFormatString[JSErr_Limit] = {
#define MSG_DEF(name, number, count, exception, format) \
    { format, count, JSEXN_ERR } ,
#include "jsshell.msg"
#undef MSG_DEF
};

static const JSErrorFormatString *
my_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSShellErr_Limit))
        return &jsShell_ErrorFormatString[errorNumber];
    return NULL;
}

static void
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;
    char *prefix, *tmp;
    const char *ctmp;

    if (!report) {
        fprintf(gErrFile, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return;

    prefix = NULL;
    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ",
                             tmp ? tmp : "",
                             JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    /* embedded newlines -- argh! */
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix)
            fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }

    /* If there were no filename or lineno, the prefix might be empty */
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    /* report->linebuf usually ends with a newline. */
    n = strlen(report->linebuf);
    fprintf(gErrFile, ":\n%s%s%s%s",
            prefix,
            report->linebuf,
            (n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n",
            prefix);
    n = PTRDIFF(report->tokenptr, report->linebuf, char);
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    if (!JSREPORT_IS_WARNING(report->flags)) {
        if (report->errorNumber == JSMSG_OUT_OF_MEMORY) {
            gExitCode = EXITCODE_OUT_OF_MEMORY;
        } else {
            gExitCode = EXITCODE_RUNTIME_ERROR;
        }
    }
    JS_free(cx, prefix);
}

static JSBool
global_enumerate(JSContext *cx, JSObject *obj)
{
#ifdef LAZY_STANDARD_CLASSES
    return JS_EnumerateStandardClasses(cx, obj);
#else
    return JS_TRUE;
#endif
}

static JSBool
global_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
               JSObject **objp)
{
#ifdef LAZY_STANDARD_CLASSES
    if ((flags & JSRESOLVE_ASSIGNING) == 0) {
        JSBool resolved;

        if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
            return JS_FALSE;
        if (resolved) {
            *objp = obj;
            return JS_TRUE;
        }
    }
#endif

    return JS_TRUE;

}

JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    global_enumerate, (JSResolveOp) global_resolve,
    JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
ContextCallback(JSContext *cx, uintN contextOp)
{
    if (contextOp == JSCONTEXT_NEW) {
        JS_SetErrorReporter(cx, my_ErrorReporter);
        JS_SetVersion(cx, JSVERSION_LATEST);
        SetContextOptions(cx);
    }
    return JS_TRUE;
}


#ifndef SEEK_SET
    #define SEEK_SET	0	/* Seek from beginning of file.  */
    #define SEEK_CUR	1	/* Seek from current position.  */
    #define SEEK_END	2	/* Seek from end of file.  */
#endif

typedef struct StringBuffer {

   char * address;
   size_t index;
   size_t allocated;
   size_t limit;

} StringBuffer;

StringBuffer NewStringBuffer() {
    return (StringBuffer) { 0,0,0,0 };
}

// Clone StringBuffer values
// Cast StringBuffer pointer to structure and return by value.
// requires compiler option -fno-strict-aliasing
StringBuffer StringBufferProperties(register StringBuffer *buffer) {
    return (StringBuffer) *buffer;
}

// Setup a new string buffer
int StringBufferInit(register StringBuffer * source, size_t allocate, size_t limit) {

    if (limit && allocate > limit) return 0;

   source->address = (char *) malloc(allocate*sizeof(unsigned char));

   source->allocated = allocate;
   source->index = 0;
   source->limit = limit;

    if (source->address) {

      source->address[0] = 0; // initialize ANSI compatible "string"

      return source->allocated;

    }

    return 0;

}

// #requiring <string.h>, <stdlib.h>

// Append a string to the current buffer position (indicated by index)
// This procedure guards against possible memory leak with realloc, employing
// a result status checked temp variable. The procedure is also capable of
// overlapped memory i/o.

int StringBufferWrite(register StringBuffer * buffer, const char * src) {

  if (!buffer || !src) return 0;

  size_t len = strlen(src) + 1;

  register size_t terminationNode = ( len + buffer->index );

  if ( buffer->limit && ( terminationNode > buffer->limit ) ) return 0;

  if ( buffer->allocated < terminationNode ) {

      void * NewMemoryBlock = realloc(buffer->address, terminationNode);
      if (!NewMemoryBlock) return 0;

      buffer->address = (char *) NewMemoryBlock;
      buffer->allocated = terminationNode;

  }

  memmove((buffer->address + buffer->index), src, len);

  buffer->index += --len; // set buffer index 1 position ahead of terminator

  return 1;

}

// Erase everything from buffer index to extent, if extent exceeds
// allocated space, the request is truncated. If extent is null, extent is
// rectified to buffer allocated through index
size_t StringBufferClear(register StringBuffer * buffer, register size_t extent) {

    // I cannot possibly clear "nothing" so if extent is zero,
    // clear  buffer content from: address+index to address+allocated

    if ( ! buffer->address || ! buffer->allocated ) return 0;

    register size_t id = ( extent + buffer->index );

    if ( id > buffer->allocated || ! extent ) {
        id = ( buffer->allocated - buffer->index );
    }

    extent = id;

    for ( id = buffer->index; id < extent; id++ ) {

        buffer->address[id] = 0; // turn off the bits

    }

    return 1;

}

// Free the address pointer, unset all members
void StringBufferRelease(register StringBuffer * buffer) {

  if (!buffer->address || ! buffer->allocated) return;

  free(buffer->address);

  *(buffer) = NewStringBuffer();

}

// Set the buffer index stdio fseek() style and (+2)
// return the resulting address located at the index.
// ZERO == FAIL
char * StringBufferSeek(register StringBuffer * b, size_t i, int o) {

  // Use stdio SEEK_* flags for o (origin)

  register size_t t; // resident temp once again..

  if (!b->address || !b->allocated) return 0;

  if (o == SEEK_CUR) {
     t = b->index + i;
     if (t > b->allocated || t < 0) return 0;
     b->index = t;
     return (b->address+t);
  }

  if (o == SEEK_SET) {
     if (i < 0 || i > b->allocated) return 0;
     b->index = i;
     return(b->address+i);
  }

  if (o == SEEK_END) {
     if (i > 0) return 0;
     t = b->allocated + i;
     if (t < 0) return 0;
     b->index = t;
     return(b->address+t);
  }

  return 0;

}

// Clone StringBuffer values and data
// Clone StringBuffer values, create an identical buffer,
// copy the source data to the new buffer and assign the
// new buffer address to the StringBuffer copy, returning
// the duplicate by value
StringBuffer StringBufferDuplicate(register StringBuffer *buffer) {

    register StringBuffer duplicate = StringBufferProperties(buffer);

    duplicate.address = strncpy(

        malloc(
            duplicate.allocated
        ),

        duplicate.address,
        duplicate.allocated

    );

    return duplicate;

}

// Directly set the allocated size of the buffer
// Return: non-zero success
int StringBufferExtent(register StringBuffer *buffer, size_t size) {

    if (buffer->limit && size > buffer->limit) return 0;

    void *t = 0;

    if (buffer->address) {

        if (size == 0) { // destructive request
            free(buffer->address);
            buffer->allocated = 0;
            buffer->index = 0;
            buffer->address = 0;
            return 1;
        }

        t = realloc(buffer->address, size);

    } else {

        if (size == 0) {
            // make sure allocated is zero
            buffer->allocated = 0; return 0;
        }

        t = malloc(size);

    }

    if (!t) return 0;

    buffer->address = (char *) t;
    buffer->address[0] = 0;
    buffer->allocated = size;

    return 1;

}

void JS_FreeNativeStringArray(JSContext * cx, char * b[], int c) {
    int i = 0;
    while (i < c) JS_free(cx, b[i++]);
}

static JSBool ExecScriptFile(JSContext * context, JSObject * global, const char * file, jsval * jsResult) {

	register FILE * filep;

	filep = fopen(file, "rb");

	if (!filep) {

		JS_ReportError(context, "%s: %s", file, "No such file or directory");

		return 0;

	}

	JSScript * jsTemp = JS_CompileFileHandle(context, global, file, filep);

	if (jsTemp) {

		return JS_ExecuteScript(context, global, jsTemp, jsResult);

	} else {

		JS_ReportPendingException(context);

		return 0;

	}

}

static JSBool ShellInclude(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc == 0) {
        return JS_FALSE;
    }

	JSBool status;
    char * string[1];
    string[0] = JS_ValueToNativeString(cx, argv[0]);
	status = ExecScriptFile(cx, JS_GetGlobalObject(cx), string[0], rval);
    JS_FreeNativeStringArray(cx, &string[0], 1);
	return status;

}

static JSBool ShellReadline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    char * string[2];
    jsval out;
    string[0] = JS_ValueToNativeString(cx, argv[0]);
    string[1] = readline(string[0]);
    if (!string[1]) {
        JS_FreeNativeStringArray(cx, string, 1);
        return JS_FALSE;
    }
    if (string[1][0] != '\0')
        add_history(string[1]);
    out = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, string[1]));

    JS_FreeNativeStringArray(cx, string, 2);
    JS_ReturnValue(out);

}

static JSBool ShellReadPassword(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    char * string[2];
    jsval out;
    string[0] = JS_ValueToNativeString(cx, argv[0]);
    displayInputs(false);
    string[1] = readline(string[0]);
    displayInputs(true);
    if (!string[1]) {
        JS_FreeNativeStringArray(cx, string, 1);
        return JS_FALSE;
    }
    out = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, string[1]));
    JS_FreeNativeStringArray(cx, string, 2);
    JS_ReturnValue(out);

}

static JSBool ShellClear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

  char * key = JS_ValueToNativeString(cx, argv[0]);

	if (!key) {
		JS_ReturnException("failed to get environment variable key from javascript", NULL);
	}

	unsetenv(key);
	JS_ReturnValueEscape(JSVAL_TRUE, key);
}

extern char ** environ;

static JSBool ShellKeys(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	JSObject* argsObj = JS_NewArrayObject(cx, 0, NULL);
	int i = 0;
	for (int x = 0; environ[x]; ++x) {
		int split = 0;
		char * variable = environ[x];
		char t;
		while ((t = variable[split]) != 0 && t != '=') split++;
		JSString * str = JS_NewStringCopyN(cx, variable, split);
		JS_DefineElement(cx, argsObj, i++, STRING_TO_JSVAL(str), NULL, NULL, JSPROP_ENUMERATE);
	}
	JS_ReturnValue(OBJECT_TO_JSVAL(argsObj));
}

static JSBool ShellGet(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
  char * key = JS_ValueToNativeString(cx, argv[0]);
	
	if (!key) {
		JS_ReturnException("failed to get environment variable key from javascript", NULL);
	}

	char * variable = getenv(key);
  JS_free(cx, key);

	if (variable == NULL) {
		JS_ReturnValue(JSVAL_NULL);
	}

	JS_ReturnValue(STRING_TO_JSVAL(JS_NewStringCopyZ(cx, variable)));
}

static JSBool ShellSet(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
  char * filename = JS_ValueToNativeString(cx, argv[0]);
	
	if (!filename) {
		JS_ReturnException("failed to get environment variable key from javascript", NULL);
	}
  
	char * contents = JS_ValueToNativeString(cx, argv[1]);
	
	if (!contents) {
		JS_ReturnExceptionEscape("failed to get data for environment variable: %s; from javascript", filename, filename);
	}

	int overwrite = (argc > 2) ? JSVAL_TO_INT(argv[2]) : 1;

	int r = setenv(filename, contents, overwrite);

	JS_ReturnValueEscape(BOOLEAN_TO_JSVAL(r == 0), filename, contents);
}

static JSBool ShellFileStat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * filename = JS_ValueToNativeString(cx, argv[0]);

	if (!filename) {
		JS_ReturnException("can't convert file name to string", NULL);
	}

	JSObject * out = JS_NewObject(cx, NULL, NULL, NULL);

#if (SYSTEM_CPU_BITS == 64)

	PRFileInfo64 fInfo;
	if (PR_GetFileInfo64(filename, &fInfo) != PR_SUCCESS) {
		JS_ReturnExceptionEscape("failed to read status of file handle for: %s", filename, filename);
	};

	JS_DefineProperty(cx, out, "type", INT_TO_JSVAL(fInfo.type), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "creationTime", INT_TO_JSVAL(fInfo.creationTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "modificationTime", INT_TO_JSVAL(fInfo.modifyTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "size", INT_TO_JSVAL(fInfo.size), NULL, NULL, JSPROP_ENUMERATE);

#else

	PRFileInfo fInfo;
	if (PR_GetFileInfo(filename, &fInfo) != PR_SUCCESS) {
		JS_ReturnExceptionEscape("failed to read file stats of: %s", filename, filename);
	};

	JS_DefineProperty(cx, out, "type", INT_TO_JSVAL(fInfo.type), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "creationTime", INT_TO_JSVAL(fInfo.creationTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "modificationTime", INT_TO_JSVAL(fInfo.modifyTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "size", INT_TO_JSVAL(fInfo.size), NULL, NULL, JSPROP_ENUMERATE);

#endif
	
	JS_DefineProperty(cx, out, "writable", BOOLEAN_TO_JSVAL(PR_Access(filename, PR_ACCESS_WRITE_OK) == PR_SUCCESS), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "readable", BOOLEAN_TO_JSVAL(PR_Access(filename, PR_ACCESS_READ_OK) == PR_SUCCESS), NULL, NULL, JSPROP_ENUMERATE);

	JS_ReturnValue(OBJECT_TO_JSVAL(out));

}

static JSBool ShellSetFileContent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * filename = JS_ValueToNativeString(cx, argv[0]);
	if (!filename) {
		JS_ReturnException("failed to get file name from javascript", NULL);
	}
	// todo: abort if file is a directory

	char * contents = JS_ValueToNativeString(cx, argv[1]);

	FILE * file = fopen(filename, "w");
	if (!file) {
		JS_ReturnExceptionEscape("failed to open file: %s; for writing", filename, filename, contents);
	}

	long contentLength = strlen(contents);

	fwrite(contents, 1, contentLength, file);
	fclose(file);

	JS_ReturnValueEscape(DOUBLE_TO_JSVAL(contentLength), filename, contents);

}

static JSBool ShellPrintFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * filename = JS_ValueToNativeString(cx, argv[0]);
	if (!filename) {
		JS_ReturnException("failed to get file name from javascript", NULL);
	}
	// todo: abort if file is a directory

	char * contents = JS_ValueToNativeString(cx, argv[1]);

	FILE * file = fopen(filename, "a");
	if (!file) {
		JS_ReturnExceptionEscape("failed to open file: %s; for appending", filename, filename, contents);
	}

	long contentLength = strlen(contents);

	fwrite(contents, 1, contentLength, file);
	fclose(file);

	JS_ReturnValueEscape(DOUBLE_TO_JSVAL(contentLength), filename, contents);
}

static JSBool ShellGetFileContent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

	char * cmd = JS_ValueToNativeString(cx, argv[0]);
	if (!cmd) {
		JS_ReturnException("failed to get file name from javascript", NULL);
	}

	PRFileDesc * fp = PR_Open(cmd, PR_RDONLY, 0);
	if (fp == NULL) {
		JS_ReturnExceptionEscape("failed to open file: %s", cmd, cmd);
	}

	char *buf = NULL;
	size_t len = 0;

#if (SYSTEM_CPU_BITS == 64)

	PRFileInfo64 fInfo;
	if (PR_GetOpenFileInfo64(fp, &fInfo) != PR_SUCCESS) {
		JS_ReturnExceptionEscape("failed to read status of file handle for: %s", cmd, cmd);
	};

	len = fInfo.size + 1;

#else

	PRFileInfo fInfo;
	if (PR_GetOpenFileInfo(fp, &fInfo) != PR_SUCCESS) {
		JS_ReturnExceptionEscape("failed to read status of file handle for: %s", cmd, cmd);
	};

	len = fInfo.size + 1;

#endif

	buf = (char *) malloc(len * sizeof (char));
	if (!buf) {
		JS_ReturnExceptionEscape("failed to allocate input buffer for file handle of: %s", cmd, cmd);
	}

	PR_Read(fp, buf, len - 1);
	buf[len - 1] = 0; 
	JSString * contents = JS_NewStringCopyN(cx, (const char *) buf, len - 1);
	free(buf);
	if (!contents) {
		JS_ReturnExceptionEscape("failed to allocate javascript string for file handle of: %s", cmd, cmd);
	}

	JS_ReturnValue(STRING_TO_JSVAL(contents));

}

static JSBool ShellFileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
  char * filename = JS_ValueToNativeString(cx, argv[0]);
	if (!filename) {
		JS_ReturnException("failed to get file name from javascript", NULL);
	}

	JS_ReturnValueEscape(
		BOOLEAN_TO_JSVAL(
			PR_Access(filename, PR_ACCESS_EXISTS) == PR_SUCCESS
		), filename);

}

static JSBool ShellSystemWrite(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * cmd = JS_ValueToNativeString(cx, argv[0]);
	if (!cmd) {
		JS_ReturnException("failed to get command string from javascript", NULL);
	}

	char * content = JS_ValueToNativeString(cx, argv[1]);
	if (!content) {
		JS_ReturnExceptionEscape("failed to get content for command: %s", cmd, cmd);
	}

	FILE *fp;

	/* Open the command for reading. */
	fp = popen(cmd, "w");
	if (fp == NULL) {
		JS_ReturnExceptionEscape("failed to run: %s", cmd, cmd, content);
	}

	long contentLength = strlen(content);

	fwrite(content, 1, contentLength, fp);

	JS_ReturnValueEscape(INT_TO_JSVAL(pclose(fp)), cmd, content);

}

static JSBool ShellSystemRead(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * cmd = JS_ValueToNativeString(cx, argv[0]);

	if (!cmd) {
		JS_ReportError(cx, "failed to get string from javascript", NULL);
		return JS_FALSE;
	}

	FILE *fp;
	char path[1035];

	/* Open the command for reading. */
	fp = popen(cmd, "r");

	if (fp == NULL) {
		JS_ReturnExceptionEscape("failed to run command: %s", cmd, cmd);
	}

	JS_FreeNativeString(cx, cmd);

	StringBuffer io = NewStringBuffer();
	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof (path) - 1, fp) != NULL) StringBufferWrite(&io, path);

	int status = pclose(fp);

	JSString * out = JS_NewStringCopyN(cx, io.address, io.index);
	StringBufferRelease(&io);

	JSObject * result = JS_NewObject(cx, NULL, NULL, NULL);

	JS_DefineProperty(cx, result, "status", INT_TO_JSVAL(status), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, result, "output", STRING_TO_JSVAL(out), NULL, NULL, JSPROP_ENUMERATE);

	JS_ReturnValue(OBJECT_TO_JSVAL(result));

}

static JSBool ShellSystem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * cmd = JS_ValueToNativeString(cx, argv[0]);
	if (!cmd) {
		JS_ReturnException("failed to get command string from javascript", NULL);
	}

	int rc = system(cmd);

	JS_ReturnValueEscape(INT_TO_JSVAL(rc), cmd);
}

bool buffer_ends_with_newline(register char * buffer, int length) {
	if (buffer == NULL) return false;
	if (length == 0) length = strlen(buffer);
	if (length == 0) return false;
	register int check = length - 1;
	if (buffer[check] == 10) return true;
	if (buffer[check] == 0 && buffer[check - 1] == 10) return true;
	return false;
}

static JSBool ShellEchoError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	uintN i, argFinal = argc - 1;
	char *bytes;
	bool have_newline = false;

	for (i = 0; i < argc; i++) {
		bytes = JS_ValueToNativeString(cx, argv[i]);
		if (! bytes) continue;
		fprintf(stderr, "%s%s", i ? " " : "", bytes);
		if (i == argFinal) have_newline = buffer_ends_with_newline(bytes, 0);
		JS_FreeNativeString(cx, bytes);
	}

	if (!have_newline) putc('\n', stderr);
	fflush(stderr);

	JS_ReturnValue(JSVAL_VOID);
}

static JSBool ShellEcho(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

	uintN i, argFinal = argc - 1;
	char *bytes;
	bool have_newline = false;
	for (i = 0; i < argc; i++) {
		bytes = JS_ValueToNativeString(cx, argv[i]);
		if (! bytes) continue;
		printf("%s%s", i ? " " : "", bytes);
		if (i == argFinal) have_newline = buffer_ends_with_newline(bytes, 0);
		JS_FreeNativeString(cx, bytes);
	}

	if (!have_newline) putc('\n', stdout); 
	
	fflush(stdout);

	JS_ReturnValue(JSVAL_VOID);
}

static JSBool ShellPrint(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

	char *bytes; uintn i;
	for (i = 0; i < argc; i++) {
		bytes = JS_ValueToNativeString(cx, argv[i]);
		if (! bytes) continue;
		printf("%s", bytes);
		JS_FreeNativeString(cx, bytes);
	}
	
	fflush(stdout);

	JS_ReturnValue(JSVAL_VOID);
}

static JSBool ShellPrintError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	uintN i;
	char *bytes;
	for (i = 0; i < argc; i++) {
		bytes = JS_ValueToNativeString(cx, argv[i]);
		if (! bytes) continue;
		fprintf(stderr, "%s", bytes);
		JS_FreeNativeString(cx, bytes);
	}
	fflush(stdout);
	JS_ReturnValue(JSVAL_VOID);
}

static JSBool ShellExit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    int exitcode = JSVAL_TO_INT((argc > 0)?argv[0]:0);

#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
#endif

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();

    exit(exitcode);
    return JS_TRUE;

}

static JSFunctionSpec shell_functions[] = {
    JS_FS("Shell",      ShellSystem,           1, JSPROP_ENUMERATE,0),
    JS_FS("echo",         ShellEcho,           0, JSPROP_ENUMERATE,0),
    JS_FS("error",         ShellEchoError,     0, JSPROP_ENUMERATE,0),
    JS_FS("print",         ShellPrint,         0, JSPROP_ENUMERATE,0),
    JS_FS("printError",   ShellPrintError,     0, JSPROP_ENUMERATE,0),
    JS_FS("exit",         ShellExit,           0, JSPROP_ENUMERATE,0),
    JS_FS_END
};

JSBool M180_ShellInit(JSContext * cx, JSObject * global) {

	JS_DefineFunctions(cx, global, shell_functions);
	jsval fun;
	JS_GetProperty(cx, global, "Shell", &fun);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "source", ShellInclude, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "error", ShellEchoError, 0, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "echo", ShellEcho, 0, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "printError", ShellPrintError, 0, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "print", ShellPrint, 0, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "readPipe", ShellSystemRead, 1, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "writePipe", ShellSystemWrite, 2, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "get", ShellGet, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "set", ShellSet, 2, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "clear", ShellClear, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "keys", ShellKeys, 0, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "readLine", ShellReadline, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "readPassword", ShellReadPassword, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "fileExists", ShellFileExists, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "getFileProperties", ShellFileStat, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "readFile", ShellGetFileContent, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "writeFile", ShellSetFileContent, 2, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "joinFile", ShellPrintFile, 2, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "exit",              ShellExit, 0, JSPROP_ENUMERATE);

    return JS_TRUE;

}

int main(int argc, char **argv, char **envp) {

    int stackDummy;
    JSObject *glob;
    int result;

    setlocale(LC_ALL, "");

    gStackBase = (jsuword)&stackDummy;

    gErrFile = stderr;
    gOutFile = stdout;

    argc--;
    argv++;

    rt = JS_NewRuntime(64L * 1024L * 1024L);
    if (!rt)
        return 1;
    JS_SetContextCallback(rt, ContextCallback);

    cx = JS_NewContext(rt, gStackChunkSize);
    if (!cx)
        return 1;

#ifdef JS_THREADSAFE
    JS_BeginRequest(cx);
#endif

    glob = JS_NewObject(cx, &global_class, NULL, NULL);
    if (!glob)
        return 1;
#ifdef LAZY_STANDARD_CLASSES
    JS_SetGlobalObject(cx, glob);
#else
    if (!JS_InitStandardClasses(cx, glob))
        return 1;
#endif

    jsval rval = 0;
    JS_EvaluateScript(cx, JS_GetGlobalObject(cx), OBJECT_KEYS_PATCH_JS, OBJECT_KEYS_PATCH_JS_LENGTH, "spider://object-keys-patch.js", 1, &rval);

    M180_ShellInit(cx, glob);

    result = ProcessArgs(cx, glob, argv, argc);

#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
#endif

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return result;

}
