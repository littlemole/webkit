
#include "pyjsGlue.h"


/*
 * custom javascript object to add a global Python object to the Javascript namespace.
 * 
 */



static bool Python_hasPropertyCallback (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    jstr str(propertyName);
    std::string s(str.str());
    if ( s == "toString" || s == "valueOf"|| s == "toJSON") 
    {
       return false;
    }
    return true;
}


/*
 * called when javascript accesses the global Python objects' 
 * members directly from the global Python object as in 
 *
 *   Python.myMember
 * 
 * this defaults to look into namespace __main__ and call getattr
 * on it.
 */

static JSValueRef Python_getPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    jstr str(propertyName);

    PyGlobalInterpreterLock pyGuard;

    PyObject*   module = PyImport_AddModule("__main__");
    PyObjectRef name   = PyUnicode_FromString(str.str());
    PyObjectRef obj    = PyObject_GetAttr(module,name);

    return PJSMarshal::make_js_value(context,obj);
}

/*
 * called when javascript accesses the global Python object
 * as a function as in
 *
 *     Python('namespace')
 * 
 * this looks into namespace 'namespace' and calls getattr
 * on it.
 */
static JSValueRef Python_callAsFunctionCallback(
                JSContextRef context, 
                JSObjectRef function, 
                JSObjectRef thisObject, 
                size_t argumentCount, 
                const JSValueRef arguments[], 
                JSValueRef* exception)
{
    if ( argumentCount < 1 ) {
        return JSValueMakeUndefined(context);
    }

    jstr import(context,arguments[0]);

    PyGlobalInterpreterLock pyGuard;

    PyObject* module = PyImport_AddModule(import.str());

    jclass clazz(&python_object_class_def);
    return clazz.create(context,module);
}


static const JSClassDefinition Python_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "Python",              // className
    NULL,                  // parentClass
    NULL,                  // staticValues
    NULL,                  // staticFunctions
    NULL,                  // initialize
    NULL,                  // finalize
    Python_hasPropertyCallback,   // hasProperty
    Python_getPropertyCallback,   // getProperty
    NULL,                  // setProperty
    NULL,                  // deleteProperty
    NULL,                  // getPropertyNames
    Python_callAsFunctionCallback, // callAsFunction
    NULL,                  // callAsConstructor
    NULL,                  // hasInstance  
    NULL                   // convertToType
};


extern "C" void set_pywebkit_python_global(gpointer ctx, gpointer user_data)
{
    JSContextRef context = (JSContextRef)ctx;

    JSObjectSetProperty(
        context, 
        JSContextGetGlobalObject(context), 
        jstr("Python").ref(), 
        jclass(&Python_class_def).create(context,user_data), 
        kJSPropertyAttributeNone, 
        NULL
    );
}

