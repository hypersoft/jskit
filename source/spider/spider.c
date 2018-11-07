
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

#ifdef JSDEBUGGER
#include "jsdebug.h"
#include "jsdb.h"
#endif /* JSDEBUGGER */

#include <stdint.h>
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYSTEM_CPU_BITS 32
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYSTEM_CPU_BITS 64
#else
/* wtf */
#endif

#if SYSTEM_CPU_BITS == 64
#define PR_Seek PR_Seek64
#define PR_Available PR_Available64
#define PRFileInfo PRFileInfo64
#define PR_GetFileInfo PR_GetFileInfo64
#define PR_GetOpenFileInfo PR_GetOpenFileInfo64
#endif

#include "object-keys-patch.c"

JSRuntime *rt;
JSContext *cx;

#define POINTER_TO_JSVAL(CX, PTR) OBJECT_TO_JSVAL(JSNewPointer(cx, PTR))
#define JSVAL_TO_POINTER(CX, VAL) JS_GetPrivate(cx, JSVAL_TO_OBJECT(VAL))

void JS_FreeNativeStrings(JSContext * cx, ...) {
    va_list args;
    va_start(args, cx);
		void * f;
		while ((f = va_arg(args, void*))) JS_free(cx, f);
		va_end(args);
}

#define STRINGIZE(x) #x
#define M180_GetCompilerString(x) STRINGIZE(x)

void * JS_GarbagePointer(JSContext *, void *);
#define JS_ValueToNativeString(CX, JSVAL) ((char *) JS_GarbagePointer(CX, JS_EncodeString(CX, JS_ValueToString(CX, JSVAL))))

#define NATIVE_STRING_TO_JSVAL(CX, NSTR) STRING_TO_JSVAL(JS_NewStringCopyZ(CX, NSTR))

#define JS_ReturnValue(VAL) JS_SET_RVAL((JSContext *)cx, vp, VAL); return JS_TRUE
#define JS_ReturnValueWithGC(VAL) JS_SET_RVAL((JSContext *)cx, vp, VAL); JS_MaybeGC(cx); return JS_TRUE
#define JS_ReturnError() return JS_FALSE
#define JS_ReturnErrorWithGC() JS_MaybeGC(cx); return JS_FALSE

#define JS_ReturnException(FORMAT) JS_ReportError((JSContext *)cx, FORMAT, NULL); JS_ReturnErrorWithGC()
#define JS_ReturnCustomException(FORMAT, ...) JS_ReportError((JSContext *)cx, FORMAT, __VA_ARGS__); JS_ReturnErrorWithGC()

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pointer.c"

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
        fprintf(stderr, "GetStdHandle failed (error %iu)\n", GetLastError());
        return;
    }

    /* Get console mode */
    DWORD mode;
    if(!GetConsoleMode(hStdIn, &mode)) {
        fprintf(stderr, "GetConsoleMode failed (error %iu)\n", GetLastError());
        return;
    }

    if(display) {
        /* Add echo input to the mode */
        if(!SetConsoleMode(hStdIn, mode | ENABLE_ECHO_INPUT)) {
            fprintf(stderr, "SetConsoleMode failed (error %iu)\n", GetLastError());
            return;
        }
    }
    else {
        /* Remove echo input to the mode */
        if(!SetConsoleMode(hStdIn, mode & ~((DWORD) ENABLE_ECHO_INPUT))) {
            fprintf(stderr, "SetConsoleMode failed (error %iu)\n", GetLastError());
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

#ifdef JSD_LOWLEVEL_SOURCE
/*
 * This facilitates sending source to JSD (the debugger system) in the shell
 * where the source is loaded using the JSFILE hack in jsscan. The function
 * below is used as a callback for the jsdbgapi JS_SetSourceHandler hook.
 * A more normal embedding (e.g. mozilla) loads source itself and can send
 * source directly to JSD without using this hook scheme.
 */
static void
SendSourceToJSDebugger(const char *filename, uintN lineno,
                       jschar *str, size_t length,
                       void **listenerTSData, JSDContext* jsdc)
{
    JSDSourceText *jsdsrc = (JSDSourceText *) *listenerTSData;

    if (!jsdsrc) {
        if (!filename)
            filename = "typein";
        if (1 == lineno) {
            jsdsrc = JSD_NewSourceText(jsdc, filename);
        } else {
            jsdsrc = JSD_FindSourceForURL(jsdc, filename);
            if (jsdsrc && JSD_SOURCE_PARTIAL !=
                JSD_GetSourceStatus(jsdc, jsdsrc)) {
                jsdsrc = NULL;
            }
        }
    }
    if (jsdsrc) {
        jsdsrc = JSD_AppendUCSourceText(jsdc,jsdsrc, str, length,
                                        JSD_SOURCE_PARTIAL);
    }
    *listenerTSData = jsdsrc;
}
#endif /* JSD_LOWLEVEL_SOURCE */

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
        jsval jsv;
        jsv = STRING_TO_JSVAL(str);
        if (!JS_SetElement(cx, argsObj, i, &jsv)) {
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

#include "stringbuffer.c"

void JS_FreeNativeStringArray(JSContext * cx, char * b[], int c) {
    int i = 0;
    while (i < c) JS_free(cx, b[i++]);
}

#include "shell.c"

static JSBool
Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;
    uint32 oldopts;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
        errno = 0;
        oldopts = JS_GetOptions(cx);
        JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
        script = JS_CompileFile(cx, obj, filename);
        if (!script) {
            ok = JS_FALSE;
        } else {
            ok = !compileOnly
                 ? JS_ExecuteScript(cx, obj, script, &result)
                 : JS_TRUE;
            JS_DestroyScript(cx, script);
        }
        JS_SetOptions(cx, oldopts);
        if (!ok)
            return JS_FALSE;
    }

    return JS_TRUE;
}

int main(int argc, char **argv, char **envp) {

    int stackDummy;
    JSObject *glob;
    int result;

#ifdef JSDEBUGGER
    JSDContext *jsdc;
    JSBool jsdbc;
#endif /* JSDEBUGGER */

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

   // JS_DefineFunction(cx, glob, "load", Load, 0, 0);

#ifdef JSDEBUGGER
    /*
    * XXX A command line option to enable debugging (or not) would be good
    */
    jsdc = JSD_DebuggerOnForUser(rt, NULL, NULL);
    if (!jsdc)
        return 1;
    JSD_JSContextInUse(jsdc, cx);
#ifdef JSD_LOWLEVEL_SOURCE
    JS_SetSourceHandler(rt, (JSSourceHandler) SendSourceToJSDebugger, jsdc);
#endif /* JSD_LOWLEVEL_SOURCE */
    jsdbc = JSDB_InitDebugger(rt, jsdc, 0);
#endif /* JSDEBUGGER */

    jsval rval = 0;
    JS_EvaluateScript(cx, JS_GetGlobalObject(cx), OBJECT_KEYS_PATCH_JS, OBJECT_KEYS_PATCH_JS_LENGTH, "spider://object-keys-patch.js", 1, &rval);

    M180_ShellInit(cx, glob);

    result = ProcessArgs(cx, glob, argv, argc);

#ifdef JSDEBUGGER
    if (jsdc) {
        if (jsdbc)
            JSDB_TermDebugger(jsdc);
        JSD_DebuggerOff(jsdc);
    }
#endif  /* JSDEBUGGER */

#ifdef JS_THREADSAFE
    JS_EndRequest(cx);
#endif

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return result;

}

