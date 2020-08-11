#include "pywebkit.h"
#include "gprop.h"
#include <sstream>
#include <dlfcn.h>
#include <libgen.h>
#include <map>

#define PROG "[PywebkitWebview] "



/**
 * SECTION: pywebkit
 * @short_description: A Pythonized WebKit widget.
 *
 * The #PyWebKit is a Webkit widget derivate enabling JS - Python bridge.
 * 
 */

G_DEFINE_TYPE (PywebkitWebview, pywebkit_webview, WEBKIT_TYPE_WEB_VIEW   )
 
/*
static gboolean on_context_menu (
    WebKitWebView       *web_view,
    WebKitContextMenu   *context_menu,
    GdkEvent            *event,
    WebKitHitTestResult *hit_test_result,
    gpointer             user_data)
{
    return TRUE;
}
*/

static void init_ext(WebKitWebContext *context, gpointer user_data)
{
    if ( !context)
    {
        g_print( PROG "init_ext: WebKitWebContext is null \n" );
        return;
    }
/*    if ( !user_data)
    {
        g_print( PROG "init_ext: user_data is null \n" );
        return;
    }

    PywebkitWebview *web = (PywebkitWebview*) user_data;

    if(!G_IS_OBJECT(web))
    {
        g_print( PROG "init_ext: user_data is not a gobj \n" );
        return;
    }

    if ( !web || !web->uid)
    {
        g_print( PROG "init_ext: web->uid is null \n" );
        return;
    }

    g_print(PROG "init_ext: %s \n", web->uid );
*/
    GVariant* s = g_variant_new_string("uid??");//web->uid);

    std::ostringstream oss;

    Dl_info info;
    char* fn = 0;
    if (dladdr((const void*)&pywebkit_webview_init, &info))
    {
        fn = strdup(info.dli_fname);
        char* dir = dirname(fn);
        oss << dir << "/webkitext";
        free(fn);

        g_print( PROG "loading extensions from path: %s\n", oss.str().c_str() );
    }

    webkit_web_context_set_web_extensions_directory(context,oss.str().c_str());
    webkit_web_context_set_web_extensions_initialization_user_data(context,s);

    //g_object_unref(web);

}

static void on_change(PywebkitWebview* web,const char* value)
{
        g_print( PROG "CHANGE WE CAN BELIEVE IN: %s\n", value );
}

static void pywebkit_webview_init(PywebkitWebview *web)
{
    if ( !web)
    {
        g_print( PROG "pywebkit_webview_init: PywebkitWebview is null \n" );
        return;
    }

    
    gchar* c = g_dbus_generate_guid();
    web->uid = g_strdup (c);
    g_free(c);

    g_print(PROG "pywebkit_webview_init %s\n", web->uid);


    //g_object_ref(web);

    g_signal_connect( G_OBJECT (web), "changed", G_CALLBACK(on_change), web);

    g_signal_emit_by_name(web,"changed","GRRRRR");

    // this would globally disable right click context-menu:
    // g_signal_connect( G_OBJECT (web), "context-menu", G_CALLBACK(on_context_menu), web);
}

static void pywebkit_webview_finalize(GObject *object)
{
    PywebkitWebview* web = (PywebkitWebview*)object;

    g_print(PROG "pywebkit_webview_finalize %s\n", web->uid);
}


static void pywebkit_webview_class_init(PywebkitWebviewClass *klass)
{
    if ( !klass)
    {
        g_print( PROG "pywebkit_webview_class_init: PywebkitWebviewClass is null \n" );
        return;
    }
    
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = pywebkit_webview_finalize;

    // GObject properties
    gprops<PywebkitWebviewClass> pywebkitWebviewProperties{
        gprop( &PywebkitWebview::dummy, g_param_spec_string( "uid", "Uid", "Unique ID", "<uid>", G_PARAM_READWRITE) )
    };

    pywebkitWebviewProperties.install(klass);

    // GObject signals
    Signals signals(object_class);
    signals.install("changed", G_TYPE_STRING );


    WebKitWebContext* ctx = webkit_web_context_get_default();
    g_signal_connect( G_OBJECT (ctx), "initialize-web-extensions", G_CALLBACK(init_ext), NULL);

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


static std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}

/**
 * pywebkit_webview_load_local_uri:
 *
 * @web: #PywebkitWebview*
 * @localpath: #gchar*
 *
 * loads a new HTML document into #PyWebKit.
 *
 */

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