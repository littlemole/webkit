#include "common.h"

BoundController theCallback;


static GVariant* make_response(
    const gchar* uid,
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex = NULL
    ) 
{
    jsctx js(ctx);

    //g_print (PROG " make_response: \n");

    jsobj obj = js.object();

    obj.set("response", js.string(uid) );
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

    GVariant* result = make_response(uid,ctx,value,ex);

    WebKitUserMessage* msg = webkit_user_message_new( "response", result);

    webkit_web_page_send_message_to_view(
        thePage,
        msg,
        NULL,
        NULL,
        NULL
    );
}


void response_handler(GVariant* message)                        
{
    //g_print (PROG " received response  %s\n", g_variant_get_type_string (message));

    jsctx js(theCallback.obj.ctx());

    // extract JSON
    gvar msg(message);
    std::string json = msg.str();
    jsobj dict = from_json(js.ctx(),json).obj();

    // extract response params
    jsval result = js.undefined();
    jsval ex = js.undefined();
    jsval uid = js.undefined();

    if ( dict.hasMember("result") )
    {
        result = dict.member("result");
    }
    if ( dict.hasMember("exception") )
    {
        ex = jsobj(dict).member("exception");
    }
    if ( dict.hasMember("response") )
    {
        uid = dict.member("response");
    }

    // retrive Promise for this response
    ResponseData* response_data = responses().get( uid.str().c_str() );
    if(!response_data)
    {
        g_print (PROG "received invalid response uid not found %s\n", uid.str().c_str() );
        return;
    }

    // call reject or response depending on response params
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


void signal_handler(GVariant* message)
{
    jsctx js(theCallback.obj.ctx());

    // unpack json
    gvar msg(message);
    std::string json = msg.str();
    jsobj dict = from_json(js.ctx(),json).obj();

    // extract data
    jsval uid = js.undefined();
    jsval method = js.undefined();
    jsval params = js.undefined();

    if ( dict.hasMember("request") )
    {
        uid = dict.member("request");
    }
    if ( dict.hasMember("method") )
    {
        method = jsobj(dict).member("method");
    }
    if ( dict.hasMember("parameters") )
    {
        params = dict.member("parameters");
    }

    // validate data
    if(!uid.isValid() || uid.isNull() || uid.isUndefined() || uid.str().size() == 0 )
    {
        g_print (PROG "invalid signal without uid \n" );
        return;
    }

    if(!method.isValid() || method.isNull() || method.isUndefined() || method.str().size() == 0 )
    {
        g_print (PROG "invalid signal without method \n" );
        return;
    }

    if(!params.isValid() || params.isNull() || params.isUndefined() )
    {
        g_print (PROG "invalid signal without parameters \n" );
        return;
    }

    // validate member of bound controller to call
    jsval member = theCallback.obj.member(method.str());

    if(member.isUndefined())
    {
        g_print (PROG "unknown signal %s\n", method.str().c_str() );
        return;
    }

    jsobj fun = member.obj();

    bool isFunction = fun.isFunction();
    if(!isFunction)
    {
        g_print (PROG "signal %s is not a function \n", method.str().c_str() );
        return;
    }

    // call the bound member function of controller
    std::vector<JSValueRef> arguments = params.obj().vector();

    jsval result = fun.invoke(arguments);

    // check if result is a Promise
    if(result.isObject())
    {
        jsobj r = result.obj();
        if(r.hasMember("then"))
        {
            // prepare Promise reject and resolve handler
            ResponseCallbackData* data = new ResponseCallbackData{ uid.str(), false  };
            jsobj onSuccess = js.object(ResponseCallback_class_def, data);

            ResponseCallbackData* exData = new ResponseCallbackData{ uid.str(), true  };
            jsobj onException = js.object(ResponseCallback_class_def, exData);

            // prepare call to promise.then()
            jsval member = r.member("then");
            jsobj then = member.obj();

            std::vector<JSValueRef> args;
            args.push_back(onSuccess.ref());
            args.push_back(onException.ref());

            // call promise.then( onSuccess, onException );
            then.invoke(args,r.ref());
            return;
        }
    }

    // default return result
    send_response(uid.str().c_str(),theCallback.obj.ctx(),result.ref());
}


///////////////////////////////////////////////////////////////
