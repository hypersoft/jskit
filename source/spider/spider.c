
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

#define JS_ReturnValue(VAL) JS_SET_RVAL((JSContext *)cx, vp, VAL); JS_MaybeGC(cx); return JS_TRUE
#define JS_ReturnError() JS_MaybeGC(cx); return JS_FALSE

#define JS_ReturnException(FORMAT) JS_ReportError((JSContext *)cx, FORMAT, NULL); JS_ReturnError()
#define JS_ReturnCustomException(FORMAT, ...) JS_ReportError((JSContext *)cx, FORMAT, __VA_ARGS__); JS_ReturnError()

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct PtrClassData  {
    void *p;
    int size, length, bytes;
    bool isDouble, isFloat, isSigned, isAllocated, isReadOnly, isString, isPointer, isStruct;
} PointerData;

PointerData NewPointerData(uintptr_t * p) {
    return (PointerData) {p, 0, 0, 0, false, false, false, false, false, false, false, false};
}

JSBool PointerClassSetPoint(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    if (JSVAL_IS_STRING(id)) {
        char * nid = JS_ValueToNativeString(cx, id);
        if (strcmp(nid, "bytes") == 0) {
            JS_ReturnException("trying to manually set byte length");
        } else if (strcmp(nid, "size") == 0) {
            pd->size = JSVAL_TO_INT(*vp);
            pd->bytes = pd->length * pd->size;
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "length") == 0 ) {
            pd->length = JSVAL_TO_INT(*vp);
            pd->bytes = pd->length * pd->size;
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isDouble") == 0) {
            pd->isDouble = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isFloat") == 0) {
            pd->isFloat = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isSigned") == 0) {
            pd->isSigned = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isAllocated") == 0) {
            pd->isAllocated = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isReadOnly") == 0) {
            pd->isReadOnly = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isString") == 0) {
            pd->isString = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        } else if (strcmp(nid, "isPointer") == 0) {
            pd->isPointer = JSVAL_TO_BOOLEAN(*vp);
            JS_ReturnValue(JS_TRUE);
        }
        JS_ReturnCustomException("invalid property set request: %s", nid);
    }

    if (pd->p == 0) {
        JS_ReturnException("cannot write null pointer");
    } else if (pd->isReadOnly) {
        JS_ReturnException("cannot write data to read only pointer");
    }

    register long index = JSVAL_TO_INT(id);
    if (index >= pd->length) {
        JS_ReturnCustomException("buffer write overflow with position: %i; limit: %i", index, pd->length);
    }

    register double value = 0;

    if (JSVAL_IS_DOUBLE(*vp)) {
        value = *JSVAL_TO_DOUBLE(*vp);
    } else {
        value = JSVAL_TO_INT(*vp);
    }

    switch (pd->size) {
        case 1: { register char * x = pd->p; x[index] = value; break; }
        case 2: { register short * x = pd->p; x[index] = value; break; }
        case 4: {
            if (pd->isFloat) { register float32 * x = pd->p; x[index] = value; }
            else { register int32_t * x = pd->p; x[index] = value; }
            break;
        }
        case 8: {
            if (pd->isFloat) { register float64 * x = pd->p; x[index] = value; }
            else if (pd->isDouble) { register double * x = pd->p; x[index] = value; }
            else { register int64_t * x = pd->p; x[index] = value; }
            break;
        }
        default: { JS_ReturnCustomException("invalid pointer type size: %i", pd->size); }
    }
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetPoint(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    if (JSVAL_IS_STRING(id))  {
        char * nid = JS_ValueToNativeString(cx, id);
        if (strcmp(nid, "bytes") == 0) {
            jsval bytes = INT_TO_JSVAL(pd->bytes);
            JS_ReturnValue(bytes);
        } else if (strcmp(nid, "size") == 0) {
            jsval bytes = INT_TO_JSVAL(pd->size);
            JS_ReturnValue(bytes);
        } else if (strcmp(nid, "length") == 0 ) {
            jsval bytes = INT_TO_JSVAL(pd->length);
            JS_ReturnValue(bytes);
        } else if (strcmp(nid, "isDouble") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isDouble);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isFloat") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isFloat);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isSigned") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isSigned);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isAllocated") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isAllocated);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isReadOnly") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isReadOnly);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isString") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isString);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isPointer") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isPointer);
            JS_ReturnValue(boolean);
        } else if (strcmp(nid, "isStruct") == 0) {
            jsval boolean = BOOLEAN_TO_JSVAL(pd->isStruct);
            JS_ReturnValue(boolean);
        }
        JS_ReturnCustomException("invalid property get request: %s", nid);
    }

    if (pd->p == 0) { JS_ReturnException("cannot read null pointer"); }

    register long index = JSVAL_TO_INT(id);

    if (index >= pd->length) {
        JS_ReturnCustomException("buffer read overflow with position: %i; limit: %i", index, pd->length);
    }

    /* for */ jsval jsv; switch (pd->size) {
        case 1: {
            if (pd->isSigned) { register signed char * x = pd->p; jsv = DOUBLE_TO_JSVAL(x[index]); }
            else { register unsigned char * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            break;
        }
        case 2: {
            if (pd->isSigned) { register signed short * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            else { register unsigned short * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            break;
        }
        case 4: {
            if (pd->isFloat) { register float32 * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            else if (pd->isSigned) { register int32_t * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            else { register uint32_t * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            break;
        }
        case 8: {
            if (pd->isFloat) { register float64 * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            else if (pd->isDouble) { register double * x = pd->p; JS_NewNumberValue(cx, x[index], &jsv); }
            else if (pd->isSigned) { register int64_t * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            else { register uint64_t * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            break;
        }
        default: { JS_ReturnCustomException("invalid pointer type size: %i", pd->size); }
    }
    JS_ReturnValue(jsv);
}

void PointerClassFinalize(JSContext *cx, JSObject *obj) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    if (pd) {
        if (pd->p && pd->isAllocated) {
            JS_free(cx, pd->p);
        }
        JS_free(cx, pd);
    }
    //JS_SetPrivate(cx, obj, NULL);
}

#include <inttypes.h>

JSBool PointerClassConvert(JSContext *cx, JSObject *obj, JSType type, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    char b[64];

    if (!pd) {
        JS_ReturnException("couldn't get pointer header");
    }

    switch (type) {
        case JSTYPE_NUMBER: {
            JS_ReturnValue(DOUBLE_TO_JSVAL(pd->p));
            break;
        }
        case JSTYPE_BOOLEAN: {
            JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->p != NULL));
            break;
        }
        case JSTYPE_STRING: {
            if (pd->isString) {
                if (pd->size == 1) JS_ReturnValue(STRING_TO_JSVAL(JS_NewStringCopyN(cx, pd->p, pd->length)));
            }
            int i = sprintf(b, "0x%" PRIXPTR, pd->p);
            JS_ReturnValue(STRING_TO_JSVAL(JS_NewStringCopyN(cx, b, i)));
        }
        default: JS_ReturnValue(JS_FALSE);
    }
}

JSClass pointer_class = {
    "pointer", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,
    PointerClassGetPoint,  PointerClassSetPoint,
    JS_EnumerateStub, JS_ResolveStub,
    PointerClassConvert,   PointerClassFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject * JSNewPointer(JSContext * cx, void * p) {
    PointerData * pd = JS_malloc(cx, sizeof(PointerData));
    *pd = NewPointerData(p);
    JSObject * out = JS_NewObject(cx, &pointer_class, NULL, NULL);
    JS_SetPrivate(cx, out, pd);
    return out;
}

void * JS_GarbagePointer(JSContext * cx, void * garbage) {
    if (garbage) {
        JSObject * ptr = JSNewPointer(cx, garbage);
        PointerData * pd = JS_GetPrivate(cx, ptr);
        pd->isAllocated = true;
    }
    return garbage;
}

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

static JSBool ShellInclude(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
    if (argc == 0) {
        return JS_FALSE;
    }

	JSBool status;
    char * string[1];
    string[0] = JS_ValueToNativeString(cx, argv[0]);
	status = ExecScriptFile(cx, JS_GetGlobalObject(cx), string[0], vp);
    JS_ReturnValue(status);

}

static JSBool ShellReadline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    char * string[2];
    jsval out;
    string[0] = JS_ValueToNativeString(cx, argv[0]);
    string[1] = readline(string[0]);

    if (!string[1]) {
        JS_ReturnError();
    }

    JS_GarbagePointer(cx, string[1]);

    if (string[1][0] != '\0')
        add_history(string[1]);
    out = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, string[1]));

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
        JS_ReturnError();
    }

    JS_GarbagePointer(cx, string[1]);
    out = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, string[1]));

    JS_ReturnValue(out);

}

