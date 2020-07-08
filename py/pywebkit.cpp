#include "pyglue.h"
#include "marshal.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <gio/gio.h>
#include <glib.h>

#define PROG "[pydbus]"

static GDBusConnection* dbus = 0;
static guint sid = 0;

struct DBusCallback 
{
    DBusCallback(PyObject* f)
        : func(f)
    {}

    PyObjectRef  func;
};

std::map<std::string,std::unique_ptr<DBusCallback>> signalMap;


static void signal_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    g_print (PROG " received signal %s %s\n", signal_name, g_variant_get_type_string (parameters));

    if(signalMap.count(signal_name) == 0)
    {
        g_print (PROG " nknwon signal %s\n", signal_name);
        return;
    }
/*
    gsize s = g_variant_n_children (parameters);
    for( gsize i = 0; i < s; i++) 
    {
        GVariant* gv = g_variant_get_child_value (parameters,i);
        GString* gs = g_variant_print_string (gv, NULL,TRUE);

         g_print (PROG " n params %s %s\n", g_variant_get_type_string (gv) ,gs->str );

        g_string_free (gs,TRUE);
        g_variant_unref(gv);
    }
*/

    DBusCallback* cb = signalMap[signal_name].get();

    PyObject* args = gvariant_to_py_value(parameters);

    PyGlobalInterpreterLock lock;
    PyObjectRef ret = PyObject_CallObject(cb->func, args);
}


static void got_dbus (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    dbus =  g_bus_get_finish (res, NULL);    

    sid = g_dbus_connection_signal_subscribe (
        dbus, 
        /*sender*/ NULL, 
        "com.example.TestService",
        /*const gchar *member*/ NULL,
        "/com/example/TestService/object",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL
    );    
}


static void send_dbus_signal( GDBusConnection* dbus, std::string s, PyObject*  msg)
{
    GVariantBuilder *builder;

    builder = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);

    g_variant_builder_add(builder,"v",make_variant(msg));

    GVariant* params = g_variant_builder_end(builder);

    g_dbus_connection_emit_signal(
        dbus,
        NULL,
        "/com/example/TestService/object",
        "com.example.TestService",
        s.c_str(),
        params,
        NULL
    );
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

    send_dbus_signal(dbus,c,msg);

    Py_RETURN_NONE;
}

static PyObject* pywebkit_on_signal(PyObject* self, PyObject* args)
{
    Py_ssize_t len = PySequence_Size(args);

    if(len<2)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than two args for call to on_signal!");
        return NULL;
    }

    PyObjectRef signal = PySequence_GetItem(args,0);

    if( !PyUnicode_Check(signal) ) 
    {
        PyErr_SetString(PyExc_RuntimeError, "first argument to on_signal is not a string (signal_name)");
        return NULL;
    }

    const char* c = PyUnicode_AsUTF8(signal);


    PyObject* handler = PySequence_GetItem(args,1);
    if(!PyCallable_Check(handler))
    {
        PyErr_SetString(PyExc_RuntimeError, "second argument to on_signal is not a stfunctionring (signal_handler)");
        return NULL;
    }

    DBusCallback* cb = new DBusCallback(handler);
    signalMap[c] = std::unique_ptr<DBusCallback>(cb);

    Py_RETURN_NONE;
}

static PyMethodDef pywebkit_module_methods[] = {
    {"send_signal",  pywebkit_send_signal, METH_VARARGS, "Send a signal to webview child process via dbus."},
    {"on_signal",  pywebkit_on_signal, METH_VARARGS, "register listening for signal via dbus."},
    {NULL}  /* Sentinel */
};



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
    pywebkit_module_methods,
    NULL,NULL,NULL,NULL
};

PyMODINIT_FUNC PyInit_pywebkit(void) {

    PyObject* m;

    m = PyModule_Create(&moduledef);

    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    return m;
}




