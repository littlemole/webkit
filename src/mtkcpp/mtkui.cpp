#include "mtkcpp/ui.h"
#include "glue/gvglue.h"
#include <dlfcn.h>
#include <libgen.h>

#define PROG "[mtkWebKit] "

Channel::Channel(WebKitWebView* w)
    : web(w)
{
    g_object_ref(w);
}

Channel::~Channel()
{
    g_object_unref(web);
}

 
std::map<WebKitWebView*,Channel*>& channels()
{
    static std::map<WebKitWebView*,Channel*> map;
    return map;
};

// 


void get_json_val(Json::Value& val, std::string& t)
{
    t = val.asString();
}

void get_json_val(Json::Value& val, int& t)
{
    t = val.asInt();
}

void get_json_val(Json::Value& val, bool& t)
{
    t = val.asBool();
}

void get_json_val(Json::Value& val, double& t)
{
    t = val.asDouble();
}

void get_json_val(Json::Value& val, Json::Value& t)
{
    t = val;
}



void Responses::add(const char* uid, ResultPromise* rf)
{
    pending_.insert( std::make_pair(std::string(uid),rf) );
    //g_print (PROG "responses add %s\n", uid );
}

ResultPromise* Responses::get(const char* uid)
{
    //g_print (PROG "responses get %s\n", uid );
    if ( pending_.count(std::string(uid)) == 0)
    {
        return 0;
    }
    ResultPromise* res = pending_[std::string(uid)];
    pending_.erase(uid);
    return res;
}


Responses& responses()
{
    static Responses r;
    return r;
}



///////////////////////////////////

static void signal_handler(WebKitWebView *web, GVariant* message)
{
    gvar msg(message);

    // parse JSON

    std::string json = msg.str();
    Json::Value dict = JSON::parse(json);

    std::string uid = dict["request"].asString();
    std::string method = dict["method"].asString();
    Json::Value parameters = dict["parameters"];

    Channel* channel = channels()[web];

    channel->invoke(uid,method,parameters);

}


void send_response( Channel* channel, const std::string& uid, const char* ex  )
{
    Json::Value dict(Json::objectValue);
    dict["response"] = uid;
    dict["result"] = Json::Value(Json::nullValue);
    if(ex)
    {
        dict["exception"] = ex;
    }
    else
    {
        dict["exception"] = Json::Value(Json::nullValue);
    }

    std::string json = JSON::flatten(dict);

    GVariant* msg = g_variant_new_string(json.c_str());
    WebKitUserMessage* message = webkit_user_message_new( "response", msg);

    webkit_web_view_send_message_to_page( channel->web, message, NULL, NULL, NULL);    
}


///////////////////////////////////

static void response_handler(GVariant* message)
{
    gvar msg(message);

    std::string json = msg.str();

    Json::Value dict = JSON::parse(json);

    std::string uid = dict["response"].asString();
    Json::Value result = dict["result"];
    const char* exception = 0;
    if( dict.isMember("exception") )
    {
        exception = dict["exception"].asCString();
    }

    ResultPromise* rf = responses().get( uid.c_str() );
    if(!rf)
    {
        return;
    }

    rf->invoke(result,exception);

    delete rf;
}

gboolean user_msg_received(
    WebKitWebView      *web,
    WebKitUserMessage *message,
    gpointer           user_data
    )
{
    GVariant* params = webkit_user_message_get_parameters(message);

    std::string name = webkit_user_message_get_name(message);

    if( name == "request")
    {
        signal_handler(web,params);
    }
    if( name== "response")
    {
        response_handler(params);
    }

    return TRUE;
}

/////////////////////////////////////////////////////

Gui::Gui()
{
    ag = gtk_accel_group_new();
}

Gui::~Gui()
{
    if(builder)
    {
        //g_object_unref(builder);
    }
}

Gui& Gui::load(const char* file)
{
    std::string path = file;

    if(path[0] != '/')
    {
        std::ostringstream oss;

        Dl_info info;
        if (dladdr((const void*)&user_msg_received, &info))
        {
            char* fn = 0;
            fn = strdup(info.dli_fname);
            char* dir = dirname(fn);
            dir = dirname(dir);
            oss << dir;
            free(fn);
        }

        oss << "/" << file;
        path = oss.str();
    }

    builder = gtk_builder_new();

    GError* error = 0;
    if ( gtk_builder_add_from_file(builder,path.c_str(), &error) == 0)
    {
        g_printerr ("Error loading file: %s\n", error->message);
        g_clear_error (&error);
        exit(1);
    }

    return *this;
}

GObject* Gui::get_object(const char* name)
{
    return gtk_builder_get_object(builder,name);
}

Gui& Gui::show(const char* mainWindow)
{
    auto window = gtk_builder_get_object (builder, mainWindow );

    gtk_window_add_accel_group( (GtkWindow*)window, ag );
    gtk_widget_show_all((GtkWidget*)window);
    return *this;
}

Gui& Gui::show()
{
    return show("mainWindow");
}

Gui& gui()
{
    static Gui ui;
    return ui;
}

