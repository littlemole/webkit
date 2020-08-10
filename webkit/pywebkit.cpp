#include "pywebkit.h"
#include <sstream>
#include <string>
#include <dlfcn.h>
#include <libgen.h>

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
    PywebkitWebview *web = (PywebkitWebview*) user_data;

    GVariant* s = g_variant_new_string(web->uid);

    g_print(PROG "init_ext: %s \n", web->uid );

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
}

static void pywebkit_webview_init(PywebkitWebview *web)
{
    g_print(PROG "pywebkit_webview_init\n");

    gchar* c = g_dbus_generate_guid();
    web->uid = g_strdup (c);
    g_free(c);


    WebKitWebContext* ctx = webkit_web_context_get_default();
    g_signal_connect( G_OBJECT (ctx), "initialize-web-extensions", G_CALLBACK(init_ext), web);

    // this would globally disable right click context-menu:
    // g_signal_connect( G_OBJECT (web), "context-menu", G_CALLBACK(on_context_menu), web);
}

static void pywebkit_webview_finalize(GObject *object)
{
}

typedef enum
{
  PROP_UID = 1,
  N_PROPERTIES
} WebProperties;

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void pywebkit_webview_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PywebkitWebview *self = (PywebkitWebview*) (object);

  switch ((WebProperties) property_id)
    {
    case PROP_UID:
      g_free (self->uid);
      self->uid = g_value_dup_string (value);
      g_print ("uid: %s\n", self->uid);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void pywebkit_webview_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PywebkitWebview *self = (PywebkitWebview*) (object);

  switch ((WebProperties) property_id)
    {
    case PROP_UID:
      g_value_set_string (value, self->uid);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void pywebkit_webview_class_init(PywebkitWebviewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = pywebkit_webview_finalize;

    object_class->set_property = pywebkit_webview_set_property;
    object_class->get_property = pywebkit_webview_get_property;

    obj_properties[PROP_UID] =
    g_param_spec_string ("uid",
                         "Uid",
                         "unique id.",
                         "<uid>"  /* default value */,
                         G_PARAM_READWRITE);


   g_object_class_install_properties(object_class,
                                     N_PROPERTIES,
                                     obj_properties);
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