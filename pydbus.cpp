//#include "pyjsGlue.h"
#include <Python.h>
#include <iostream>
#include <gio/gio.h>
#include <glib.h>
//#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <vector>
#include <map>
#include <structmember.h>
//#include <JavaScriptCore/JSContextRef.h>

class pyobj_ref
{
public:

    pyobj_ref()
        : ref_(0)
    {}

    pyobj_ref(PyObject* ref)
        : ref_(ref)
    {}

    ~pyobj_ref()
    {
        if(ref_)
            Py_DECREF(ref_);
        ref_ = 0;
    }

    PyObject* operator->()
    {
        return ref_;
    }


    pyobj_ref& operator=(const PyObjectRef& rhs)
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


GDBusConnection* dbus = 0;

GVariant* make_variant(PyObject* pyObj)
{
//    PyObject_Print(pyObj, stdout,0);
//    printf("\n");
    if (!pyObj)
    {
        return g_variant_new_maybe (NULL,NULL);
    }

    if ( PyUnicode_Check(pyObj) ) 
    {
        const char* c = PyUnicode_AsUTF8(pyObj);
        return g_variant_new("s",c);                       
    }
    else if ( PyByteArray_Check(pyObj) )
    {
        char* c = PyByteArray_AsString(pyObj);
        return g_variant_new("s",c);                       
    }
    else if ( pyObj == Py_None ) 
    {
       return g_variant_new_maybe (NULL,NULL);
    }
    else if ( PyBool_Check(pyObj) ) 
    {
        bool b = pyObj == Py_True;
        return g_variant_new_boolean(b);               
    }
    else if (PyLong_Check(pyObj) ) 
    {
        long l = PyLong_AsLong(pyObj);
        return g_variant_new("x",l);
    }
    else if (PyFloat_Check(pyObj) ) 
    {
        double d = PyFloat_AsDouble(pyObj);
        return g_variant_new_double(d);
    }
    else if (PyTuple_CheckExact(pyObj)) 
    {
        Py_ssize_t len = PySequence_Size(pyObj);

        //builder
        GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
        for(Py_ssize_t i = 0; i < len; i++)
        {
            pyobj_ref item = PySequence_GetItem(pyObj,i);
            g_variant_builder_add(builder, "v", make_variant(item));
        }
        return g_variant_builder_end(builder);
    }
    else if (PyList_CheckExact(pyObj)) 
    {
        Py_ssize_t len = PySequence_Size(pyObj);
        //builder
        GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
        for(Py_ssize_t i = 0; i < len; i++)
        {
            pyobj_ref item = PySequence_GetItem(pyObj,i);
            g_variant_builder_add(builder, "v", make_variant(item));
        }
        return g_variant_builder_end(builder);
    }
    else if (PyDict_CheckExact(pyObj))
    {
            PyObject_Print(pyObj, stdout,0);
                printf("\n");

        GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
        pyobj_ref keys = PyDict_Keys(pyObj);//PyObject_CallMethod(pyObj,(char*)"keys",NULL);
            PyObject_Print(keys, stdout,0);
                printf("\n");
        Py_ssize_t len = PySequence_Size(keys);
        std::cout << "DICT: " << len << std::endl;
        for(Py_ssize_t i = 0; i < len; i++)
        {
            std::cout << "DICT: " << i  << std::endl;
            pyobj_ref key = PySequence_GetItem(keys,i);

            PyObject_Print(key, stdout,0);
                printf("\n");

            const char* k = PyUnicode_AsUTF8(key);
            std::cout << "K: " << k << std::endl;
            pyobj_ref value = PyMapping_GetItemString(pyObj, k);

            GVariant* dict = g_variant_new("{sv}", k,make_variant(value));
            g_variant_builder_add_value(builder,dict);
        }  
        return g_variant_builder_end(builder);
    }
/*
#define PyClass_Check(obj) PyObject_IsInstance(obj, (PyObject *)&PyType_Type)

    else if (  PyObject_TypeCheck(pyObj, &PyMethod_Type) 
             || PyObject_TypeCheck(pyObj, &PyFunction_Type)
             || PyObject_TypeCheck(pyObj, &PyBaseObject_Type)
  //           || PyInstance_Check(pyObj)
             || PyClass_Check(pyObj)
             || PyType_Check(pyObj)
             )
    {
        jclass clazz(&python_object_class_def);
        return clazz.create(context,pyObj);
    }
*/
    return g_variant_new_maybe (NULL,NULL);
}



typedef struct {
    PyObject_HEAD
 //   GDBusConnection* dbus;
//    JSObjectRef  that;
//    JSObjectRef  func;
//    JSContextRef context;
} webkit_dbus_object;



static int webkit_dbus_object_init(webkit_dbus_object *self, PyObject *args, PyObject *kwds)
{
//    self->that = 0;
  //  self->func = 0;

   //   self->dbus =  g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL);

    return 0;
}

static void webkit_dbus_object_dealloc(webkit_dbus_object* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * webkit_dbus_object_getattr(webkit_dbus_object* self, char* name)
{
//    if ( self->that == 0 )
  //      return Py_BuildValue("");

    //jstr propertyName(name);

//    JSValueRef ex=0;
//    JSValueRef result = JSObjectGetProperty(self->context, self->that, propertyName.ref(), &ex);

//    return PJSMarshal::get_js_arg(self->context, result, self->that);
    return 0;
}


static int webkit_dbus_object_setattr(webkit_dbus_object* self, char* name, PyObject * value)
{
/*    jstr propertyName(name);

    JSValueRef ex=0;
    JSValueRef val = PJSMarshal::make_js_value(self->context,value);
    JSObjectSetProperty(self->context, self->that, propertyName.ref(), val, kJSPropertyAttributeNone, &ex);
*/
    return 0;
}

static void send_dbus_signal( GDBusConnection* dbus, std::string s, PyObject*  msg)
{
  GVariantBuilder *builder;

  builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);

    g_variant_builder_add(builder,"v",make_variant(msg));

    GVariant* params = g_variant_builder_end (builder);

    g_dbus_connection_emit_signal (dbus,
                                 NULL,
                                 "/com/example/TestService/object",
                                 "com.example.TestService",
                                 s.c_str(),
                                 params,
                                 NULL);
}


static PyObject* webkit_dbus_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    webkit_dbus_object* webit_dbus = (webkit_dbus_object*)self;

