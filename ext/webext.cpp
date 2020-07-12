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
#include "gvglue.h"

#define PROG "[web_extension.so]"

static const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_prefix = "/org/oha7/webkit/WebKitDBus/view/";
static const std::string dbus_object_path_recv_prefix = "/org/oha7/webkit/WebKitDBus/controller/";

static std::string dbus_object_path_send_path;
static std::string dbus_object_path_recv_path; 

std::map<const JSClassDefinition*,JSClassRef> jclass::map_;

static GDBusConnection* dbuscon = 0;
static std::string sid;


struct DBusCallback 
{
    DBusCallback()
    {}

    DBusCallback(jsobj& o)
        : obj(o)
    {
        obj.protect();
    }

    ~DBusCallback()
    {
        obj.unprotect();
    }

    DBusCallback& operator=(const DBusCallback& rhs)
    {
        if( &rhs.obj == &this->obj) 
        {
            return *this;
        }
        
        if( obj.isValid())
        {
            obj.unprotect();
        }

        obj = rhs.obj;

        obj.protect();

        return *this;
    }

    jsobj obj;
};

static DBusCallback cb;


static void signal_handler(
    GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data
    )
{
    std::vector<JSValueRef> arguments = gvariant_to_js_values( cb.obj.ctx(), parameters );

    jsval member = cb.obj.member(signal_name);

    if(member.isUndefined())
    {
        g_print (PROG "unknown signal %s\n", signal_name);
    }
    else 
    {
        jsobj fun = member.obj();

        bool isFunction = fun.isFunction();
        if(!isFunction)
        {
            g_print (PROG " error: %s is not a function \n", signal_name);
        }
        else 
        {
            jsval result = fun.invoke(arguments);
        }
    }
}

static void got_dbus (
    GObject *source_object,
    GAsyncResult *res,
    gpointer user_data
    )
{
    dbuscon =  g_bus_get_finish (res, NULL);    

    sid = g_dbus_connection_signal_subscribe (
        dbuscon, 
        /*sender*/ NULL, 
        dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
        dbus_object_path_recv_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL
    );
}

static JSValueRef send_signal(
    JSContextRef ctx, 
    JSObjectRef function, 
    JSObjectRef thisObject, 
    size_t argumentCount, 
    const JSValueRef arguments[], 
    JSValueRef* exception
    ) 
{
    jsctx js(ctx);

    if(argumentCount<1) 
    {
        return js.undefined();
    }

    std::string signal = jstr(ctx,arguments[0]).str();

    g_print (PROG " send_signal: %s \n", signal.c_str());

    gvar_builder builder = gtuple();

    for( size_t i = 1; i < argumentCount; i++)
    {
        jsval arg(ctx,arguments[i]);
        GVariant* v = make_variant(arg);
        builder.add(v);
    }

    GVariant* params = builder.build();

    g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        dbus_object_path_send_path.c_str(),
        dbus_interface.c_str(),
        signal.c_str(),
        params,
        NULL
    );    

    return js.undefined();
}


static JSValueRef bind_signals(
    JSContextRef ctx, 
    JSObjectRef function, 
    JSObjectRef thisObject, 
    size_t argumentCount, 
    const JSValueRef arguments[], 
    JSValueRef* exception
    ) 
{
    jsctx js(ctx);

    if(argumentCount<1) 
    {
        g_print (PROG " error: less than 1 args \n");
        return js.undefined();   
    }

    jsval arg(ctx,arguments[0]);
    if(!arg.isObject())
    {
        g_print (PROG " error: first arg is not a obj \n");
       return js.undefined();
    }    

    jsobj obj = arg.obj();

    cb = DBusCallback(obj);

    g_print (PROG " bind signals \n");

    return js.undefined();
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
    jsctx js(ctx);
    jsobj fun(ctx,function);

    gchar* signal_name = (gchar*)fun.private_data();

    g_print (PROG " send_signal: %s \n", signal_name);

    gvar_builder builder = gtuple();

    for( size_t i = 0; i < argumentCount; i++)
    {
        jsval arg(ctx,arguments[i]);
        GVariant* v = make_variant(arg);
        builder.add(v);
    }

    GVariant* params = builder.build();

    g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        dbus_object_path_send_path.c_str(),
        dbus_interface.c_str(),
        signal_name,
        params,
        NULL
    );    

    return js.undefined();
}

static void Signal_object_class_finalize_cb(JSObjectRef object)
{
    gchar* c = (gchar*)JSObjectGetPrivate(object);
    g_free(c);
}


static void Signal_object_class_init_cb(JSContextRef ctx,JSObjectRef object)
{
}

static const JSClassDefinition Signal_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "Signal",              // className
    NULL,                  // parentClass
    NULL,                  // staticValues
    NULL,                  // staticFunctions
    Signal_object_class_init_cb, // initialize
    Signal_object_class_finalize_cb, // finalize
    NULL,                  // hasProperty
    NULL,                  // getProperty
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

    return jclass(&Signal_class_def).create(context,name);
}


static const JSClassDefinition Controller_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "Controller",          // className
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
    g_print (PROG " Page created\n");

    WebKitFrame * frame = webkit_web_page_get_main_frame(web_page);
    JSGlobalContextRef ctx = webkit_frame_get_javascript_global_context(frame);
    jsctx js(ctx);

    jsobj global = js.globalObject();

    jstr sendSignalFunctionName("send_signal");
    jsobj sendSignalFunction = js.make_function(sendSignalFunctionName.ref(), &send_signal);

    jstr bindSignalsFunctionName("bind");
    jsobj bindSignalsFunction = js.make_function(bindSignalsFunctionName.ref(), &bind_signals);

    jsobj WebKitDBus = js.object();

    WebKitDBus.set(sendSignalFunctionName.ref(), sendSignalFunction.ref());
    WebKitDBus.set(bindSignalsFunctionName.ref(), bindSignalsFunction.ref());

    jsobj controller = js.object(Controller_class_def);
    WebKitDBus.set("Controller",controller.ref());
     
    global.set("WebKit", WebKitDBus.ref());
}

// ======================================================================================================
/**
 * Entry point function.
 * when WebKit scans the directories for shared libraries (.so), it looks for this function.
 * If it's absent the .so file is ignored.
*/

G_MODULE_EXPORT void webkit_web_extension_initialize_with_user_data (WebKitWebExtension *extension,   GVariant *user_data)
{
    gvar data(user_data);
    sid = std::string(data.str());

    g_print(PROG " PLUGIN activated %s\n", sid.c_str());

    std::ostringstream oss_send;
    oss_send << dbus_object_path_send_prefix << sid;
    dbus_object_path_send_path = oss_send.str();

    std::ostringstream oss_recv;
    oss_recv << dbus_object_path_recv_prefix << sid;
    dbus_object_path_recv_path = oss_recv.str();

    g_print (PROG "Interface: %s.\n", dbus_interface.c_str());
    g_print (PROG "Send: %s.\n", dbus_object_path_send_path.c_str());
    g_print (PROG "Recv: %s.\n", dbus_object_path_recv_path.c_str());
    
    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    g_signal_connect (extension, "page-created", G_CALLBACK (web_page_created_cb),  NULL);
}

} // extern C
