/**
  * Minimal example of WebKit2 Web Extension, using C.
  *
  * [Using Linux, start a shell]
  * Compile this code using the Makefile (type the command 'make'),
  * ignore the warnings about unused parameters, they are normal.
  *
  * This will generate "bin/Release/webkit2_web_extension.so".
  * Copy this file in one of the GUI examples directories and start the GUI
  * example from the console: if the web-extension works,
  * you should see messages in the console.
  *
  * The make file is generated using "cbp2make.linux-x86_64" on a Code::Blocks project file.
  */
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit-web-extension.h>
#include <glib.h>
#include <string>
#include <vector>
#include <map>
#include <gio/gio.h>
#include <memory>
#include "marshal.h"

#define PROG "[web_extension.so]"

static GDBusConnection* dbuscon = 0;
guint sid = 0;

struct DBusCallback 
{
    DBusCallback(JSObjectRef f, JSContextRef ctx)
        : func(f), context(ctx)
    {
        JSValueProtect(ctx,f);
    }

    ~DBusCallback()
    {
         JSValueProtect(context,func);
    }

    JSObjectRef  func;
    JSContextRef context;    
};

std::map<std::string,std::unique_ptr<DBusCallback>> signalMap;


struct SignalHandlerData
{
    std::string signal_name;
    GVariant *parameters;
};

int  main_thread_signal_handler(void* data )
{
    SignalHandlerData* shd = (SignalHandlerData*)data;
    DBusCallback* cb = signalMap[shd->signal_name].get();

    std::vector<JSValueRef> arguments = gvariant_to_js_values(cb->context,shd->parameters);

    g_print (PROG " n params %ld \n", arguments.size() ); 

    JSValueRef ex = 0;
    JSValueRef result = JSObjectCallAsFunction(
        cb->context,
        cb->func,
        NULL,  
        arguments.size(), 
        &arguments[0], 
        &ex
    );

    g_variant_unref(shd->parameters);

    delete shd;
    return 0;
}

static void signal_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    //g_print (PROG " received signal %s %s\n", signal_name, g_variant_get_type_string (parameters));

    if(signalMap.count(signal_name) == 0)
    {
        g_print (PROG "unknown signal %s\n", signal_name);
        return;
    }

    gsize s = g_variant_n_children (parameters);
    for( gsize i = 0; i < s; i++) 
    {
        GVariant* gv = g_variant_get_child_value (parameters,i);
        GString* gs = g_variant_print_string (gv, NULL,TRUE);

        //g_print (PROG " n params %s %s\n", g_variant_get_type_string (gv) ,gs->str );

        g_string_free (gs,TRUE);
        g_variant_unref(gv);
    }

    SignalHandlerData* shd = new SignalHandlerData;
    shd->signal_name = signal_name;
    shd->parameters = parameters;

    g_variant_ref(parameters);

    gdk_threads_add_idle ( &main_thread_signal_handler, shd);
}

static void got_dbus (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
    dbuscon =  g_bus_get_finish (res, NULL);    

    sid = g_dbus_connection_signal_subscribe (
        dbuscon, 
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

static JSValueRef send_signal(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) 
{
    if(argumentCount<1) 
    {
        return JSValueMakeUndefined(ctx);        
    }

    std::string signal = jstr(ctx,arguments[0]).str();

    g_print (PROG " send_signal: %s \n", signal.c_str());

    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
    for( size_t i = 1; i < argumentCount; i++)
    {
        GVariant* v = make_variant(ctx,arguments[i]);
        g_variant_builder_add(builder, "v", v);
    }
    GVariant* params = g_variant_builder_end(builder);

    g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        "/com/example/TestService/object",
        "com.example.TestService",
        signal.c_str(),
        params,
        NULL
    );    

    return JSValueMakeUndefined(ctx);
}


static JSValueRef on_signal(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) 
{
    if(argumentCount<2) 
    {
        g_print (PROG " error: less than 2 args \n");
        return JSValueMakeUndefined(ctx);        
    }

    JSType jt = JSValueGetType(ctx, arguments[0]);
    if ( jt !=  kJSTypeString )
    {
        g_print (PROG " error: first arg is not a string \n");
        return JSValueMakeUndefined(ctx); 
    }    

    std::string signal = jstr(ctx,arguments[0]).str();

    jt = JSValueGetType(ctx, arguments[1]);
    if ( jt !=  kJSTypeObject )
    {
        g_print (PROG " error: second arg is not a obj \n");
        return JSValueMakeUndefined(ctx); 
    }    

    JSValueRef   ex = 0;
    JSObjectRef obj = JSValueToObject(ctx, arguments[1], &ex);

    bool isFunction = JSObjectIsFunction(ctx, obj);
    if(!isFunction)
    {
        g_print (PROG " error: second arg is not a function \n");
        return JSValueMakeUndefined(ctx); 
    }

    DBusCallback* cb = new DBusCallback(obj,ctx);

    signalMap[signal] = std::unique_ptr<DBusCallback>(cb);

    g_print (PROG " on_signal: %s \n", signal.c_str());

    return JSValueMakeUndefined(ctx);
}

extern "C" {

static void web_page_created_cb (WebKitWebExtension *extension, WebKitWebPage *web_page, gpointer user_data)
{
    g_print (PROG " SIGNAL: Page created\n");

    WebKitFrame * frame = webkit_web_page_get_main_frame(web_page);
    JSGlobalContextRef jsctx = webkit_frame_get_javascript_global_context  (frame);
    JSObjectRef globalObject = JSContextGetGlobalObject(jsctx);

    JSStringRef sendSignalFunctionName = JSStringCreateWithUTF8CString("send_signal");
    JSObjectRef sendSignalFunctionObject = JSObjectMakeFunctionWithCallback(jsctx, sendSignalFunctionName, &send_signal);
    JSObjectSetProperty(jsctx, globalObject, sendSignalFunctionName, sendSignalFunctionObject, kJSPropertyAttributeNone, nullptr);

    JSStringRef onSignalName = JSStringCreateWithUTF8CString("on_signal");
    JSObjectRef onsSignalfunctionObject = JSObjectMakeFunctionWithCallback(jsctx, onSignalName, &on_signal);
    JSObjectSetProperty(jsctx, globalObject, onSignalName, onsSignalfunctionObject, kJSPropertyAttributeNone, nullptr);
}

// ======================================================================================================
/**
 * Entry point function.
 * when WebKit scans the directories for shared libraries (.so), it looks for this function.
 * If it's absent the .so file is ignored.
*/
G_MODULE_EXPORT void webkit_web_extension_initialize (WebKitWebExtension *extension)
{
    g_print(PROG " PLUGIN activated\n");

    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    g_signal_connect (extension, "page-created", G_CALLBACK (web_page_created_cb),  NULL);
}

} // extern C
