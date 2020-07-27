#include "common.h"

static std::string sid;

WebKitWebPage* thePage = 0;

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

    theCallback = BoundController(obj);

    //g_print (PROG " bind signals \n");

    return js.undefined().ref();
}


////////////////////////////////////////////////////////
extern "C" {

void dom_loaded_cb (WebKitWebPage *web_page, gpointer user_data)
{
    g_print (PROG " DOM loaded\n");
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static JSGlobalContextRef get_global_ctx(WebKitFrame* frame)
{
    JSGlobalContextRef ctx = webkit_frame_get_javascript_global_context(frame);
    return ctx;
}

#pragma GCC diagnostic pop


static void window_object_cleared(
    WebKitScriptWorld *world,
    WebKitWebPage     *page,
    WebKitFrame       *frame,
    gpointer           user_data)
{
    g_print (PROG " Page window_object_cleared\n");

    if( thePage )
    {
        g_object_unref(thePage);
    }
    thePage = page;
    g_object_ref(thePage);
    
    JSGlobalContextRef ctx = get_global_ctx(frame);
    jsctx js(ctx);

    jsobj global = js.globalObject();

    jstr bindSignalsFunctionName("bind");
    jsobj bindSignalsFunction = js.make_function(bindSignalsFunctionName.ref(), &bind_signals);

    jsobj WebKit = js.object();

    WebKit.set(bindSignalsFunctionName.ref(), bindSignalsFunction.ref());

    jsobj controller = js.object(Controller_class_def);
    WebKit.set("Python",controller.ref());
     
    global.set("WebKit", WebKit.ref());

}

static gboolean user_msg_received(
    WebKitWebPage     *web_page,
    WebKitUserMessage *message,
    gpointer           user_data
    )
{
    GVariant* params = webkit_user_message_get_parameters(message);

    std::string name = webkit_user_message_get_name(message);

    if( name == "request")
    {
        signal_handler(params);
    }
    if( name== "response")
    {
        response_handler(params);
    }

    return TRUE;
}

static void web_page_created_cb (WebKitWebExtension *extension, WebKitWebPage *web_page, gpointer user_data)
{
    g_print (PROG " Page created\n");

    //WebKitFrame* frame = webkit_web_page_get_main_frame(web_page);
    WebKitScriptWorld* jsworld = webkit_script_world_get_default();

    g_signal_connect(jsworld, "window-object-cleared", G_CALLBACK (window_object_cleared),  NULL);
    g_signal_connect (web_page, "user-message-received", G_CALLBACK (user_msg_received),  NULL);
    g_signal_connect (web_page, "document-loaded", G_CALLBACK (dom_loaded_cb),  NULL);
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

    g_signal_connect (extension, "page-created", G_CALLBACK (web_page_created_cb),  NULL);
}

} // extern C
