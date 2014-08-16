#include "pyjsGlue.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <list>
#include <functional>

extern "C" PyTypeObject jswrapper_python_objectType;

extern "C" PyObject* new_js_wrapper_python_object(JSContextRef context, JSObjectRef that, JSObjectRef func);


std::map<const JSClassDefinition*,JSClassRef> jclass::map_;


void PJSMarshal::eval(const char* s)
{
    PyObjectRef code =  Py_CompileString(s, "test", Py_file_input);
    PyObject* main_module = PyImport_AddModule("__main__");

    PyObject* global_dict = PyModule_GetDict(main_module);
    PyObjectRef local_dict = PyDict_New();

    PyObjectRef obj = PyEval_EvalCode((PyCodeObject*)(PyObject*)code, global_dict, local_dict);
}

JSValueRef PJSMarshal::call(PyObject* that, JSContextRef context, size_t argumentCount, const JSValueRef arguments[])
{
    PyObjectRef args = get_js_args(context,argumentCount,arguments);
    PyObjectRef result = invoke(that,args);
    return make_js_value(context,result);
}

PyObject* PJSMarshal::invoke(PyObject* controller, PyObject* args)
{
    PyObject* ret = PyObject_CallObject(controller, args);
    return ret;
}


bool PJSMarshal::is_js_array(JSContextRef context,JSValueRef jsObject)
{
    jstr scriptJS("return arguments[0].constructor == Array.prototype.constructor");
    JSObjectRef fn = JSObjectMakeFunction(context, NULL, 0, NULL, scriptJS.ref(), NULL, 1, NULL);
    JSValueRef result = JSObjectCallAsFunction(context, fn, NULL, 1, (JSValueRef*)&jsObject, NULL);
   
    bool ret = JSValueToBoolean(context,result);
    return ret;
}



bool PJSMarshal::is_jswrapper_object(JSContextRef context,JSValueRef jsObject)
{
    return JSValueIsObjectOfClass(context, jsObject, jclass(&python_object_class_def).ref() );
}

std::string PJSMarshal::get_js_string(JSContextRef context, JSValueRef arg)
{
    jstr str(context,arg);
    return str.str();
}

double PJSMarshal::get_js_number(JSContextRef context, JSValueRef arg)
{
    JSValueRef ex = 0;
    double ret = JSValueToNumber(context, arg, &ex);    
    return ret;
}



PyObject* PJSMarshal::get_js_object(JSContextRef context, JSValueRef arg)
{
    JSValueRef ex = 0;
    JSObjectRef obj = JSValueToObject(context, arg, &ex);
    JSPropertyNameArrayRef names = JSObjectCopyPropertyNames(context, obj);

    PyObject* ret = PyDict_New();
    size_t len = JSPropertyNameArrayGetCount(names);
    for ( size_t i = 0; i < len; i++ )
    {
        jstr key = JSPropertyNameArrayGetNameAtIndex(names,i);
        JSValueRef  val = JSObjectGetProperty(context, obj, key.ref(), &ex);

        PyDict_SetItemString(ret, key.str(), get_js_arg(context,val,arg) );
    }

    JSPropertyNameArrayRelease(names);
    return ret;
}


PyObject* PJSMarshal::get_js_array(JSContextRef context, JSValueRef arg)
{
    JSValueRef ex = 0;
    JSObjectRef obj = JSValueToObject(context, arg, &ex);
    
    jstr lengthPropertyName("length");
    JSValueRef length = JSObjectGetProperty(context, obj, lengthPropertyName.ref(), &ex);

    int len = jnum(context,length).integer();

    PyObject* ret = PyList_New(len);

    for ( int i = 0; i < len; i++ )
    {
        JSValueRef  val = JSObjectGetPropertyAtIndex(context, obj, i, &ex);
        PyList_SetItem(ret, i, get_js_arg(context,val) );
    }
    
    return ret;
}


PyObject* PJSMarshal::get_js_arg(JSContextRef context, JSValueRef argument, JSValueRef that)
{
    size_t len;
    char *cstr;
    JSType jt = JSValueGetType(context, argument);
    if ( jt ==  kJSTypeString )
    {
        std::string str = get_js_string(context,argument);
        return PyString_FromString(str.c_str());
    }
    else if ( jt == kJSTypeNumber )
    {
        double num = jnum(context,argument).number();
        return PyFloat_FromDouble(num);
    }
    else if ( jt == kJSTypeObject )
    {
        JSValueRef   ex = 0;
        JSObjectRef obj = JSValueToObject(context, argument, &ex);
        JSObjectRef   t = that ? JSValueToObject(context, that, &ex) : NULL;

        if(is_jswrapper_object(context,argument))
        {
             PyObject* py = (PyObject*)JSObjectGetPrivate(obj);
             Py_INCREF(py);
             return py;
        }

        bool isFunction = JSObjectIsFunction(context, obj);
        if(isFunction)
        {
            return new_js_wrapper_python_object(context,t,obj);
        }

        bool isArray = is_js_array(context,argument);
        if ( isArray )
        {
            return  get_js_array(context,argument);
        }
        else 
        {
            return new_js_wrapper_python_object(context,obj,NULL);
        }
    }
    
    return Py_BuildValue("");
}