static JSBool ShellClear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    char * key = JS_ValueToNativeString(cx, argv[0]);
	
    if (!key) {
		JS_ReturnException("failed to get environment variable key from javascript");
	}

	unsetenv(key);

	JS_ReturnValue(JSVAL_TRUE);

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
        jsval jsv;
        jsv = STRING_TO_JSVAL(str);
		JS_SetElement(cx, argsObj, i++, &jsv);
	}
	JS_ReturnValue(OBJECT_TO_JSVAL(argsObj));
}

static JSBool ShellGet(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
  char * key = JS_ValueToNativeString(cx, argv[0]);
	
	if (!key) {
		JS_ReturnException("failed to get environment variable key from javascript");
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
		JS_ReturnException("failed to get environment variable key from javascript");
	}
  
	char * contents = JS_ValueToNativeString(cx, argv[1]);
	
	if (!contents) {
		JS_ReturnCustomException("failed to get data for environment variable: %s; from javascript", filename);
	}

	int overwrite = (argc > 2) ? JSVAL_TO_INT(argv[2]) : 1;

	int r = setenv(filename, contents, overwrite);

	JS_ReturnValue(BOOLEAN_TO_JSVAL(r == 0));

}

static JSBool ShellFileStat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * filename = JS_ValueToNativeString(cx, argv[0]);

	if (!filename) {
		JS_ReturnException("can't convert file name to string");
	}

	JSObject * out = JS_NewObject(cx, NULL, NULL, NULL);

	PRFileInfo fInfo;
	if (PR_GetFileInfo(filename, &fInfo) != PR_SUCCESS) {
		JS_ReturnCustomException("failed to read file stats of: %s", filename);
	};

	JS_DefineProperty(cx, out, "type", INT_TO_JSVAL(fInfo.type), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "creationTime", INT_TO_JSVAL(fInfo.creationTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "modificationTime", INT_TO_JSVAL(fInfo.modifyTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "size", INT_TO_JSVAL(fInfo.size), NULL, NULL, JSPROP_ENUMERATE);
	
	JS_DefineProperty(cx, out, "writable", BOOLEAN_TO_JSVAL(PR_Access(filename, PR_ACCESS_WRITE_OK) == PR_SUCCESS), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "readable", BOOLEAN_TO_JSVAL(PR_Access(filename, PR_ACCESS_READ_OK) == PR_SUCCESS), NULL, NULL, JSPROP_ENUMERATE);

	JS_ReturnValue(OBJECT_TO_JSVAL(out));

}

static JSBool ShellSetFileContent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * filename = JS_ValueToNativeString(cx, argv[0]);
	if (!filename) {
		JS_ReturnException("failed to get file name from javascript");
	}
	// todo: abort if file is a directory

	char * contents = JS_ValueToNativeString(cx, argv[1]);

	FILE * file = fopen(filename, "w");
	if (!file) {
		JS_ReturnCustomException("failed to open file: %s; for writing", filename);
	}

	long contentLength = strlen(contents);

	fwrite(contents, 1, contentLength, file);
	fclose(file);

	JS_ReturnValue(DOUBLE_TO_JSVAL(contentLength));

}

