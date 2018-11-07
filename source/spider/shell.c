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
        } else if (pd->flags.allocated == false) {
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
        } else if (pd->flags.readonly) {
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