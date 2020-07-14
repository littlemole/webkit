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
static const std::string dbus_object_path_send_req_prefix = "/org/oha7/webkit/WebKitDBus/controller/request/";
static const std::string dbus_object_path_recv_req_prefix = "/org/oha7/webkit/WebKitDBus/view/request/";

static std::string dbus_object_path_send_req_path;
static std::string dbus_object_path_recv_req_path; 

static GDBusConnection* dbus = 0;
static std::string sid;
static PyObjectRef cb;

/////////////////////////////////////////////
// forwards

static void send_dbus_signal( GDBusConnection* dbus, std::string s, GVariant*  params);


/////////////////////////////////////////////
// signal object - function object that 
// emits signal when invoked

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
    py_dealloc(self);
}

static PyObject* signal_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    signal_object* that = (signal_object*)self;

    int len = length(args);
    GVariant* params = 0;

    if(len>0)
    {
        gvar_builder builder = gtuple();
        for_each(args,[&builder](int index, PyObjectRef& arg)
        {
            builder.add(make_variant(arg));
        });
        params = builder.build();
    }
    send_dbus_signal(dbus,that->signal_name,params);

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
    signal_object* self = py_alloc<signal_object>(&signal_objectType);
    self->signal_name = name;

    return (PyObject*)self;
}

/////////////////////
// signals object is a helper to invoke signals

typedef struct {
    PyObject_HEAD
} signals_object;

static int signals_object_init(signals_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void signals_object_dealloc(signals_object* self)
{
   py_dealloc(self);
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
    signals_object* self = py_alloc<signals_object>(&signals_objectType);

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

    gvar params(parameters);
    int len = params.length();
    if(len < 1)
    {
        g_print (PROG " received invalid signal %s %s\n", signal_name, g_variant_get_type_string (parameters));
        return;
    }

    PyObjectRef uid = gvariant_to_py_value(params.item(0));
    g_print (PROG "received signal %s %s\n", signal_name, uid.str() );

    if(len>1)
    {
        PyObjectRef args = gvariant_to_py_value(params.item(1));
        PyObjectRef ret = cb.invoke(signal_name, args);
    }
    else
    {
        PyObjectRef emptyTuple = PyTuple_New(0);
        PyObjectRef ret = cb.invoke(signal_name, emptyTuple);        
    }

}

static void got_dbus (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    dbus =  g_bus_get_finish (res, NULL);    

    sid = g_dbus_connection_signal_subscribe (
        dbus, 
        /*sender*/ NULL, 
         dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
        dbus_object_path_recv_req_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL
    );    
}


static void send_dbus_signal( GDBusConnection* dbus, std::string s, GVariant*  params)
{
    gchar* uid = g_dbus_generate_guid();

    gvar_builder builder = gtuple();
    builder.add(g_variant_new_string(uid));
    if(params)
    {
        builder.add(params);
    }
    GVariant* parameters = builder.build();

    g_free(uid);

    g_dbus_connection_emit_signal(
        dbus,
        NULL,
        dbus_object_path_send_req_path.c_str(),
        dbus_interface.c_str(),
        s.c_str(),
        parameters,
        NULL
    );
}

static PyObject* pywebkit_send_signal(PyObject* self, PyObject* args)
{
    int len = length(args);

    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than one arg for call to send_signal!");
        return NULL;
    }
    
    PyObjectRef signal = item(args,0);
    const char* signal_name = PyUnicode_AsUTF8(signal);

    GVariant* params = 0;
    if(len>1)
    {
        auto builder = gtuple();
        for( int i = 1; i < len; i++)
        {
            PyObjectRef arg = item( args, i);
            builder.add(make_variant(arg));
        }
        params = builder.build();
    }
    send_dbus_signal(dbus,signal_name,params);

    Py_RETURN_NONE;
}

static PyObject* pywebkit_bind(PyObject* self, PyObject* args)
{
    int len = length(args);

    g_print (PROG "on signal \n");

    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than one args for call to on_signal!");
        return NULL;
    }

    cb = item(args,0);

    Py_RETURN_NONE;
}

static PyMethodDef pywebkit_module_methods[] = {
    {"send_signal",  pywebkit_send_signal, METH_VARARGS, "Send a signal to webview child process via dbus."},
    {"bind",  pywebkit_bind, METH_VARARGS, "register listening for signal via dbus."},
    {NULL}  /* Sentinel */
};



static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "WebKitDBus",
    "dbus interface",
    -1,
    pywebkit_module_methods,
    NULL,NULL,NULL,NULL
};

void add_future_obj_def(PyObjectRef& m);
void add_future_iter_obj_def(PyObjectRef& m);
void add_task_obj_def(PyObjectRef& m);

PyMODINIT_FUNC PyInit_WebKitDBus(void) 
{
    // ready guards
    if (PyType_Ready(&signal_objectType) < 0)
        return 0;    

    if (PyType_Ready(&signals_objectType) < 0)
        return 0;    

    // generate guid
    gchar* c = g_dbus_generate_guid();
    sid = std::string(c);
    g_free(c);

    // assemble module config
    std::ostringstream oss_send;
    oss_send << dbus_object_path_send_req_prefix << sid;
    dbus_object_path_send_req_path = oss_send.str();

    std::ostringstream oss_recv;
    oss_recv << dbus_object_path_recv_req_prefix << sid;
    dbus_object_path_recv_req_path = oss_recv.str();

    g_print (PROG "Interface; %s.\n", dbus_interface.c_str());
    g_print (PROG "Send; %s.\n", dbus_object_path_send_req_path.c_str());
    g_print (PROG "Recv; %s.\n", dbus_object_path_recv_req_path.c_str());

    // acquire dbus session
    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    // create and populate module
    PyObjectRef m = PyModule_Create(&moduledef);

    m.addString( "uid", sid.c_str());

    m.addObject("SignalObject", &signal_objectType);
    m.addObject("SignalsObject", &signals_objectType);

    add_future_obj_def(m);
    add_future_iter_obj_def(m);    
    add_task_obj_def(m);
    
    PyObjectRef signalsObject = new_signals_object();
    m.addObject("View", signalsObject);

    return m.incr();
}




