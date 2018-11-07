typedef struct PtrClassData  {
    void *p;
    int size, length, bytes;
    struct {
        int garbage : 1;
        int allocated : 1;
        int readonly : 1;
        int vtboolean : 1;
        int vtfloat : 1;
        int vtdouble : 1;
        int vtsigned : 1;
        int vtutf : 1;
    } flags;
} PointerData;

PointerData NewPointerData(uintptr_t * p) {
    PointerData out;
    memset(&out, 0, sizeof(PointerData));
    out.p = p;
    return out;
}

JSBool PointerClassSetSize(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->size = JSVAL_TO_INT(*vp);
    pd->bytes = pd->length * pd->size;
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetSize(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(INT_TO_JSVAL(pd->size));
}

JSBool PointerClassSetLength(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->length = JSVAL_TO_INT(*vp);
    pd->bytes = pd->length * pd->size;
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetLength(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(INT_TO_JSVAL(pd->length));
}

JSBool PointerClassSetBytes(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    JS_ReturnException("trying to manually set byte length");
}

JSBool PointerClassGetBytes(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(INT_TO_JSVAL(pd->bytes));
}

JSBool PointerClassSetBoolean(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.vtboolean = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetBoolean(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.vtboolean));
}

JSBool PointerClassSetDouble(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.vtdouble = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetDouble(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.vtdouble));
}

JSBool PointerClassSetFloat(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.vtfloat = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetFloat(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.vtfloat));
}

JSBool PointerClassSetSigned(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.vtsigned = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetSigned(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.vtsigned));
}

JSBool PointerClassSetAllocated(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.allocated = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetAllocated(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.allocated));
}

JSBool PointerClassSetReadOnly(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.readonly = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetReadOnly(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.readonly));
}

JSBool PointerClassSetUtf(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    pd->flags.vtutf = JSVAL_TO_BOOLEAN(*vp);
    JS_ReturnValue(JS_TRUE);
}

JSBool PointerClassGetUtf(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);
    JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->flags.vtutf));
}

