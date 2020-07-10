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
#include <sstream>
#include "marshal.h"

std::map<const JSClassDefinition*,JSClassRef> jclass::map_;

static const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_prefix = "/org/oha7/webkit/WebKitDBus/view/";
static const std::string dbus_object_path_recv_prefix = "/org/oha7/webkit/WebKitDBus/controller/";

static std::string dbus_object_path_send_path;
static std::string dbus_object_path_recv_path; 

#define PROG "[web_extension.so]"

static GDBusConnection* dbuscon = 0;
std::string sid;

struct DBusCallback 
{
    DBusCallback()
        : func(0), context(0)
    {}

    DBusCallback(JSObjectRef f, JSContextRef ctx)
        : func(f), context(ctx)
    {
        JSValueProtect(ctx,f);
    }

    ~DBusCallback()
    {
         JSValueUnprotect(context,func);
    }

    DBusCallback& operator=(const DBusCallback& rhs)
    {
        if( &rhs == this) 
        {
            return *this;
        }
        
        if(context && func)
        {
             JSValueUnprotect(context,func);
        }

        context = rhs.context;
        func = rhs.func;
        
        if(context && func)
        {
             JSValueProtect(context,func);
        }
        return *this;
    }


    JSObjectRef  func;
    JSContextRef context;    
};

//std::map<std::string,std::unique_ptr<DBusCallback>> signalMap;

static DBusCallback cb;


struct SignalHandlerData
{
    std::string signal_name;
    GVariant *parameters;
};

int  main_thread_signal_handler(void* data )
{
    SignalHandlerData* shd = (SignalHandlerData*)data;
    //DBusCallback* cb = signalMap[shd->signal_name].get();

    std::vector<JSValueRef> arguments = gvariant_to_js_values(cb.context,shd->parameters);

    g_print (PROG " n params %ld \n", arguments.size() ); 
    
    JSValueRef ex = 0;

    jstr name(shd->signal_name.c_str());

    JSValueRef member = JSObjectGetProperty(cb.context,cb.func,name.ref(),&ex);
    if ( JSValueIsUndefined(cb.context,member))
    {
        g_print (PROG "unknown signal %s\n", shd->signal_name.c_str());
    }
    else 
    {
        JSObjectRef fun = JSValueToObject(cb.context, member, &ex);

        bool isFunction = JSObjectIsFunction(cb.context, fun);
        if(!isFunction)
        {
            g_print (PROG " error: first arg is not a function \n");
        }
        else 
        {
            JSValueRef result = JSObjectCallAsFunction(
                cb.context,
                fun,
                NULL,  
                arguments.size(), 
                &arguments[0], 
                &ex
            );
        }
    }

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
/*
    if(signalMap.count(signal_name) == 0)
    {
        g_print (PROG "unknown signal %s\n", signal_name);
        return;
    }
*/
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
        //"com.example.TestService",
        dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
//        "/com/example/TestService/view",
        dbus_object_path_recv_path.c_str(),
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
        dbus_object_path_send_path.c_str(),
        dbus_interface.c_str(),
       // "/com/example/TestService/object",
       // "com.example.TestService",
        signal.c_str(),
        params,
        NULL
    );    

    return JSValueMakeUndefined(ctx);
}


static JSValueRef bind_signals(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) 
{
    if(argumentCount<1) 
    {
        g_print (PROG " error: less than 1 args \n");
        return JSValueMakeUndefined(ctx);        
    }

    JSType jt = JSValueGetType(ctx, arguments[0]);
    if ( jt !=  kJSTypeObject )
    {
        g_print (PROG " error: first arg is not a obj \n");
        return JSValueMakeUndefined(ctx); 
    }    

    JSValueRef   ex = 0;
    JSObjectRef obj = JSValueToObject(ctx, arguments[0], &ex);

/*
    bool isFunction = JSObjectIsFunction(ctx, obj);
    if(!isFunction)
    {
        g_print (PROG " error: first arg is not a function \n");
        return JSValueMakeUndefined(ctx); 
    }
*/
    cb = DBusCallback(obj,ctx);

//    DBusCallback* cb = new DBusCallback(obj,ctx);

  //  signalMap[signal] = std::unique_ptr<DBusCallback>(cb);

    g_print (PROG " on_signal \n");

    return JSValueMakeUndefined(ctx);
}
///////////////////////////////////////////////////


static JSValueRef Signal_callAsFunctionCallback(
                JSContextRef ctx, 
                JSObjectRef function, 
                JSObjectRef thisObject, 
                size_t argumentCount, 
                const JSValueRef arguments[], 
                JSValueRef* exception)
{
    gchar* signal_name = (gchar*)JSObjectGetPrivate(function);

    g_print (PROG " send_signal: %s \n", signal_name);

    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
    for( size_t i = 0; i < argumentCount; i++)
    {
        GVariant* v = make_variant(ctx,arguments[i]);
        g_variant_builder_add(builder, "v", v);
    }
    GVariant* params = g_variant_builder_end(builder);

    g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        dbus_object_path_send_path.c_str(),
        dbus_interface.c_str(),
       // "/com/example/TestService/object",
       // "com.example.TestService",
        signal_name,
        params,
        NULL
    );    

    return JSValueMakeUndefined(ctx);    
}