static JSBool ShellPrintFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * filename = JS_ValueToNativeString(cx, argv[0]);
	if (!filename) {
		JS_ReturnException("failed to get file name from javascript");
	}
	// todo: abort if file is a directory

	char * contents = JS_ValueToNativeString(cx, argv[1]);

	FILE * file = fopen(filename, "a");
	if (!file) {
		JS_ReturnCustomException("failed to open file: %s; for appending", filename);
	}

	long contentLength = strlen(contents);

	fwrite(contents, 1, contentLength, file);
	fclose(file);

	JS_ReturnValue(DOUBLE_TO_JSVAL(contentLength));
}

static JSBool ShellGetFileContent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

	char * cmd = JS_ValueToNativeString(cx, argv[0]);
	if (!cmd) {
		JS_ReturnException("failed to get file name from javascript");
	}

	PRFileDesc * fp = PR_Open(cmd, PR_RDONLY, 0);
	if (fp == NULL) {
        JS_ReturnCustomException("failed to open file: %s", cmd);
	}

	char *buf = NULL;
	size_t len = 0;

	PRFileInfo fInfo;
	if (PR_GetOpenFileInfo(fp, &fInfo) != PR_SUCCESS) {
		JS_ReturnCustomException("failed to read status of file handle for: %s", cmd);
	};

	len = fInfo.size + 1;

	buf = (char *) malloc(len * sizeof (char));
	if (!buf) {
		JS_ReturnCustomException("failed to allocate input buffer for file handle of: %s", cmd);
	}

	PR_Read(fp, buf, len - 1);
	buf[len - 1] = 0; 
	JSString * contents = JS_NewStringCopyN(cx, (const char *) buf, len - 1);
	free(buf);
	if (!contents) {
		JS_ReturnCustomException("failed to allocate javascript string for file handle of: %s", cmd);
	}

	JS_ReturnValue(STRING_TO_JSVAL(contents));

}

