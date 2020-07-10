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
    PyWebKit *web = (PyWebKit*) user_data;

    //
    //GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);

    GVariant* s = g_variant_new_string(web->uid);
    //GVariant* s = g_variant_new_int32(4711);
    //g_variant_builder_add_value(builder,s);

    //GVariant* v = g_variant_builder_end(builder);
   // g_variant_ref(v);

     g_print ("init_ext: %s \n", g_variant_get_type_string((GVariant*)s));

    g_print("KILLROYY\n");
    webkit_web_context_set_web_extensions_directory(context,"./ext/build");
    g_print("KILLROYY\n");
    webkit_web_context_set_web_extensions_initialization_user_data(context,s);
    g_print("KILLROYY\n");
    //g_variant_unref(v);
}


static void py_webkit_init (PyWebKit *web)
{
    //web->uid = g_dbus_generate_guid();

    g_print("udi: %s\n",web->uid);

    PyObject* n = PyUnicode_FromString("WebKitDBus");
    PyObject* m = PyImport_GetModule(n);

//    PyModule_AddStringConstant(m, "uid", web->uid);

    PyObject* uid = PyObject_GetAttrString(m,"uid");
    const char* c = PyUnicode_AsUTF8(uid);
    web->uid = g_strdup (c);

    Py_XDECREF(n);
    Py_XDECREF(m);
    Py_XDECREF(uid);

    WebKitWebContext* ctx = webkit_web_context_get_default ();

    g_signal_connect(G_OBJECT (ctx), "initialize-web-extensions", G_CALLBACK(init_ext), web);
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