PyObject* PJSMarshal::get_js_args(JSContextRef context, size_t argumentCount, const JSValueRef arguments[])
{
    PyObject* args = PyTuple_New(argumentCount);
    for (int i = 0; i < argumentCount; i++ )
    {
        PyTuple_SetItem(args,i,get_js_arg(context,arguments[i]));
    }

    return args;
}


std::vector<JSValueRef> PJSMarshal::make_js_values(JSContextRef context,PyObject* args)
{
    Py_ssize_t len = PySequence_Size(args);
    std::vector<JSValueRef> arguments;
    for(Py_ssize_t i = 0; i < len; i++)
    {
        PyObjectRef item = PySequence_GetItem(args,i);
        JSValueRef value = make_js_value(context,item);
        arguments.push_back(value);
    }    
    return arguments;
}



JSValueRef PJSMarshal::make_js_value(JSContextRef context, PyObject* pyObj)
{
//    PyObject_Print(pyObj, stdout,0);
//    printf("\n");
    JSValueRef ex = 0;

    if (!pyObj)
        return JSValueMakeUndefined(context);

    if( PyObject_TypeCheck(pyObj, &jswrapper_python_objectType) )
    {
        jswrapper_python_object* object = (jswrapper_python_object*)pyObj;
        return object->that;
    }
    else if ( PyString_Check(pyObj) ) 
    {
        char* c = PyString_AsString(pyObj);

        PyObjectRef uni = PyUnicode_Decode(c, strlen(c), "ISO-8859-1", "");
        PyObjectRef utf8 = PyUnicode_AsUTF8String(uni);

        c = PyString_AsString(utf8);
        jstr str(c);

        return JSValueMakeString(context, str.ref());
    }
    else if ( PyUnicode_Check(pyObj) )
    {
        PyObjectRef utf8 = PyUnicode_AsUTF8String(pyObj);
        char* c = PyString_AsString(utf8);
        jstr str(c);
        return JSValueMakeString(context, str.ref());

    }
    else if ( pyObj == Py_None ) 
    {
        return JSValueMakeUndefined(context);
    }
    else if ( PyBool_Check(pyObj) ) 
    {
        bool b = pyObj == Py_True;
        return JSValueMakeBoolean(context, b);
    }
    else if (PyInt_Check(pyObj) ) 
    {
        long l = PyInt_AsLong(pyObj);
        return JSValueMakeNumber(context,(double)l);
    }
    else if (PyFloat_Check(pyObj) ) 
    {
        double d = PyFloat_AsDouble(pyObj);
        return JSValueMakeNumber(context,d);
    }
    else if (PyTuple_CheckExact(pyObj)) 
    {
        Py_ssize_t len = PySequence_Size(pyObj);
        JSValueRef arguments[len];
        for(Py_ssize_t i = 0; i < len; i++)
        {
            PyObjectRef item = PySequence_GetItem(pyObj,i);
            JSValueRef value = make_js_value(context,item);
            arguments[i] = value;
        }
        return JSObjectMakeArray(context, len, arguments, &ex);
    }
    else if (PyList_CheckExact(pyObj)) 
    {
        Py_ssize_t len = PySequence_Size(pyObj);
        JSValueRef arguments[len];
        for(Py_ssize_t i = 0; i < len; i++)
        {
            PyObjectRef item = PySequence_GetItem(pyObj,i);
            JSValueRef value = make_js_value(context,item);
            arguments[i] = value;
        }
        return JSObjectMakeArray(context, len, arguments, &ex);
    }
    else if (PyDict_CheckExact(pyObj))
    {
        JSObjectRef ret = JSObjectMake(context,0,NULL);
        PyObjectRef keys = PyObject_CallMethod(pyObj,(char*)"keys",NULL);
        Py_ssize_t len = PySequence_Size(keys);
        for(Py_ssize_t i = 0; i < len; i++)
        {
            PyObjectRef key = PySequence_GetItem(keys,i);
            char* k = PyString_AsString(key);
            PyObjectRef value = PyMapping_GetItemString(pyObj, k);
            JSValueRef val = make_js_value(context,value);
            jstr str(k);
            JSObjectSetProperty(context, ret, str.ref(), val, kJSPropertyAttributeNone, &ex);
        }  
        return ret;      
    }
    else if (  PyObject_TypeCheck(pyObj, &PyMethod_Type) 
             || PyObject_TypeCheck(pyObj, &PyFunction_Type)
             || PyObject_TypeCheck(pyObj, &PyBaseObject_Type)
             || PyInstance_Check(pyObj)
             || PyClass_Check(pyObj)
             || PyType_Check(pyObj)
             )
    {
        jclass clazz(&python_object_class_def);
        return clazz.create(context,pyObj);
    }

    return JSValueMakeUndefined(context);
}




