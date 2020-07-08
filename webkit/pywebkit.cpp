#include "pywebkit.h"
#include <iostream>
#include <Python.h>


/**
 * SECTION: pywebkit
 * @short_description: A Pythonized WebKit widget.
 *
 * The #PyWebKit is a Webkit widget derivate enabling JS - Python bridge.
 * 
 */

G_DEFINE_TYPE (PyWebKit, py_webkit, WEBKIT_TYPE_WEB_VIEW   )


static void init_ext (WebKitWebContext *context, gpointer user_data)
{
    webkit_web_context_set_web_extensions_directory(context,"./ext/build");
}


static void py_webkit_init (PyWebKit *web)
{
    web->uid = "123456789";

    PyObject* n = PyUnicode_FromString("pywebkit");
    PyObject* m = PyImport_GetModule(n);

    PyModule_AddStringConstant(m, "uid", web->uid);

    Py_XDECREF(n);
    Py_XDECREF(m);

    WebKitWebContext* ctx = webkit_web_context_get_default ();

    g_signal_connect(G_OBJECT (ctx), "initialize-web-extensions", G_CALLBACK(init_ext), NULL);
}

static void py_webkit_finalize (GObject *object)
{
}



static void py_webkit_class_init (PyWebKitClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = py_webkit_finalize;
}

// could have params like this:
// * @module: #gchar*
// * @klass: #gchar*

/**
 * py_webkit_new:
 *
 * Allocates a new #PyWebKit.
 *
 * Return value: a new #PyWebKit.
 */
PyWebKit* py_webkit_new ()
{
    PyWebKit *web;

    web = (PyWebKit*)g_object_new (PY_WEBKIT_TYPE, NULL);
    return web;
}
