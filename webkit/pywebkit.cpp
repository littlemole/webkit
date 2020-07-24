#include "pywebkit.h"
#include <iostream>
#include <sstream>
#include <Python.h>
#include "pyglue.h"
#include <string>
#include <unistd.h>
#include <limits.h>

#define PROG "[libwebview] "

/**
 * SECTION: pywebkit
 * @short_description: A Pythonized WebKit widget.
 *
 * The #PyWebKit is a Webkit widget derivate enabling JS - Python bridge.
 * 
 */

G_DEFINE_TYPE (PywebkitWebview, pywebkit_webview, WEBKIT_TYPE_WEB_VIEW   )
 

static gboolean on_context_menu (
    WebKitWebView       *web_view,
    WebKitContextMenu   *context_menu,
    GdkEvent            *event,
    WebKitHitTestResult *hit_test_result,
    gpointer             user_data)
{
    return TRUE;
}

static void init_ext(WebKitWebContext *context, gpointer user_data)
{
    PywebkitWebview *web = (PywebkitWebview*) user_data;

    GVariant* s = g_variant_new_string(web->uid);

    g_print(PROG "init_ext: %s \n", web->uid );

    webkit_web_context_set_web_extensions_directory(context,"./ext/build");
    webkit_web_context_set_web_extensions_initialization_user_data(context,s);
}


static void pywebkit_webview_init(PywebkitWebview *web)
{
    g_print(PROG "pywebkit_webview_init\n");

    // generate guid
    gchar* c = g_dbus_generate_guid();
    //sid = std::string(c);
    web->uid = g_strdup (c);
    g_free(c);

//    web->uid = "dummy uid"

    pyobj_ref n = PyUnicode_FromString("pygtk.WebKitDBus");
    pyobj_ref m = PyImport_GetModule(n);

    if(m.isValid())
    {
        g_print(PROG "lookup cb \n");
        //pyobj_ref cb = PyObject_GetAttrString(m,"callback");
        pyobj_ref cb = pyobj(PyModule_GetDict(m)).member("callback");
         PyObject_Print(cb, stdout, Py_PRINT_RAW);
        printf("\n");

        if( pyobj(cb).isValid() && !pyobj(cb).isNone() )
        {
            g_print(PROG "bind controller cb\n");
            pyobj_ref bind = pyobj(m).attr("bind");
         PyObject_Print(bind, stdout, Py_PRINT_RAW);
        printf("\n");
            pyobj_ref str = PyUnicode_FromString(web->uid);
            pyobj_ref r = pyobj(bind).call( str.ref(), cb.ref());

            if(py_error())
            {
                PyErr_Print();
            }              
        }

        //const char* c = PyUnicode_AsUTF8(uid);
        //web->uid = g_strdup (c);

    }

    WebKitWebContext* ctx = webkit_web_context_get_default();
    g_signal_connect( G_OBJECT (ctx), "initialize-web-extensions", G_CALLBACK(init_ext), web);

    // this would globally disable right click context-menu:
    // g_signal_connect( G_OBJECT (web), "context-menu", G_CALLBACK(on_context_menu), web);
}

static void pywebkit_webview_finalize(GObject *object)
{
}



static void pywebkit_webview_class_init(PywebkitWebviewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = pywebkit_webview_finalize;
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

PywebkitWebview* pywebkit_webview_new()
{
    PywebkitWebview *web;

    web = (PywebkitWebview*)g_object_new (PY_WEBKIT_TYPE, NULL);
    return web;
}


std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}


void pywebkit_webview_load_local_uri(PywebkitWebview *web, const gchar* localpath)
{
    const char* lp = localpath ? localpath : "index.html";

    std::string fp;

    if(lp && lp[0] == '/')
    {
        std::ostringstream oss;
        oss << "file://" << lp;
        fp = oss.str();
    }
    else
    {
        std::ostringstream oss;
        oss << "file://" << cwd() << "/" << lp;
        fp = oss.str();
    }

    webkit_web_view_load_uri( (WebKitWebView*)web, fp.c_str() );
}