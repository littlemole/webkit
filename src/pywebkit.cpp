#include "pywebkit.h"

extern "C" void set_pywebkit_python_global(gpointer context, gpointer user_data);
extern "C" void declare_jswrapper_python_object();

/**
 * SECTION: pywebkit
 * @short_description: A Pythonized WebKit widget.
 *
 * The #PyWebKit is a Webkit widget derivate enabling JS - Python bridge.
 * 
 */

G_DEFINE_TYPE (PyWebKit, py_webkit, WEBKIT_TYPE_WEB_VIEW   )


/* Callback - JavaScript window object has been cleared */
static void window_object_cleared_cb(
                WebKitWebView  *web_view,
                WebKitWebFrame *frame,
                gpointer        context,
                gpointer        window_object,
                gpointer        user_data)

{
    set_pywebkit_python_global(context,user_data);
}


static void py_webkit_init (PyWebKit *object)
{
    /* Connect the window object cleared event with callback */
    g_signal_connect(
        G_OBJECT (object), 
        "window-object-cleared", 
        G_CALLBACK(window_object_cleared_cb), 
        object
    );
}

static void py_webkit_finalize (GObject *object)
{
}



static void py_webkit_class_init (PyWebKitClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = py_webkit_finalize;
    declare_jswrapper_python_object();
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

