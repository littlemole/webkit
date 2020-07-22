#include "common.h"

std::string sid;
std::string rsid;

DBusCallback theCallback;

GDBusConnection* dbuscon = 0;

static GVariant* make_response(
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex = NULL
    ) 
{
    jsctx js(ctx);

    //g_print (PROG " make_response: \n");

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

void send_response(
    const gchar* uid,
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex 
    ) 
{
    jsctx js(ctx);

    //g_print (PROG " send_response: %s \n", uid);

    gvar_builder builder = gtuple();  

    GVariant* guid = g_variant_new_string(uid);
    builder.add(guid);

    GVariant* data = make_response(ctx,value,ex);
    builder.add(data);

    GVariant* params = builder.build();
   
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
    }
    else
    {
        // g_print (PROG "send_response sent %i\n",r);
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
    //g_print (PROG " received response %s %s\n", signal_name, g_variant_get_type_string (parameters));

    jsctx js(theCallback.obj.ctx());

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
        v.push_back(ex.ref());
        jsobj(js.ctx(),response_data->reject).invoke(v);
    }
    else 
    {
        v.push_back(result.ref());
        jsobj(js.ctx(),response_data->resolve).invoke(v);
    }

    delete response_data;
}


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
    jsctx js(theCallback.obj.ctx());

    gvar params(parameters);

    if(!params.isTuple())
    {
        g_print (PROG "invalid signal params is not a tuple:  %s\n", signal_name);
        return;
    }

    int len = params.length();

    gvar uid = params.item(0);
    //g_print (PROG "recevied signal with uid:  %s %s\n", signal_name,uid.str() );
    
    jsval arr;  

    if(len>1)
    {
        gvar args = params.item(1);
        std::string json = args.str();

        //g_print (PROG " JSON:  %s \n", json.c_str() );
        arr = from_json(js.ctx(),json);
    }

    std::vector<JSValueRef> arguments;
    for( int i = 0; i < arr.obj().length(); i++)
    {
        arguments.push_back(arr.obj().item(i).ref());
    }

    jsval member = theCallback.obj.member(signal_name);

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
                    ResponseCallbackData* data = new ResponseCallbackData{ uid.str(), false  };
                    jsobj responseCB1 = js.object(ResponseCallback_class_def, data);

                    ResponseCallbackData* exData = new ResponseCallbackData{ uid.str(), true  };
                    jsobj responseCB2 = js.object(ResponseCallback_class_def, exData);
                    jsval member = r.member("then");
                    jsobj then = member.obj();


                    std::vector<JSValueRef> args;
                    args.push_back(responseCB1.ref());
                    args.push_back(responseCB2.ref());

                    then.invoke(args,r.ref());
                }
                else
                {
                    send_response(uid.str(),theCallback.obj.ctx(),result.ref());
                }
            }
            else
            {
                send_response(uid.str(),theCallback.obj.ctx(),result.ref());
            }
        }
    }
}


///////////////////////////////////////////////////////////////


void got_dbus (
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

