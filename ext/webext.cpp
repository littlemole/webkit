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
#include "gvglue.h"
#include "jsglue.h"

#define PROG "[web_extension.so]"

static const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_req_prefix = "/org/oha7/webkit/WebKitDBus/view/request/";
static const std::string dbus_object_path_recv_req_prefix = "/org/oha7/webkit/WebKitDBus/controller/request/";
static const std::string dbus_object_path_send_res_prefix = "/org/oha7/webkit/WebKitDBus/view/response/";
static const std::string dbus_object_path_recv_res_prefix = "/org/oha7/webkit/WebKitDBus/controller/response/";

static std::string dbus_object_path_send_req_path;
static std::string dbus_object_path_recv_req_path; 

static std::string dbus_object_path_send_res_path;
static std::string dbus_object_path_recv_res_path; 



static GDBusConnection* dbuscon = 0;
static std::string sid;
static std::string rsid;

struct ResponseData
{
    ResponseData(JSContextRef context, JSObjectRef res, JSObjectRef rej)
        : ctx(context), resolve(res), reject(rej)
    {
        jsobj(ctx,resolve).protect();
        jsobj(ctx,reject).protect();
    }

    ~ResponseData()
    {
        jsobj(ctx,resolve).unprotect();
        jsobj(ctx,reject).unprotect();
    }

    JSContextRef ctx;
    JSObjectRef resolve;
    JSObjectRef reject;
};

class Responses
{
public:

    void add(const char* uid, ResponseData* p)
    {
        pending_.insert( std::make_pair(std::string(uid),p) );

        g_print (PROG "responses add %s\n", uid );
    }

    ResponseData* get(const char* uid)
    {
        g_print (PROG "responses get %s\n", uid );

        if ( pending_.count(std::string(uid)) == 0)
        {
            return 0;
        }
        ResponseData* res = pending_[std::string(uid)];
        pending_.erase(uid);
        return res;
    }

private:

    std::map<std::string,ResponseData*> pending_;
};

Responses& responses()
{
    static Responses r;
    return r;
}


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

static GVariant* make_response(
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex = NULL
    ) 
{
    jsctx js(ctx);

    g_print (PROG " make_response: \n");

    jsobj obj = js.object();

    obj.set("result", js.undefined().ref() );
    obj.set("exception", js.undefined().ref() );

    if(value)
    {
        obj.set("result", value);
    }
    if(ex)
    {
        obj.set("exception", ex);
    }

    std::string json = to_json(ctx,obj.ref());
    GVariant* result = g_variant_new_string(json.c_str());

    return result;
}

static void send_response(
    const gchar* uid,
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex = NULL
    ) 
{
    jsctx js(ctx);

    g_print (PROG " send_response: %s \n", uid);

    gvar_builder builder = gtuple();  

    GVariant* guid = g_variant_new_string(uid);
    builder.add(guid);

    GVariant* data = make_response(ctx,value,ex);
    builder.add(data);

    GVariant* params = builder.build();

    g_print (PROG "send_response cf: %s %s\n", uid, g_variant_get_type_string (params));
    g_print (PROG "%s\n",g_variant_print (params,TRUE));
   
    GError* gerror = 0;

    int r = g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        dbus_object_path_send_res_path.c_str(),
        dbus_interface.c_str(),
        "response",
        params,
        &gerror
    );    

    if(!r && gerror)
    {
        g_print (PROG "send_response error %s\n",gerror->message);
       // g_error_free(gerror);
    }
    else
    {
        g_print (PROG "send_response sent %i\n",r);
    }
}

static void response_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    g_print (PROG " received response %s %s\n", signal_name, g_variant_get_type_string (parameters));

    jsctx js(cb.obj.ctx());

    gvar params(parameters);

    int len = params.length();
    if(len < 2)
    {
        g_print (PROG " received invalid response tuple %s %s\n", signal_name, g_variant_get_type_string (parameters));
        return;
    }

    gvar uid = params.item(0);
    gvar args = params.item(1);

    std::string json = args.str();

    g_print (PROG "response_handler has %s %s \n", uid.str(), json.c_str() );

    jsobj dict = from_json(js.ctx(),json).obj();

    jsval result = js.undefined();
    jsval ex = js.undefined();

    if ( dict.hasMember("result") )
    {
        result = dict.member("result");
    }
    if ( dict.hasMember("exception") )
    {
        ex = jsobj(dict).member("exception");
    }

    ResponseData* response_data = responses().get( uid.str() );
    if(!response_data)
    {
        return;
    }

    std::vector<JSValueRef> v;
    if( !ex.isUndefined() && !ex.isNull() )
    {
        g_print (PROG "response_handler has ex \n" );
        v.push_back(ex.ref());
        jsobj(js.ctx(),response_data->reject).invoke(v);
    }
    else 
    {
        g_print (PROG "response_handler has result \n" );
        v.push_back(result.ref());
        jsobj(js.ctx(),response_data->resolve).invoke(v);
    }

    delete response_data;
}

