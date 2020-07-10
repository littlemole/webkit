#include "pyglue.h"
#include "marshal.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <gio/gio.h>
#include <glib.h>
#include <gdk/gdk.h>

/////////////////////////////////////////////
// globals

#define PROG "[pydbus]"

static const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_prefix = "/org/oha7/webkit/WebKitDBus/controller/";
static const std::string dbus_object_path_recv_prefix = "/org/oha7/webkit/WebKitDBus/view/";

static std::string dbus_object_path_send_path;
static std::string dbus_object_path_recv_path; 

static GDBusConnection* dbus = 0;
static std::string sid;
static PyObjectRef cb;

/////////////////////////////////////////////


struct SendSignalData
{
    std::string signal_name;
    GVariant *parameters;
};

/////////////////////////////////////////////

int  main_thread_send_signal(void* data );


/////////////////////////////////////////////

typedef struct {
    PyObject_HEAD
    std::string signal_name;
} signal_object;

static int signal_object_init(signal_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void signal_object_dealloc(signal_object* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* signal_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    signal_object* that = (signal_object*)self;

    Py_ssize_t len = PySequence_Size(args);

    GVariantBuilder *builder;

    builder = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);

    for( int i = 0; i < len; i++)
    {
        PyObjectRef arg = PySequence_GetItem(args,0);

        g_variant_builder_add(builder,"v",make_variant(arg));
    }

    GVariant* params = g_variant_builder_end(builder);

    SendSignalData* ssd = new SendSignalData;
    ssd->signal_name = that->signal_name;
    ssd->parameters = params;

    gdk_threads_add_idle ( &main_thread_send_signal, ssd);

    Py_RETURN_NONE;
}

PyTypeObject signal_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.SignalObject",        /*tp_name*/
    sizeof(signal_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)signal_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,/*tp_getattr*/
    0,/*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    signal_object_call,     /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "signal wrapper objects",       /* tp_doc */
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
    (initproc)signal_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew                         /* tp_new */
};

extern "C" PyObject* new_signal_object(const char* name)
{
    signal_object* self = (signal_object *)(&signal_objectType)->tp_alloc(&signal_objectType, 0);
    self->signal_name = name;

    return (PyObject*)self;
}
/////////////////////

typedef struct {
    PyObject_HEAD
} signals_object;

static int signals_object_init(signals_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void signals_object_dealloc(signals_object* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * signals_object_getattr(signals_object* self, char* name)
{
    return new_signal_object(name);
}
 



PyTypeObject signals_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.SignalsObject",        /*tp_name*/
    sizeof(signals_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)signals_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    (getattrfunc)signals_object_getattr, /*tp_getattr*/
    0,/*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,    /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "signals objects",       /* tp_doc */
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
    (initproc)signals_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,                         /* tp_new */
};

extern "C" PyObject* new_signals_object()
{
    signals_object* self = (signals_object *)(&signals_objectType)->tp_alloc(&signals_objectType, 0);

    return (PyObject*)self;
}
///////////////////////////////////

static void signal_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    g_print (PROG " received signal %s %s\n", signal_name, g_variant_get_type_string (parameters));

    PyGlobalInterpreterLock lock;

    int b = PyObject_HasAttrString(cb,signal_name);
    if(!b)
    {
        g_print (PROG "signal handler for signal name %s not found.\n", signal_name);
        return;        
    }

    PyObjectRef args = gvariant_to_py_value(parameters);
    PyObjectRef name = PyUnicode_FromString(signal_name);

    PyObjectRef callable = PyObject_GetAttr(cb,name);
    if(!PyMethod_Check(callable))
    {
        g_print (PROG "signal handler for signal name %s is not a method.\n", signal_name);
        return;        
    }

    int s = PyTuple_Size(args);

    PyObjectRef tuple = PyTuple_New(s+1);
    Py_INCREF(cb); // SetItem steals ref
    PyTuple_SetItem(tuple,0,cb);

    for ( int i = 0; i < s; i++)
    {
        // GetItem gives borrowed ref
        PyObject* item = PyTuple_GetItem(args,i);
        Py_INCREF(item);
        PyTuple_SetItem(tuple,i+1,item);
    }

    // borrowed ref
    PyObject* fun = PyMethod_Function(callable);

    // call python!
    PyObjectRef ret = PyObject_CallObject(fun,tuple);
}