static void Signal_object_class_finalize_cb(JSObjectRef object)
{
    gchar* c = (gchar*)JSObjectGetPrivate(object);
    g_print (PROG " Signal_object_class_finalize_cb: %s \n", c);
    g_free(c);
}


static void Signal_object_class_init_cb(JSContextRef ctx,JSObjectRef object)
{
    gchar* c = (gchar*)JSObjectGetPrivate(object);
    g_print (PROG " Signal_object_class_init_cb: %s \n", c);
}

static const JSClassDefinition Signal_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "Signal",              // className
    NULL,                  // parentClass
    NULL,                  // staticValues
    NULL,                  // staticFunctions
    Signal_object_class_init_cb,                  // initialize
    Signal_object_class_finalize_cb,                  // finalize
    NULL,   // hasProperty
    NULL,   // getProperty
    NULL,                  // setProperty
    NULL,                  // deleteProperty
    NULL,                  // getPropertyNames
    Signal_callAsFunctionCallback, // callAsFunction
    NULL,                  // callAsConstructor
    NULL,                  // hasInstance  
    NULL                   // convertToType
};


/////////////////////////////////////////////////////////////

static bool Controller_hasPropertyCallback (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    jstr str(propertyName);
    std::string s(str.str());
    if ( s == "toString" || s == "valueOf"|| s == "toJSON") 
    {
       return false;
    }
    return true;
}


static JSValueRef Controller_getPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    jstr str(propertyName);

    gchar* name = g_strdup(str.str());

    g_print (PROG " get send_signal: %s \n", name);

    return jclass(&Signal_class_def).create(context,name);
}


static const JSClassDefinition Controller_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "Controller",              // className
    NULL,                  // parentClass
    NULL,                  // staticValues
    NULL,                  // staticFunctions
    NULL,                  // initialize
    NULL,                  // finalize
    Controller_hasPropertyCallback,   // hasProperty
    Controller_getPropertyCallback,   // getProperty
    NULL,                  // setProperty
    NULL,                  // deleteProperty
    NULL,                  // getPropertyNames
    NULL,                  // callAsFunction
    NULL,                  // callAsConstructor
    NULL,                  // hasInstance  
    NULL                   // convertToType
};


////////////////////////////////////////////////////////

extern "C" {

static void web_page_created_cb (WebKitWebExtension *extension, WebKitWebPage *web_page, gpointer user_data)
{
    g_print (PROG " SIGNAL: Page created\n");

    WebKitFrame * frame = webkit_web_page_get_main_frame(web_page);
    JSGlobalContextRef jsctx = webkit_frame_get_javascript_global_context  (frame);
    JSObjectRef globalObject = JSContextGetGlobalObject(jsctx);

    jstr sendSignalFunctionName("send_signal");
    JSObjectRef sendSignalFunctionObject = JSObjectMakeFunctionWithCallback(jsctx, sendSignalFunctionName.ref(), &send_signal);

    jstr bindSignalsName("bind");
    JSObjectRef bindSignalsunctionObject = JSObjectMakeFunctionWithCallback(jsctx, bindSignalsName.ref(), &bind_signals);

    JSObjectRef WebKitDBus = JSObjectMake(jsctx,0,NULL);
    JSObjectSetProperty(jsctx, WebKitDBus, bindSignalsName.ref(), bindSignalsunctionObject, kJSPropertyAttributeNone, nullptr);
    JSObjectSetProperty(jsctx, WebKitDBus, sendSignalFunctionName.ref(), sendSignalFunctionObject, kJSPropertyAttributeNone, nullptr);

    JSObjectSetProperty(
        jsctx, 
        WebKitDBus, 
        jstr("Controller").ref(), 
        jclass(&Controller_class_def).create(jsctx,0), 
        kJSPropertyAttributeNone, 
        NULL
    );

    jstr WebKitDBusName("WebKitDBus");
    JSObjectSetProperty(jsctx, globalObject, WebKitDBusName.ref(), WebKitDBus, kJSPropertyAttributeNone, nullptr);

}

// ======================================================================================================
/**
 * Entry point function.
 * when WebKit scans the directories for shared libraries (.so), it looks for this function.
 * If it's absent the .so file is ignored.
*/

G_MODULE_EXPORT void webkit_web_extension_initialize_with_user_data (WebKitWebExtension *extension,   GVariant *user_data)
{
        g_print(PROG "KILLROYY %i\n", (void*)user_data);

 g_print (PROG " webkit_web_extension_initialize: %s \n", g_variant_get_type_string((GVariant*)user_data));

    const gchar* c = g_variant_get_string((GVariant*)user_data,NULL);

        g_print(PROG "KILLROYY %s\n",c);

        sid = std::string(c);

    g_print(PROG " PLUGIN activated %s\n", sid.c_str());


    std::ostringstream oss_send;
    oss_send << dbus_object_path_send_prefix << sid;
    dbus_object_path_send_path = oss_send.str();

    std::ostringstream oss_recv;
    oss_recv << dbus_object_path_recv_prefix << sid;
    dbus_object_path_recv_path = oss_recv.str();

    g_print (PROG "Interface; %s.\n", dbus_interface.c_str());
    g_print (PROG "Send; %s.\n", dbus_object_path_send_path.c_str());
    g_print (PROG "Recv; %s.\n", dbus_object_path_recv_path.c_str());
    
    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    g_signal_connect (extension, "page-created", G_CALLBACK (web_page_created_cb),  NULL);
}

} // extern C
