
#include "common.h"


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

    //g_print (PROG "ResponseCallback send_response  cf: %s \n", data->uid.c_str() );

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

const jsclassdef ResponseCallback_class_def( [] (JSClassDefinition& clazz)
{
    clazz.className = "ResponseCallback";
    clazz.initialize = ResponseCallback_object_class_init_cb;
    clazz.finalize = ResponseCallback_object_class_finalize_cb;
    clazz.callAsFunction = ResponseCallback_callAsFunctionCallback;
});

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

    //g_print (PROG " send_signal cf: %s \n", signal_name);

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

    //g_print (PROG "send_signal cf: %s %s\n", signal_name, g_variant_get_type_string (parameters));

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


const jsclassdef Signal_class_def( [] (JSClassDefinition& clazz)
{
    clazz.className = "Signal";
    clazz.initialize = Signal_object_class_init_cb;
    clazz.finalize = Signal_object_class_finalize_cb;
    clazz.callAsFunction = Signal_callAsFunctionCallback;
});

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


const jsclassdef Controller_class_def( [] (JSClassDefinition& clazz)
{
    clazz.className = "Controller";
    clazz.hasProperty = Controller_hasPropertyCallback;
    clazz.getProperty = Controller_getPropertyCallback;
});

////////////////////////////////////////////////////////