static JSBool ShellFileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
  char * filename = JS_ValueToNativeString(cx, argv[0]);
	if (!filename) {
		JS_ReturnException("failed to get file name from javascript");
	}

	JS_ReturnValue(BOOLEAN_TO_JSVAL(
        PR_Access(filename, PR_ACCESS_EXISTS) == PR_SUCCESS
    ));

}

static JSBool ShellSystemWrite(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * cmd = JS_ValueToNativeString(cx, argv[0]);

	if (!cmd) {
		JS_ReturnException("failed to get command string from javascript");
	}

	char * content = JS_ValueToNativeString(cx, argv[1]);

	if (!content) {
		JS_ReturnCustomException("failed to get content for command: %s", cmd);
	}

	FILE *fp;

	/* Open the command for reading. */
	fp = popen(cmd, "w");
	if (fp == NULL) {
		JS_ReturnCustomException("failed to run: %s", cmd);
	}

	long contentLength = strlen(content);

	fwrite(content, 1, contentLength, fp);

	JS_ReturnValue(INT_TO_JSVAL(pclose(fp)));

}

static JSBool ShellSystemRead(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	char * cmd = JS_ValueToNativeString(cx, argv[0]);

	if (!cmd) {
		JS_ReturnException("failed to get string from javascript");
	}

	FILE *fp;
	char path[1035];

	/* Open the command for reading. */
	fp = popen(cmd, "r");

	if (fp == NULL) {
		JS_ReturnCustomException("failed to run command: %s", cmd);
	}

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
    if (argc != 1) {
		JS_ReturnException("this procedure requires one parameter: system command string");
    }

	char * cmd = JS_ValueToNativeString(cx, argv[0]);

	if (!cmd) {
		JS_ReturnException("failed to get command string from javascript");
	}

	int rc = system(cmd);

	JS_ReturnValue(INT_TO_JSVAL(rc));
}

static JSBool ShellFDProperties(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

	PRFileDesc * filename = JSVAL_TO_POINTER(cx, argv[0]);

	if (!filename) {
		JS_ReturnException("can't convert file name to string");
	}

	JSObject * out = JS_NewObject(cx, NULL, NULL, NULL);

	PRFileInfo fInfo;
	if (PR_GetOpenFileInfo(filename, &fInfo) != PR_SUCCESS) {
		JS_ReturnCustomException("failed to read file stats of: %s", filename);
	};

	JS_DefineProperty(cx, out, "type", INT_TO_JSVAL(fInfo.type), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "creationTime", INT_TO_JSVAL(fInfo.creationTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "modificationTime", INT_TO_JSVAL(fInfo.modifyTime), NULL, NULL, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, out, "size", INT_TO_JSVAL(fInfo.size), NULL, NULL, JSPROP_ENUMERATE);

	JS_ReturnValue(OBJECT_TO_JSVAL(out));

}

static JSBool ShellFDType(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (argc != 1) {
		JS_ReturnException("this procedure requires one parameter: FILEDESC");
    }

    PointerData * pd = JSVAL_TO_POINTER(cx, argv[0]);
    PRFileDesc * pfd = pd->p;
    int r = PR_GetDescType(pfd);

    JS_ReturnValue(INT_TO_JSVAL(r));

}

// TODO: return an allocated buffer pointer
static JSBool ShellFDPipe(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
    JSObject * arrayObj = JS_NewArrayObject(cx, 2, NULL);
    PRFileDesc * in[2];
    int r = PR_CreatePipe(&in[0], &in[1]);
    jsval jsv1, jsv2;
    jsv1 = POINTER_TO_JSVAL(cx, in[0]);
    jsv2 = POINTER_TO_JSVAL(cx, in[1]);
    JS_SetElement(cx, arrayObj, 0, &jsv1);
    JS_SetElement(cx, arrayObj, 1, &jsv2);
    JS_ReturnValue(OBJECT_TO_JSVAL(arrayObj));
}

static JSBool ShellFDSeek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
    /*
        2 args: fd, absolute-position
        3 args: fd, destination, boolean forward
    */

    if (argc != 3 && argc != 2) {
		JS_ReturnException("this procedure requires three parameters: FILEDESC, COUNT[, BOOLEAN]");
    }

    PointerData * pd = JSVAL_TO_POINTER(cx, argv[0]);
    PRFileDesc * pfd = pd->p;
    int r, count = JSVAL_TO_INT(argv[1]);

    if (argc == 2) {
        r = PR_Seek(pfd, count, PR_SEEK_SET);
    }

    bool forward = JSVAL_TO_BOOLEAN(argv[2]);

    if (forward) {
        r = PR_Seek(pfd, count, PR_SEEK_CUR);
    } else {
        r = PR_Seek(pfd, count, PR_SEEK_END);
    }

    JS_ReturnValue(INT_TO_JSVAL(r));

}

