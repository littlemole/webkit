#include "pyjsGlue.h"
#include <structmember.h>
#include <JavaScriptCore/JSContextRef.h>

/*
 * jswrapper custom python object wrapping a wekbit javascriptcore object
 *
 */

static int jswrapper_python_object_init(jswrapper_python_object *self, PyObject *args, PyObject *kwds)
{
    self->that = 0;
    self->func = 0;
    return 0;
}

static void jswrapper_python_object_dealloc(jswrapper_python_object* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * jswrapper_python_object_getattr(jswrapper_python_object* self, char* name)
{
    if ( self->that == 0 )
        return Py_BuildValue("");

    jstr propertyName(name);

    JSValueRef ex=0;
    JSValueRef result = JSObjectGetProperty(self->context, self->that, propertyName.ref(), &ex);

    return PJSMarshal::get_js_arg(self->context, result, self->that);
}


static int jswrapper_python_object_setattr(jswrapper_python_object* self, char* name, PyObject * value)
{
    jstr propertyName(name);

    JSValueRef ex=0;
    JSValueRef val = PJSMarshal::make_js_value(self->context,value);
    JSObjectSetProperty(self->context, self->that, propertyName.ref(), val, kJSPropertyAttributeNone, &ex);

    return 0;
}

static PyObject* jswrapper_python_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    jswrapper_python_object* wrapper = (jswrapper_python_object*)self;

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
}

PyTypeObject jswrapper_python_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "jswrapper.Object",        /*tp_name*/
    sizeof(jswrapper_python_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)jswrapper_python_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    (getattrfunc)jswrapper_python_object_getattr, /*tp_getattr*/
    (setattrfunc)jswrapper_python_object_setattr, /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    jswrapper_python_object_call,     /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "jswrapper objects",       /* tp_doc */
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
    (initproc)jswrapper_python_object_init, /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyMethodDef jswrapper_python_object_methods[] = {
    {NULL}  /* Sentinel */
};


/*
 * create a new jswrapper Python object from native code.
 *
 */

extern "C" PyObject* new_js_wrapper_python_object(JSContextRef context, JSObjectRef that, JSObjectRef func)
{
    jswrapper_python_object* self = (jswrapper_python_object *)(&jswrapper_python_objectType)->tp_alloc(&jswrapper_python_objectType, 0);
    self->that = that;
    self->func = func;
    self->context = context;

    return (PyObject*)self;
}

/*
 * external interface to make the jswrapper object avail from Python code.
 * ( in this example will be called once in the class initialization 
 *   of the webkit widget )
 */

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "jswrapper",
    "Javascript Function Python wrapper object",
    -1,
    jswrapper_python_object_methods,
    NULL,NULL,NULL,NULL
};

extern "C" void declare_jswrapper_python_object()
{
    if(!Py_IsInitialized())
    {
        Py_Initialize();
    }

    PyObject* m;

    jswrapper_python_objectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&jswrapper_python_objectType) < 0)
        return;

    m = PyModule_Create(&moduledef);

    Py_INCREF(&jswrapper_python_objectType);
    PyModule_AddObject(m, "Object", (PyObject *)&jswrapper_python_objectType);
}