    std::cout << "HERE!" << std::endl;

PyObject_Print(args, stdout,0);
    printf("\n");    

    Py_ssize_t len = PySequence_Size(args);

    //builder
    pyobj_ref signal = PySequence_GetItem(args,0);
    pyobj_ref msg = PySequence_GetItem(args,1);

    const char* c = PyUnicode_AsUTF8(signal);

    //Py_INCREF(msg);

    send_dbus_signal(dbus,c,msg);

/*
    if( wrapper->func == NULL && wrapper->that == NULL) {
         return Py_BuildValue("");
    }

    std::vector<JSValueRef> arguments = PJSMarshal::make_js_values(wrapper->context,args);

    JSValueRef ex = 0;
    JSValueRef result = JSObjectCallAsFunction(
                            wrapper->context,
                            wrapper->func,
                            wrapper->that,  
                            arguments.size(), 
                            &arguments[0], 
                            &ex
                        );

    PyObject* ret = PJSMarshal::get_js_arg(wrapper->context, result, wrapper->that);

    return ret;
    */
     Py_RETURN_NONE;
}

PyTypeObject webkit_dbus_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "pywebkit.PyDBUSObject",        /*tp_name*/
    sizeof(webkit_dbus_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)webkit_dbus_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    (getattrfunc)webkit_dbus_object_getattr, /*tp_getattr*/
    (setattrfunc)webkit_dbus_object_setattr, /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    webkit_dbus_object_call,     /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "webkit dbus",       /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)webkit_dbus_object_init, /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyObject* send_signal(PyObject* self, PyObject* args)
{
    Py_ssize_t len = PySequence_Size(args);

    if(len<2)
    {
        std::cout << "less than two args" << std::endl;
    }

    pyobj_ref signal = PySequence_GetItem(args,0);
    pyobj_ref msg = PySequence_GetItem(args,1);
    const char* c = PyUnicode_AsUTF8(signal);

    std::cout << "SEND SIGNAL " << (void*)dbus << " " << c << " " << len <<  std::endl;

    send_dbus_signal(dbus,c,msg);

    Py_RETURN_NONE;
}


static PyMethodDef webkit_dbus_module_methods[] = {
    {"send_signal",  send_signal, METH_VARARGS, "Send a signal to webview child process via dbus."},
    {NULL}  /* Sentinel */
};


/*
 * create a new jswrapper Python object from native code.
 *
 */

extern "C" PyObject* new_webkit_dbus_object()//JSContextRef context, JSObjectRef that, JSObjectRef func)
{
    webkit_dbus_object* self = (webkit_dbus_object *)(&webkit_dbus_objectType)->tp_alloc(&webkit_dbus_objectType, 0);
 //   self->that = that;
 //   self->func = func;
 //   self->context = context;

    return (PyObject*)self;
}

/*
 * external interface to make the jswrapper object avail from Python code.
 * ( in this example will be called once in the class initialization 
 *   of the webkit widget )
 */

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "pywebkit",
    "dbus interface",
    -1,
    webkit_dbus_module_methods,
    NULL,NULL,NULL,NULL
};

PyMODINIT_FUNC PyInit_pywebkit(void) {

    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;

        PyObject* m;

    std::cout << "inject_webkit_dbus_object" << std::endl;
    webkit_dbus_objectType.tp_new =  PyType_GenericNew;

   // jswrapper_python_objectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&webkit_dbus_objectType) < 0)
        return NULL;

    std::cout << "inject_webkit_dbus_object2" << std::endl;

    m = PyModule_Create(&moduledef);

    // ?????
    Py_INCREF(&webkit_dbus_objectType);
    Py_INCREF(m);

    int r = PyModule_AddObject(m, "PyDBUSObject", (PyObject *)&webkit_dbus_objectType);

    std::cout << "addObj: " << r << std::endl;


     r = PyModule_AddStringConstant(m, "testValue", "testVALUE");
    std::cout << "addConst: " << r << std::endl;

//    dbus =  g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL);

    return m;
}

extern "C" void inject_webkit_dbus_object()
{
    if(!Py_IsInitialized())
    {
        Py_Initialize();
    }

    PyObject* m = PyInit_pywebkit();

    PyObject* sys_modules = PyImport_GetModuleDict();
    PyDict_SetItemString(sys_modules, "pywebkit", m);
    //PyImport_ImportModule("pywebkit");
    
  /*  
    PyObject* n = PyUnicode_FromString("gi.repository.Py");
    PyObject* m = PyImport_GetModule(n);
    PyObject*d = PyModule_GetDict(m);
    PyObject*v = PyCFunction_NewEx(&webkit_dbus_module_methods[0], (PyObject*)NULL, n);
    PyDict_SetItemString(d, "send_signal", v);
*/
        dbus =  g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL);

}