static void signal_handler(
    GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data
    );

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
        dbus_object_path_recv_req_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL
    );

    rsid = g_dbus_connection_signal_subscribe (
        dbuscon, 
        /*sender*/ NULL, 
        dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
        dbus_object_path_recv_res_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &response_handler,
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
        return js.undefined().ref();
    }

    std::string signal = jsval(ctx,arguments[0]).str();

    g_print (PROG " send_signal: %s \n", signal.c_str());

    gchar* uid = g_dbus_generate_guid();
    GVariant* guid = g_variant_new_string(uid);

    std::vector<JSValueRef> args;
    for(size_t i = 1; i < argumentCount; i++)
    {
        args.push_back(arguments[i]);
    }

    jsobj arr = js.array(args);
    std::string json = to_json(ctx,arr.ref());

    GVariant* param = g_variant_new_string(json.c_str());

    gvar_builder tuple = gtuple();
    tuple.add(guid);
    tuple.add(param);

    GVariant* parameters = tuple.build();

    g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        dbus_object_path_send_req_path.c_str(),
        dbus_interface.c_str(),
        signal.c_str(),
        parameters,
        NULL
    );    

    JSObjectRef resolve = 0;
    JSObjectRef reject = 0;
    JSValueRef ex = 0;
    JSObjectRef promise = JSObjectMakeDeferredPromise( ctx, &resolve, &reject, &ex);

    ResponseData* response_data = new ResponseData(ctx,resolve,reject);
    responses().add(uid,response_data);

    g_free(uid);

    return promise;
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
        return js.undefined().ref();   
    }

    jsval arg(ctx,arguments[0]);
    if(!arg.isObject())
    {
        g_print (PROG " error: first arg is not a obj \n");
       return js.undefined().ref();
    }    

    jsobj obj = arg.obj();

    cb = DBusCallback(obj);

    g_print (PROG " bind signals \n");

    return js.undefined().ref();
}
///////////////////////////////////////////////////

struct ResponseCallbackData
{
    std::string uid;
    bool isExHandler;
};

static JSValueRef ResponseCallback_callAsFunctionCallback(
    JSContextRef ctx, 
    JSObjectRef function, 
    JSObjectRef thisObject, 
    size_t argumentCount, 
    const JSValueRef arguments[], 
    JSValueRef* exception)
{
    jsctx js(ctx);
    jsobj fun(ctx,function);

    ResponseCallbackData* data = (ResponseCallbackData*)fun.private_data();

    g_print (PROG "ResponseCallback send_response  cf: %s \n", data->uid.c_str() );

    JSValueRef result = js.undefined().ref();

    if(arguments>0)
    {
        result = arguments[0];
    }

    if(!data->isExHandler)
    {
        send_response(data->uid.c_str(),ctx,result);
    }
    else
    {
        send_response(data->uid.c_str(),ctx,NULL,result);
    }

    return js.undefined().ref();
}

static void ResponseCallback_object_class_finalize_cb(JSObjectRef object)
{
    ResponseCallbackData* data = (ResponseCallbackData*)JSObjectGetPrivate(object);
    delete data;
}


static void ResponseCallback_object_class_init_cb(JSContextRef ctx,JSObjectRef object)
{
}