static JSBool ShellFDWriteBytes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
    if (argc != 3) {
		JS_ReturnException("this procedure requires three parameters: FILEDESC, ARRAY, COUNT");
    }
    PointerData * pd = JSVAL_TO_POINTER(cx, argv[0]);
    PRFileDesc * pfd = pd->p;
    PointerData * buffer = JSVAL_TO_POINTER(cx, argv[1]);
    int count = JSVAL_TO_INT(argv[2]);
    int r = PR_Write(pfd, buffer->p, count);
    JS_ReturnValue(INT_TO_JSVAL(r));
}

static JSBool ShellFDReadBytes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (argc != 3) {
		JS_ReturnException("this procedure requires three parameters: FILEDESC, BUFFER, COUNT");
    }

    PointerData * pd = JSVAL_TO_POINTER(cx, argv[0]);
    PRFileDesc * pfd = pd->p;

    PointerData * buffer = JSVAL_TO_POINTER(cx, argv[1]);
    int count = JSVAL_TO_INT(argv[2]);

    int r = PR_Read(pfd, buffer->p, count);

    JS_ReturnValue(INT_TO_JSVAL(r));

}

static JSBool ShellFDBytes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (argc != 1) {
		JS_ReturnException("this procedure requires one parameter: FILEDESC");
    }

    PRFileDesc * pfd = JSVAL_TO_POINTER(cx, argv[0]);

      int r = PR_Available(pfd);

    JS_ReturnValue(INT_TO_JSVAL(r));

}

static JSBool ShellFDSync(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (argc != 1) {
		JS_ReturnException("this procedure requires one parameter: FILEDESC");
    }

    PRFileDesc * pfd = JSVAL_TO_POINTER(cx, argv[0]);
    int r = PR_Sync(pfd);

    JS_ReturnValue(BOOLEAN_TO_JSVAL(r == PR_SUCCESS));

}

static JSBool ShellFDClose(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (argc != 1) {
		JS_ReturnException("this procedure requires one parameter: FILEDESC");
    }

    PRFileDesc * pfd = JSVAL_TO_POINTER(cx, argv[0]);
    int r = PR_Close(pfd);

    JS_ReturnValue(BOOLEAN_TO_JSVAL(r == PR_SUCCESS));

}

static JSBool ShellFDOpenFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
    if (argc < 2) {
		JS_ReturnException("this procedure requires two parameters: PATH, SPEC; where SPEC: r[ead]w[rite]c[reate]");
    }

    int DEFAULT_MODE_FLAGS = PR_IRUSR | PR_IWUSR | PR_IWGRP | PR_IRGRP | PR_IROTH;
    // Number Shell.fd.openFile(PATH, SPEC)
    /*
        Where SPEC: [r][w][c]
        where r = read
        where w = write
        where c = create
    */
    char * string[2];

	string[0] = JS_ValueToNativeString(cx, argv[0]);
    string[1] = JS_ValueToNativeString(cx, argv[1]);

    int flags = 0;

	if (!string[0]) {
		JS_ReturnException("failed to get file name from javascript");
	}

	if (!string[1]) {
        JS_ReturnException("failed to get file access flags from javascript");
	}

    int i = 0, r = 0, w = 0, c = 0, a = 0;
    while ((c = string[1][i++])) {
        switch (c) {
            case 'r': r = 1; 
            break;
            case 'w': w = 1;
            break;
            case 'c': c = 1;
            break;
            case 'a': a = 1;
            default:
            JS_ReturnCustomException("wrong file access request in character: %c; expected r, w, a, and or c; in any order", c);
        }
    }

    if (c) flags = PR_CREATE_FILE;
    if (r && (w || a)) flags |= PR_RDWR;
    else {
        if (r) flags |= PR_RDONLY;
        if (w) flags |= PR_WRONLY;
        if (a) flags |= PR_APPEND;
    }

    void * p = PR_Open(string[0], flags, 
        (c) ? // create mode, yes so use the int value int param 3 if it exists
        JSVAL_TO_INT((argc == 3)?argv[2]:DEFAULT_MODE_FLAGS):
        DEFAULT_MODE_FLAGS
    );

    JSObject * ptr = JSNewPointer(cx, p);
    PointerData * pd = JS_GetPrivate(cx, ptr);
    pd->size = 1; pd->length = sizeof(PRFileDesc);
    pd->isStruct = true;

	JS_ReturnValue(OBJECT_TO_JSVAL(ptr));

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
		bytes = JS_EncodeString(cx, JS_ValueToString(cx, argv[i]));
		if (! bytes) continue;
		fprintf(stderr, "%s%s", i ? " " : "", bytes);
		if (i == argFinal) have_newline = buffer_ends_with_newline(bytes, 0);
		JS_free(cx, bytes);
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
		bytes = JS_EncodeString(cx, JS_ValueToString(cx, argv[i]));
		if (! bytes) continue;
		printf("%s%s", i ? " " : "", bytes);
		if (i == argFinal) have_newline = buffer_ends_with_newline(bytes, 0);
		JS_free(cx, bytes);
	}

	if (!have_newline) putc('\n', stdout); 
	
	fflush(stdout);

	JS_ReturnValue(JSVAL_VOID);
}

