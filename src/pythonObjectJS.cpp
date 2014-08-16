
#include "pyjsGlue.h"

/*
 * custom javascript object to expose Python objects to Javascript.
 *
 */

static inline PyObject* my_py_obj(JSObjectRef object)
{
    return (PyObject*)JSObjectGetPrivate(object);
}

static void python_object_class_init_cb(JSContextRef ctx,JSObjectRef object)
{
    PyGlobalInterpreterLock pyGuard;
    Py_INCREF(my_py_obj(object));
}

static void python_object_class_finalize_cb(JSObjectRef object)
{
    PyGlobalInterpreterLock pyGuard;
    Py_DECREF(my_py_obj(object));    
}


static bool python_object_hasPropertyCallback (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    jstr str(propertyName);
    std::string s(str.str());
    if ( s == "toString" || s == "valueOf" || s == "toJSON") 
    {
       return false;
    }
    return true;
}


/*
 * called to fetch a property of a Python object from javascript.
 *
 */

static JSValueRef python_object_getPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    PyGlobalInterpreterLock pyGuard;

    PyObjectRef name = PyString_FromString( jstr(propertyName).str() );
    PyObjectRef ref  = PyObject_GetAttr( my_py_obj(object), name );

    return PJSMarshal::make_js_value( context, ref );
}

/*
 * called to on invocation of a Python method from javascript.
 *
 */
static JSValueRef python_object_callAsFunctionCallback(
                JSContextRef context, 
                JSObjectRef function, 
                JSObjectRef thisObject, 
                size_t argumentCount, 
                const JSValueRef arguments[], 
                JSValueRef* exception)
{
    PyGlobalInterpreterLock pyGuard;

    return PJSMarshal::call( 
                my_py_obj(function), 
                context, 
                argumentCount, 
                arguments 
    );
}

/*
 * called to on invocation of a Python constructor from javascript using new.
 *
 * note this is syntactic sugar as it is possible to just call the Python
 * constructor as a function without new. unsurprisingly this does almost
 * the same as the function above, expect returning a JSObjectRef instead
 * of a JSValueRef
 */

static JSObjectRef python_object_class_constructor_cb(
                JSContextRef context, 
                JSObjectRef constructor, 
                size_t argumentCount, 
                const JSValueRef arguments[], 
                JSValueRef* exception)
{
    PyGlobalInterpreterLock pyGuard;

    JSValueRef ref = PJSMarshal::call( 
                            my_py_obj(constructor), 
                            context, 
                            argumentCount, 
                            arguments 
                        );
    JSValueRef ex = 0;
    return JSValueToObject(context, ref, &ex);
}

extern "C" const JSClassDefinition python_object_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "PyObject",              // className
    NULL,                  // parentClass
    NULL,                  // staticValues
    NULL,                  // staticFunctions
    python_object_class_init_cb,         // initialize
    python_object_class_finalize_cb,     // finalize
    python_object_hasPropertyCallback,   // hasProperty
    python_object_getPropertyCallback,   // getProperty
    NULL,                  // setProperty
    NULL,                  // deleteProperty
    NULL,                  // getPropertyNames
    python_object_callAsFunctionCallback,// callAsFunction
    python_object_class_constructor_cb,  // callAsConstructor
    NULL,                  // hasInstance  
    NULL                   // convertToType
};




