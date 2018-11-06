
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
        } else if (strcmp(nid, "isStruct") == 0) {
            pd->isStruct = JSVAL_TO_BOOLEAN(*vp);
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
        JS_ReturnCustomException("buffer write overflow with position: %i; max: %i", index, pd->length - 1);
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
        JS_ReturnCustomException("buffer read overflow with position: %i; max: %i", index, pd->length - 1);
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
        if (pd->p && pd->isAllocated) { JS_free(cx, pd->p); }
        JS_free(cx, pd);
    }
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