static JSBool ShellPrint(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

	char *bytes; uintn i;
	for (i = 0; i < argc; i++) {
		bytes = JS_EncodeString(cx, JS_ValueToString(cx, argv[i]));
		if (! bytes) continue;
		printf("%s", bytes);
		JS_free(cx, bytes);
	}
	
	fflush(stdout);

	JS_ReturnValue(JSVAL_VOID);
}

static JSBool ShellPrintError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
	uintN i;
	char *bytes;
	for (i = 0; i < argc; i++) {
		bytes = JS_EncodeString(cx, JS_ValueToString(cx, argv[i]));
		if (! bytes) continue;
		fprintf(stderr, "%s", bytes);
		JS_free(cx, bytes);
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

static JSBool ShellBuffer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (!argc) {
        JS_ReturnCustomException("failed to allocate %i bytes for buffer", 0);
    }

    if (argc != 2) {
        JS_ReturnCustomException("expected 2 parameters: SIZE, LENGTH; have: %i", argc);
    }

    long size = JSVAL_TO_INT(argv[0]);
    long count = JSVAL_TO_INT(argv[1]);
    long bytes = size * count;
    void * buffer = JS_malloc(cx, bytes);

    if (!buffer) {
        JS_ReturnCustomException("failed to allocate %i bytes for buffer", bytes);
    }

    JSObject * ptr = JSNewPointer(cx, buffer);
    PointerData * pd = JS_GetPrivate(cx, ptr);
    pd->size = size;
    pd->length = count;
    pd->bytes = bytes;
    
    JS_ReturnValue(OBJECT_TO_JSVAL(ptr));

}

static JSBool ShellBufferResize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (!argc) {
        JS_ReturnCustomException("failed to allocate %i bytes for buffer", 0);
    }

    if (argc != 3) {
        JS_ReturnCustomException("expected 3 parameters: BUFFER, SIZE, LENGTH; have: %i", argc);
    }

    PointerData * pd = JSVAL_TO_POINTER(cx, argv[0]);

    void * buffer = pd->p;
    long size = JSVAL_TO_INT(argv[1]);
    long count = JSVAL_TO_INT(argv[2]);
    long bytes = size * count;
    buffer = JS_realloc(cx, buffer, bytes);

    if (!buffer) {
        JS_free(cx, pd->p); pd->p = NULL;
        JS_ReturnCustomException("failed to change buffer span to %i", bytes);
    }

    pd->p = buffer;
    pd->size = size;
    pd->length = count;
    pd->bytes = bytes;
    
    JS_ReturnValue(argv[0]);

}

