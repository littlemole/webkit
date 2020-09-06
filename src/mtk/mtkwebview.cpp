#include "mtk/mtkwebview.h"
#include "glue/gprop.h"
#include <sstream>
#include <dlfcn.h>
#include <libgen.h>
#include <map>

#define PROG "[MtkWebView] "


static std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}


/**
 * SECTION: MtkWebView
 * @short_description: A  WebKit widget.
 *
 * 
 */

G_DEFINE_TYPE (MtkWebView, mtk_webview, WEBKIT_TYPE_WEB_VIEW   )
 


static void init_ext(WebKitWebContext *context, gpointer user_data)
{
    if ( !context)
    {
        g_print( PROG "init_ext: WebKitWebContext is null \n" );
        return;
    }

    GVariant* s = g_variant_new_string("uid??");

    std::ostringstream oss;

    Dl_info info;
    char* fn = 0;
    if (dladdr((const void*)&mtk_webview_init, &info))
    {
        fn = strdup(info.dli_fname);
        char* dir = dirname(fn);
        oss << dir << "/mtkext";
        free(fn);

        g_print( PROG "loading extensions from path: %s\n", oss.str().c_str() );
    }

    webkit_web_context_set_web_extensions_directory(context,oss.str().c_str());
    webkit_web_context_set_web_extensions_initialization_user_data(context,s);
}

static void on_change(MtkWebView* web,const char* value)
{
    g_print( PROG "CHANGE WE CAN BELIEVE IN: %s\n", value );
}

static void on_notify(MtkWebView* web,GParamSpec *pspec, void* user_data)
{
    g_print( PROG "NOTIFY prop changed: %s %s\n", pspec->name, web->localpath );

    mtk_webview_load_local_uri(web,web->localpath);
}

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

static void mtk_webview_init(MtkWebView *web)
{
    if ( !web)
    {
        g_print( PROG "mtk_webview_init: MtkWebView is null \n" );
        return;
    }

    gchar* c = g_dbus_generate_guid();
    web->uid = g_strdup (c);
    g_free(c);

    g_print(PROG "mtk_webview_init %s\n", web->uid);

    g_signal_connect( G_OBJECT (web), "changed", G_CALLBACK(on_change), web);

    g_signal_connect( G_OBJECT (web), "notify::local", G_CALLBACK(on_notify), web);

    g_signal_emit_by_name(web,"changed","GRRRRR");

    // this would globally disable right click context-menu:
    // g_signal_connect( G_OBJECT (web), "context-menu", G_CALLBACK(on_context_menu), web);
}
 
static void mtk_webview_finalize(GObject *object)
{
    MtkWebView* web = (MtkWebView*)object;

    g_print(PROG "mtk_webview_finalize %s\n", web->uid);

    g_free(web->uid);
    g_free(web->localpath);
}


static void mtk_webview_class_init(MtkWebViewClass *klass)
{
    if ( !klass)
    {
        g_print( PROG "mtk_webview_class_init: MtkWebViewClass is null \n" );
        return;
    }

    std::string d = cwd();
    klass->dir = g_strdup(d.c_str());
    
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = mtk_webview_finalize;

    // GObject properties
    gprops<MtkWebViewClass> pywebkitWebviewProperties{
        gprop( &MtkWebView::localpath, g_param_spec_string( "local", "Local", "Local file", "index.html", G_PARAM_READWRITE) )
    };

    pywebkitWebviewProperties.install(klass);

    // GObject signals
    Signals signals(klass);
    signals.install("changed", G_TYPE_STRING );


    WebKitWebContext* ctx = webkit_web_context_get_default();
    g_signal_connect( G_OBJECT (ctx), "initialize-web-extensions", G_CALLBACK(init_ext), klass);
 
}

// could have params like this:
// * @module: #gchar*
// * @klass: #gchar*



MtkWebView* mtk_webview_new()
{
    MtkWebView *web;

    web = (MtkWebView*)g_object_new (MTK_WEBVIEW_TYPE, NULL);
    return web;
}


void mtk_webview_load_local_uri(MtkWebView *web, const gchar* localpath)
{
    const char* lp = localpath ? localpath : "index.html";
 
    MtkWebViewClass* clazz = MTK_WEBVIEW_GET_CLASS(web) ;
    gchar* dir = clazz->dir;

    std::string fp = lp;

    // has protocol?
    size_t pos = fp.find("://");
    if( pos != std::string::npos)
    {
        webkit_web_view_load_uri( (WebKitWebView*)web, fp.c_str() );
        return;
    }

    // is absolute?
    if(lp && lp[0] == '/')
    {
        std::ostringstream oss;
        oss << "file://" << lp;
        fp = oss.str();
    }
    else // treat as local relative path
    {
        std::ostringstream oss;
        if(dir)
        {
            oss << "file://" << dir << "/" << lp;
        }
        else
        {
            oss << "file://" << cwd() << "/" << lp;
        }
        fp = oss.str();
    }

    g_print( PROG "LOAD URL %s %s\n", fp.c_str(), dir  );

    webkit_web_view_load_uri( (WebKitWebView*)web, fp.c_str() );
}

void  mtk_webview_class_set_dir(MtkWebViewClass* clazz, const gchar* dir)
{
    g_free(clazz->dir);
    clazz->dir = g_strdup(dir);
}
