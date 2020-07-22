#include "common.h"

const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_req_prefix = "/org/oha7/webkit/WebKitDBus/view/request/";
static const std::string dbus_object_path_recv_req_prefix = "/org/oha7/webkit/WebKitDBus/controller/request/";
static const std::string dbus_object_path_send_res_prefix = "/org/oha7/webkit/WebKitDBus/view/response/";
static const std::string dbus_object_path_recv_res_prefix = "/org/oha7/webkit/WebKitDBus/controller/response/";

std::string dbus_object_path_send_req_path;
std::string dbus_object_path_recv_req_path; 

std::string dbus_object_path_send_res_path;
std::string dbus_object_path_recv_res_path; 



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

    theCallback = DBusCallback(obj);

    //g_print (PROG " bind signals \n");

    return js.undefined().ref();
}


////////////////////////////////////////////////////////
extern "C" {

void dom_loaded_cb (WebKitWebPage *web_page, gpointer user_data)
{
    g_print (PROG " DOM loaded\n");
}


static void window_object_cleared(
    WebKitScriptWorld *world,
    WebKitWebPage     *page,
    WebKitFrame       *frame,
    gpointer           user_data)
{
    g_print (PROG " Page window_object_cleared\n");

    JSGlobalContextRef ctx = webkit_frame_get_javascript_global_context(frame);
    jsctx js(ctx);

    jsobj global = js.globalObject();

    jstr bindSignalsFunctionName("bind");
    jsobj bindSignalsFunction = js.make_function(bindSignalsFunctionName.ref(), &bind_signals);

    jsobj WebKitDBus = js.object();

    WebKitDBus.set(bindSignalsFunctionName.ref(), bindSignalsFunction.ref());

    jsobj controller = js.object(Controller_class_def);
    WebKitDBus.set("Host",controller.ref());
     
    global.set("WebKit", WebKitDBus.ref());

}

static void web_page_created_cb (WebKitWebExtension *extension, WebKitWebPage *web_page, gpointer user_data)
{
    g_print (PROG " Page created\n");

    WebKitFrame* frame = webkit_web_page_get_main_frame(web_page);

    WebKitScriptWorld* jsworld = webkit_script_world_get_default();
    g_signal_connect(jsworld, "window-object-cleared", G_CALLBACK (window_object_cleared),  NULL);



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