/*
static JSBool ShellBufferPaste(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{
    jsval jsv;
    JSObject * oBufferWrite = JSVAL_TO_OBJECT(argv[0]);
    JS_GetProperty(cx, oBufferWrite, "size", &jsv);
    long pBufferSize = JSVAL_TO_INT(jsv);

    JS_GetProperty(cx, oBufferWrite, "length", &jsv);
    long pBufferLength = JSVAL_TO_INT(jsv);

    long pBufferBytes = pBufferSize * pBufferLength;

    long start = JSVAL_TO_INT(argv[1]);
    long length = JSVAL_TO_INT(argv[2]);
    
    JSObject * oBufferSource = JSVAL_TO_OBJECT(argv[0]);

    if (JS_IsArrayObject(cx, oBufferSource)) {
        uint i = 0, m = 0;
        JS_GetArrayLength(cx, oBufferSource, &m);
        if (length > m) length = m;
        if (length > pBufferLength - start) length = pBufferLength - start;
        switch (pBufferSize) {
            case 1: {
                char * buffer = JS_GetPrivate(cx, oBufferWrite);
                for (; start < length; start++) {
                    JS_GetElement(cx, oBufferSource, i++, &jsv);
                    buffer[start] = JSVAL_TO_INT(jsv);
                }
                break;
            }
            case 2: {
                short * buffer = JS_GetPrivate(cx, oBufferWrite);
                for (; start < length; start++) {
                    JS_GetElement(cx, oBufferSource, i++, &jsv);
                    buffer[start] = JSVAL_TO_INT(jsv);
                }
                break;
            }
            case 4: {
                bool isFloat = false;
                JS_GetProperty(cx, oBufferWrite, "float32", &jsv);
                isFloat = JSVAL_TO_BOOLEAN(jsv);
                if (isFloat) {
                    float32 * buffer = JS_GetPrivate(cx, oBufferWrite);
                    for (; start < length; start++) {
                        JS_GetElement(cx, oBufferSource, i++, &jsv);
                        buffer[start] = *JSVAL_TO_DOUBLE(jsv);
                    }
                } else {
                    int32_t * buffer = JS_GetPrivate(cx, oBufferWrite);
                    for (; start < length; start++) {
                        JS_GetElement(cx, oBufferSource, i++, &jsv);
                        buffer[start] = JSVAL_TO_INT(jsv);
                    }
                }
                break;
            }
            case 8: {
                bool isDouble = false;
                JS_GetProperty(cx, oBufferWrite, "double", &jsv);
                isDouble = JSVAL_TO_BOOLEAN(jsv);
                bool isFloat = false;
                JS_GetProperty(cx, oBufferWrite, "float64", &jsv);
                isFloat = JSVAL_TO_BOOLEAN(jsv);
                if (isDouble) {
                    double * buffer = JS_GetPrivate(cx, oBufferWrite);
                    for (; start < length; start++) {
                        JS_GetElement(cx, oBufferSource, i++, &jsv);
                        buffer[start] = *JSVAL_TO_DOUBLE(jsv);
                    }
                } else if (isFloat) {
                    float64 * buffer = JS_GetPrivate(cx, oBufferWrite);
                    for (; start < length; start++) {
                        JS_GetElement(cx, oBufferSource, i++, &jsv);
                        buffer[start] = *JSVAL_TO_DOUBLE(jsv);
                    }
                }
                else {
                    int64_t * buffer = JS_GetPrivate(cx, oBufferWrite);
                    for (; start < length; start++) {
                        JS_GetElement(cx, oBufferSource, i++, &jsv);
                        buffer[start] = *JSVAL_TO_DOUBLE(jsv);
                    }
                }
                break;
            }
            default: {
                JS_ReturnException("invalid type size: %i", NULL, pBufferSize);
            }
        }
    } else {
        void * source = JS_GetPrivate(cx, oBufferSource);
        long sLength = 0;
        JS_GetProperty(cx, oBufferSource, "length", &jsv);
        sLength = *JSVAL_TO_DOUBLE(jsv);
        if (length > sLength) length = sLength;
        if (length > pBufferLength - start) length = pBufferLength - start;
        long bytes = length * pBufferSize;
        memcpy(JS_GetPrivate(cx, oBufferWrite) + (start * pBufferSize), source, bytes);
    }

    JS_ReturnValue(argv[2]);

}
*/

/*
static JSBool ShellBufferCut(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (!argc) {
        JS_ReturnException("failed to cut %i bytes for from no buffer", 0);
    }

    if (argc != 3) {
        JS_ReturnException("expected 3 parameters: BUFFER, START, LENGTH; have: %i", argc);
    }

    JSObject * oBuffer = JSVAL_TO_OBJECT(argv[0]);
    jsval jsv = JS_FALSE;
    void * pBuffer = JSVAL_TO_POINTER(cx, argv[0]);
 
    JS_GetProperty(cx, oBuffer, "size", &jsv);
    long pBufferSize = JSVAL_TO_INT(jsv);

    JS_GetProperty(cx, oBuffer, "length", &jsv);
    long pBufferLength = JSVAL_TO_INT(jsv);

    long pBufferBytes = pBufferSize * pBufferLength;

    long start = JSVAL_TO_INT(argv[1]);
    long count = JSVAL_TO_INT(argv[2]);
    long bytes = pBufferSize * count;

    void * destination = JS_malloc(cx, bytes);

    if (!destination) {
        JS_ReturnException("failed to cut %i bytes to buffer", bytes);
    }

    memcpy(destination, pBuffer + (start * pBufferSize), bytes);

    JSObject * ptr = JSNewPointer(cx, destination);

    JS_DefineProperty(cx, ptr, "size", pBufferSize, NULL, NULL, JSPROP_ENUMERATE);
    JS_DefineProperty(cx, ptr, "length", argv[2], NULL, NULL, JSPROP_ENUMERATE);
    JS_DefineProperty(cx, ptr, "bytes", INT_TO_JSVAL(bytes), NULL, NULL, JSPROP_ENUMERATE);
    
    JS_ReturnValue(OBJECT_TO_JSVAL(ptr));

}
*/