static const JSClassDefinition ResponseCallback_class_def =
{
    0,                     // version
    kJSClassAttributeNone, // attributes
    "ResponseCallback",    // className
    NULL,                  // parentClass
    NULL,                  // staticValues
    NULL,                  // staticFunctions
    ResponseCallback_object_class_init_cb, // initialize
    ResponseCallback_object_class_finalize_cb, // finalize
    NULL,                  // hasProperty
    NULL,                  // getProperty
    NULL,                  // setProperty
    NULL,                  // deleteProperty
    NULL,                  // getPropertyNames
    ResponseCallback_callAsFunctionCallback, // callAsFunction
    NULL,                  // callAsConstructor
    NULL,                  // hasInstance  
    NULL                   // convertToType
};



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

    g_print (PROG " send_signal cf: %s \n", signal_name);

    std::vector<JSValueRef> args;
    for( size_t i = 0; i < argumentCount; i++)
    {
        args.push_back(arguments[i]);
    }

    jsobj arr = js.array(args);

    std::string json = to_json(ctx,arr.ref());

    GVariant* param = g_variant_new_string(json.c_str());

    gchar* uid = g_dbus_generate_guid();

    gvar_builder tuple = gtuple();

    tuple.add(g_variant_new_string(uid));
    tuple.add(param);

    GVariant* parameters = tuple.build();

    g_print (PROG "send_signal cf: %s %s\n", signal_name, g_variant_get_type_string (parameters));

    g_dbus_connection_emit_signal(
        dbuscon,
        NULL,
        dbus_object_path_send_req_path.c_str(),
        dbus_interface.c_str(),
        signal_name,
        parameters,
        NULL
    );    


    JSObjectRef resolve = 0;
    JSObjectRef reject = 0;
    JSValueRef ex = 0;
    JSObjectRef promise = JSObjectMakeDeferredPromise( ctx, &resolve, &reject, &ex);

    ResponseData* response_data = new ResponseData(ctx,resolve,reject);
    responses().add(uid,response_data);

    g_free(uid);

    return promise;
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
    jsctx js(cb.obj.ctx());

    gvar params(parameters);

    if(!params.isTuple())
    {
        g_print (PROG "invalid signal params is not a tuple:  %s\n", signal_name);
        return;
    }

    int len = params.length();

    gvar uid = params.item(0);
    g_print (PROG "recevied signal with uid:  %s %s\n", signal_name,uid.str() );
    
    jsval arr;  

    if(len>1)
    {
        gvar args = params.item(1);
        std::string json = args.str();

        g_print (PROG " JSON:  %s \n", json.c_str() );
        arr = from_json(js.ctx(),json);
    }

    g_print (PROG " arr:  %i \n", arr.isValid() );



    std::vector<JSValueRef> arguments;
    for( int i = 0; i < arr.obj().length(); i++)
    {
        arguments.push_back(arr.obj().item(i).ref());
    }

    g_print (PROG " ARGS size:  %i \n", arguments.size() );

    jsval member = cb.obj.member(signal_name);

    g_print (PROG " ARGS size:  %i \n", arguments.size() );

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
            if(result.isObject())
            {
                jsobj r = result.obj();
                if(r.hasMember("then"))
                {
                    g_print (PROG "houston, we have a promise %i\n", result.isValid());

                    ResponseCallbackData* data = new ResponseCallbackData{ uid.str(), false  };
                    jsobj responseCB1 = js.object(ResponseCallback_class_def, data);

                    ResponseCallbackData* exData = new ResponseCallbackData{ uid.str(), true  };
                    jsobj responseCB2 = js.object(ResponseCallback_class_def, exData);
                    jsval member = r.member("then");
                    jsobj then = member.obj();


                    std::vector<JSValueRef> args;
                    args.push_back(responseCB1.ref());
                    args.push_back(responseCB2.ref());

                    g_print (PROG "houston, we call then on a promise \n", signal_name);

                    then.invoke(args,r.ref());//std::vector<JSValueRef>& arguments, JSObjectRef that = NULL);

/*
                    JSValueRef ex = 0;
                    JSValueRef result = JSObjectCallAsFunction(
                        cb.obj.ctx(),
                        then.ref(),
                        r.ref(),  
                        args.size(), 
                        &args[0], 
                        &ex
                    );                   
                    if(ex)
                    {
                        g_print (PROG "houston, we have an ex \n" );
                    }
*/                    
                    g_print (PROG "houston, we have called a promise \n", signal_name);
                }
                else
                {
                    g_print (PROG "houston, we do NOT have a promise \n", signal_name);
                    send_response(uid.str(),cb.obj.ctx(),result.ref());
                }
            }
            else
            {
                g_print (PROG "houston, we do NOT have a promise \n", signal_name);
                send_response(uid.str(),cb.obj.ctx(),result.ref());
            }
        }
    }
}

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
    oss_send << dbus_object_path_send_req_prefix << sid;
    dbus_object_path_send_req_path = oss_send.str();

    std::ostringstream oss_recv;
    oss_recv << dbus_object_path_recv_req_prefix << sid;
    dbus_object_path_recv_req_path = oss_recv.str();

    std::ostringstream oss_res_send;
    oss_res_send << dbus_object_path_send_res_prefix << sid;
    dbus_object_path_send_res_path = oss_res_send.str();

    std::ostringstream oss_res_recv;
    oss_res_recv << dbus_object_path_recv_res_prefix << sid;
    dbus_object_path_recv_res_path = oss_res_recv.str();

    g_print (PROG "Interface: %s.\n", dbus_interface.c_str());
    g_print (PROG "Send: %s.\n", dbus_object_path_send_req_path.c_str());
    g_print (PROG "Recv: %s.\n", dbus_object_path_recv_req_path.c_str());
    
    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    g_signal_connect (extension, "page-created", G_CALLBACK (web_page_created_cb),  NULL);
}

} // extern C