static void got_dbus (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    dbus =  g_bus_get_finish (res, NULL);    

    sid = g_dbus_connection_signal_subscribe (
        dbus, 
        /*sender*/ NULL, 
        //"com.example.TestService",
         dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
        //"/com/example/TestService/object",
        dbus_object_path_recv_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL
    );    
}


static void send_dbus_signal( GDBusConnection* dbus, std::string s, GVariant*  params)
{
    g_dbus_connection_emit_signal(
        dbus,
        NULL,
        dbus_object_path_send_path.c_str(),
        dbus_interface.c_str(),
       // "/com/example/TestService/view",
       // "com.example.TestService",
        s.c_str(),
        params,
        NULL
    );
}

int  main_thread_send_signal(void* data )
{
    SendSignalData* ssd = (SendSignalData*)data;

    send_dbus_signal(dbus,ssd->signal_name,ssd->parameters);

    delete ssd;

    return 0;
}

static PyObject* pywebkit_send_signal(PyObject* self, PyObject* args)
{
    Py_ssize_t len = PySequence_Size(args);

    if(len<2)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than two args for call to send_signal!");
        return NULL;
    }
    

    PyObjectRef signal = PySequence_GetItem(args,0);
    PyObjectRef msg = PySequence_GetItem(args,1);
    const char* c = PyUnicode_AsUTF8(signal);

    GVariantBuilder *builder;

    builder = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);

    g_variant_builder_add(builder,"v",make_variant(msg));

    GVariant* params = g_variant_builder_end(builder);

    SendSignalData* ssd = new SendSignalData;
    ssd->signal_name = c;
    ssd->parameters = params;

    gdk_threads_add_idle ( &main_thread_send_signal, ssd);

    Py_RETURN_NONE;
}

static PyObject* pywebkit_bind(PyObject* self, PyObject* args)
{
    Py_ssize_t len = PySequence_Size(args);

    g_print (PROG "on signal \n");

    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than one args for call to on_signal!");
        return NULL;
    }

    cb = PySequence_GetItem(args,0);

    Py_RETURN_NONE;
}

static PyMethodDef pywebkit_module_methods[] = {
    {"send_signal",  pywebkit_send_signal, METH_VARARGS, "Send a signal to webview child process via dbus."},
    {"bind",  pywebkit_bind, METH_VARARGS, "register listening for signal via dbus."},
    {NULL}  /* Sentinel */
};



/*
 * external interface to make the jswrapper object avail from Python code.
 * ( in this example will be called once in the class initialization 
 *   of the webkit widget )
 */

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "WebKitDBus",
    "dbus interface",
    -1,
    pywebkit_module_methods,
    NULL,NULL,NULL,NULL
};

PyMODINIT_FUNC PyInit_WebKitDBus(void) {

    PyObject* m;

    m = PyModule_Create(&moduledef);

//    jswrapper_python_objectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&signal_objectType) < 0)
        return 0;    

    if (PyType_Ready(&signals_objectType) < 0)
        return 0;    

    Py_INCREF(&signal_objectType);
    PyModule_AddObject(m, "SignalObject", (PyObject *)&signal_objectType);

    Py_INCREF(&signals_objectType);
    PyModule_AddObject(m, "SignalsObject", (PyObject *)&signals_objectType);

    PyObject* signalsObject = new_signals_object();
    PyModule_AddObject(m, "View", signalsObject);

    gchar* c = g_dbus_generate_guid();
    sid = std::string(c);
    g_free(c);

    PyModule_AddStringConstant(m, "uid", sid.c_str());

    std::ostringstream oss_send;
    oss_send << dbus_object_path_send_prefix << sid;
    dbus_object_path_send_path = oss_send.str();

    std::ostringstream oss_recv;
    oss_recv << dbus_object_path_recv_prefix << sid;
    dbus_object_path_recv_path = oss_recv.str();

    g_print (PROG "Interface; %s.\n", dbus_interface.c_str());
    g_print (PROG "Send; %s.\n", dbus_object_path_send_path.c_str());
    g_print (PROG "Recv; %s.\n", dbus_object_path_recv_path.c_str());

    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    return m;
}