static JSBool ShellBufferFree(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (!argc) {
        JS_ReturnException("failed to free no buffers");
    }

    int i = 0;

    while (i < argc) {
        PointerData * pd = JSVAL_TO_POINTER(cx, argv[i++]);
        if (pd == NULL) {
            JS_ReturnException("failed to get pointer header");
        } else if (pd->p == NULL) {
            JS_ReturnException("failed to free null pointer");
        } else if (pd->isAllocated == false) {
            char * pStr = JS_ValueToNativeString(cx, OBJECT_TO_JSVAL(obj));
            JS_ReturnCustomException("failed fo free foreign pointer: %s", pStr);
        }
        JS_free(cx, pd->p);
        // erase it
        memset(pd, 0, sizeof(PointerData));
    }

    JS_ReturnValue(JS_TRUE);

}

static JSBool ShellBufferClear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *vp)
{

    if (!argc) {
        JS_ReturnException("failed to clear no buffers");
    }

    int i = 0;
    
    while (i < argc) {
        JSObject * pObj = JSVAL_TO_OBJECT(argv[i++]);
        PointerData * pd = JS_GetPrivate(cx, pObj);
        if (!pd) {
            JS_ReturnException("failed to get pointer header");
        } else
        if (!pd->p) {
            JS_ReturnException("cannot write to null pointer");
        } else if (pd->isReadOnly) {
            JS_ReturnException("cannot write to read only pointer");
        }
        memset(pd->p, 0, pd->bytes);
    }

    JS_ReturnValue(JS_TRUE);

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

JSBool ShellFDGetStandard(JSContext *cx, JSObject *obj, jsval id,
                                 jsval *vp) {

    switch (JSVAL_TO_INT(id)) {
        case 0: JS_ReturnValue(POINTER_TO_JSVAL(cx, (PR_STDIN)));
        case 1: JS_ReturnValue(POINTER_TO_JSVAL(cx, (PR_STDOUT)));
        case 2: JS_ReturnValue(POINTER_TO_JSVAL(cx, (PR_STDERR)));
        default: JS_ReturnValue(JS_FALSE);
    }

}

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
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "exit", ShellExit, 0, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, JSVAL_TO_OBJECT(fun), "buffer", ShellBuffer, 2, JSPROP_ENUMERATE);

    jsval bufferFunction;
    JS_GetProperty(cx, JSVAL_TO_OBJECT(fun), "buffer", &bufferFunction);
    JSObject * bFunc = JSVAL_TO_OBJECT(bufferFunction);
    JS_DefineFunction(cx, bFunc, "free", ShellBufferFree, 0, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, bFunc, "span", ShellBufferResize, 3, JSPROP_ENUMERATE); // realloc
//    JS_DefineFunction(cx, bFunc, "slice", ShellBufferFree, 3, JSPROP_ENUMERATE); // convert to js
//    JS_DefineFunction(cx, bFunc, "cut",   ShellBufferCut, 3, JSPROP_ENUMERATE);   // dup
//    JS_DefineFunction(cx, bFunc, "paste", ShellBufferPaste, 3, JSPROP_ENUMERATE); // convert to native or use native and write
    JS_DefineFunction(cx, bFunc, "clear", ShellBufferClear, 2, JSPROP_ENUMERATE); // value, erase...

    JSObject * fd = JS_NewObject(cx, NULL, NULL, NULL);
    JS_DefineProperty(cx, JSVAL_TO_OBJECT(fun), "fd", OBJECT_TO_JSVAL(fd), NULL, NULL, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "openFile", ShellFDOpenFile, 2, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "sync", ShellFDSync, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "close", ShellFDClose, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "type", ShellFDType, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "properties", ShellFDProperties, 1, JSPROP_ENUMERATE);
    JS_DefineElement(cx, fd, 0, 0, ShellFDGetStandard, NULL, JSPROP_ENUMERATE);
    JS_DefineElement(cx, fd, 1, 0, ShellFDGetStandard, NULL, JSPROP_ENUMERATE);
    JS_DefineElement(cx, fd, 2, 0, ShellFDGetStandard, NULL, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "bytes", ShellFDBytes, 1, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "read", ShellFDReadBytes, 3, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "write", ShellFDWriteBytes, 3, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "seek", ShellFDSeek, 2, JSPROP_ENUMERATE);
    JS_DefineFunction(cx, fd, "pipe", ShellFDPipe, 0, JSPROP_ENUMERATE);

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
