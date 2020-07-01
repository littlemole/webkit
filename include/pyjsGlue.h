
#ifndef __MO_WEBKIT_GLUE_H__
#define __MO_WEBKIT_GLUE_H__

#include <Python.h>
#include <glib.h>
#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <vector>
#include <map>

/*
 * jswrapper_object custom Python object definition.
 * this essentially wraps a Javascript object for
 * use from Python.
 *
 * in practice, you should only call into it from the main GUI thread.
 * use GObject.add_idle(callback,*args) to return results
 * when on a worker thread.
 *
 */

typedef struct {
    PyObject_HEAD
    JSObjectRef  that;
    JSObjectRef  func;
    JSContextRef context;
} jswrapper_python_object;


/*
 * acquire the Python GIL in RAII way.
 * necessary when calling into Python from C
 * when on threads not created by Python, ie
 * some javascript thread.
 */

class PyGlobalInterpreterLock
{
public:

    PyGlobalInterpreterLock()
    {
        gstate_ = PyGILState_Ensure();
    }

    ~PyGlobalInterpreterLock()
    {
        PyGILState_Release(gstate_);
    }

private:

    PyGILState_STATE gstate_;

};

/*
 * simple smart pointer for PyObjects. calls Py_DECREF on desctruction.
 *
 */

class PyObjectRef
{
public:

    PyObjectRef()
        : ref_(0)
    {}

    PyObjectRef(PyObject* ref)
        : ref_(ref)
    {}

    ~PyObjectRef()
    {
        if(ref_)
            Py_DECREF(ref_);
        ref_ = 0;
    }

    PyObject* operator->()
    {
        return ref_;
    }


    PyObjectRef& operator=(const PyObjectRef& rhs)
    {
        if ( this == &rhs )
            return *this;

        if(ref_)
            Py_DECREF(ref_);
        ref_ = rhs.ref_;
        Py_INCREF(ref_);
        return *this;
    }

    operator PyObject* ()
    {
        return ref_;
    }

    operator const PyObject* () const
    {
        return ref_;
    }

private:
    PyObject* ref_;
};

/*
 * deal with Javascript Strings. there are three ways to construct a 
 * String:
 *
 * - from a JStringRef obtained elsewhere. the ref will NOT be released on destruction.
 * - from a C string (char*). the JSStringRef provided will be released on destruction.
 * - from a generic JSValueRef. the JSStringRef provided will be released on destruction.
 *
 * after initialization, jstr provides the referenced char as
 * - ref() which will provide a JStringRef representation of the string
 * - str() which will provide a char* representation of the string
 *
 * allways UTF-8
 */

class jstr
{
public:
    jstr(JSStringRef ref)
        : value_(0), str_(0), ref_(ref)
    {
        get_str();
    }

    jstr(const gchar* str)
        : value_((JSValueRef)1), str_(g_strdup(str)), ref_(0)
    {
        ref_ = JSStringCreateWithUTF8CString(str_);
    }

    jstr(JSContextRef context,JSValueRef value)
        :value_(value), str_(0), ref_(0)
    {
        JSValueRef ex = 0;
        ref_ = JSValueToStringCopy(context,value,&ex);
        get_str();
    }

    ~jstr()
    {
        if(value_)
            JSStringRelease(ref_);       
        g_free(str_); 
    }

    gchar* str()
    {
        return str_;
    }

    JSStringRef ref()
    {
        return ref_;
    }

private:

    void get_str()
    {
        size_t len = JSStringGetMaximumUTF8CStringSize(ref_);
        str_ = g_new(char, len);
        JSStringGetUTF8CString(ref_, str_, len);
    }

    JSValueRef value_;
    JSStringRef ref_;
    gchar* str_;
};


/*
 * deal with custom Javascript Numbers.
 *
 */

class jnum
{
public:
    jnum(JSContextRef context,JSValueRef value)
    {
        JSValueRef ex = 0;
        value_ = JSValueToNumber(context, value, &ex);           
    }

    int integer()
    {
        return (int)value_;
    }

    double number()
    {
        return value_;
    }

private:
    double value_;
};

class jbool
{
public:
    jbool(JSContextRef context,JSValueRef value)
    {
        value_ = JSValueToBoolean(context, value);           
    }

    bool boolean()
    {
        return value_;
    }

private:
    bool value_ = false;
};

/*
 * deal with custom Javascript Classes. there are three ways to construct a 
 *
 */

class jclass
{
public:

    jclass(const JSClassDefinition* clazz)
    {
        if( map_.count(clazz) == 0 )
        {
            map_.insert( std::make_pair(clazz,JSClassCreate(clazz)) );
        }
        class_ = map_[clazz];
    }

    ~jclass()
    { 
       // no longer free it as we cache it in the map.
       // only one instance per class will be created
       // for the process runtime ever.
       //JSClassRelease(class_);
    }

    JSClassRef ref()
    {
        return class_;
    }

    JSObjectRef create(JSContextRef ctx, void* data)
    {
        return JSObjectMake(ctx, class_, data);
    }

private:
    static std::map<const JSClassDefinition*,JSClassRef> map_;
    JSClassRef class_;
};

/*
 * Javascript to Python Marshaler.
 *
 * - allows to convert Javascript to Python values and vice versa.
 * - provides facilites to call into python code from Javascript 
 *   objects implemented in native code.
 *
 * caveat: on threads that are not owned by python - ie js threads -
 *         you are required to use PyGlobalInterpreterLock
 */

class PJSMarshal
{
public:

    static void eval(const char* s);

    static JSValueRef call(PyObject* that,JSContextRef context, size_t argumentCount, const JSValueRef arguments[]);

    static JSValueRef make_js_value(JSContextRef context, PyObject* pyObj);
    static PyObject* get_js_arg(JSContextRef context, JSValueRef argument, JSValueRef that = 0);

    static std::vector<JSValueRef> make_js_values(JSContextRef context,PyObject* args);

private:

    static PyObject* invoke(PyObject* controller, PyObject* args);

    static PyObject* get_js_args(JSContextRef context, size_t argumentCount, const JSValueRef arguments[]);



    static std::string get_js_string(JSContextRef context, JSValueRef arg);
    static double get_js_number(JSContextRef context, JSValueRef arg);

    static PyObject* get_js_object(JSContextRef context, JSValueRef arg);
    static PyObject* get_js_array(JSContextRef context, JSValueRef arg);

    static bool is_js_array(JSContextRef context,JSValueRef jsObject);
    static bool is_jswrapper_object(JSContextRef context,JSValueRef jsObject);
};


extern "C" const JSClassDefinition python_object_class_def;


#endif