JSBool PointerClassSetPoint(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    PointerData * pd = JS_GetPrivate(cx, obj);

    if (JSVAL_IS_STRING(id)) {
        char * nid = JS_ValueToNativeString(cx, id);
        JS_ReturnCustomException("invalid property set request: %s", nid);
    }

    if (pd->p == 0) {
        JS_ReturnException("cannot write null pointer");
    } else if (pd->flags.readonly) {
        JS_ReturnException("cannot write data to read only pointer");
    }

    register long index = JSVAL_TO_INT(id);
    if (index >= pd->length) {
        JS_ReturnCustomException("buffer write overflow with position: %i; max: %i", index, pd->length - 1);
    }

    register double value = 0;

    if (JSVAL_IS_DOUBLE(*vp)) {
        value = *JSVAL_TO_DOUBLE(*vp);
    } else if (JSVAL_IS_BOOLEAN(*vp)) {
        value = JSVAL_TO_BOOLEAN(*vp);
    } else {
        value = JSVAL_TO_INT(*vp);
    }

    switch (pd->size) {
        case 1: { register char * x = pd->p; x[index] = value; break; }
        case 2: { register short * x = pd->p; x[index] = value; break; }
        case 4: {
            if (pd->flags.vtfloat) { register float32 * x = pd->p; x[index] = value; }
            else { register int32_t * x = pd->p; x[index] = value; }
            break;
        }
        case 8: {
            if (pd->flags.vtfloat) { register float64 * x = pd->p; x[index] = value; }
            else if (pd->flags.vtdouble) { register double * x = pd->p; x[index] = value; }
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
        JS_ReturnCustomException("invalid property get request: %s", nid);
    }

    if (pd->p == 0) { JS_ReturnException("cannot read null pointer"); }

    register long index = JSVAL_TO_INT(id);

    if (index >= pd->length) {
        JS_ReturnCustomException("buffer read overflow with position: %i; max: %i", index, pd->length - 1);
    }

    /* for */ jsval jsv; switch (pd->size) {
        case 1: {
            if (pd->flags.vtsigned) { register signed char * x = pd->p; jsv = DOUBLE_TO_JSVAL(x[index]); }
            else if (pd->flags.vtboolean) { register bool * x = pd->p; jsv = BOOLEAN_TO_JSVAL(x[index]); }
            else { register unsigned char * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            break;
        }
        case 2: {
            if (pd->flags.vtsigned) { register signed short * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            else { register unsigned short * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            break;
        }
        case 4: {
            if (pd->flags.vtfloat) { register float32 * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            else if (pd->flags.vtsigned) { register int32_t * x = pd->p; jsv = INT_TO_JSVAL(x[index]); }
            else { register uint32_t * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            break;
        }
        case 8: {
            if (pd->flags.vtfloat) { register float64 * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
            else if (pd->flags.vtdouble) { register double * x = pd->p; JS_NewNumberValue(cx, x[index], &jsv); }
            else if (pd->flags.vtsigned) { register int64_t * x = pd->p; JS_NewNumberValue(cx, (double) x[index], &jsv); }
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
        if (pd->p && pd->flags.allocated) { JS_free(cx, pd->p); }
        JS_free(cx, pd);
    }
}

JSBool PointerClassConvert(JSContext *cx, JSObject *obj, JSType type, jsval *vp) {

    PointerData * pd = JS_GetPrivate(cx, obj);

    if (!pd) {
        JS_ReturnException("couldn't get pointer header");
    }

    switch (type) {
        case JSTYPE_NUMBER: {
            jsval n = 0;
            JS_NewNumberValue(cx, (double) (uintptr_t) pd->p, &n);
            JS_ReturnValue(n);
            break;
        }
        case JSTYPE_BOOLEAN: {
            JS_ReturnValue(BOOLEAN_TO_JSVAL(pd->p != NULL));
            break;
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

static JSPropertySpec PointerProperties[] = {
    {"size",   -1,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetSize,    PointerClassSetSize},
    {"length", -2,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetLength,    PointerClassSetLength},
    {"bytes",  -3,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetBytes,    PointerClassSetBytes},
    {"boolean",  -4,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetBoolean,    PointerClassSetBoolean},
    {"double",  -5,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetDouble,    PointerClassSetDouble},
    {"float",  -6,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetFloat,    PointerClassSetFloat},
    {"signed",  -7,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetSigned,    PointerClassSetSigned},
    {"allocated",  -8,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetAllocated,    PointerClassSetAllocated},
    {"readOnly",  -9,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetReadOnly,    PointerClassSetReadOnly},
    {"utf",  -10,   JSPROP_SHARED | JSPROP_PERMANENT, PointerClassGetUtf,    PointerClassSetUtf},
    {0,0,0,0,0}
};

JSObject * JSNewPointer(JSContext * cx, void * p) {
    PointerData * pd = JS_malloc(cx, sizeof(PointerData));
    *pd = NewPointerData(p);
    JSObject * out = JS_NewObject(cx, &pointer_class, NULL, NULL);
    JS_SetPrivate(cx, out, pd);
    JS_DefineProperties(cx, out, &PointerProperties);
    return out;
}

JSObject * JSNewGarbagePointer(JSContext * cx, void * p) {
    PointerData * pd = JS_malloc(cx, sizeof(PointerData));
    *pd = NewPointerData(p);
    pd->flags.allocated = true;
    pd->flags.garbage = true;
    JSObject * out = JS_NewObject(cx, &pointer_class, NULL, NULL);
    JS_SetPrivate(cx, out, pd);
    return out;
}

void * JS_GarbagePointer(JSContext * cx, void * garbage) {
    if (garbage) JSNewGarbagePointer(cx, garbage);
    return garbage;
}