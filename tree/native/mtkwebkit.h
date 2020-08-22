#ifndef _MOL_DEF_GUARD_DEFINE_MTK_WEBKIT_H_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MTK_WEBKIT_H_DEF_GUARD_

#include "../common.h"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <metacpp/meta.h>
#include <type_traits>
#include "gvglue.h"
#include "pywebkit.h"
#include <map>
#include <memory>
#include <functional>
#include <typeindex>
/////////////////////////////////////////////
// forwards

class Channel;
/*
static PyObject* pywebkit_run_async(
    PyObject* self, 
    PyObject* args
);
*/

template<class T>
void send_response( Channel* channel, const std::string& uid,  T& t, const char* ex = NULL );
void send_response( Channel* channel, const std::string& uid, const char* ex = NULL );

/////////////////////////////////////////////
// Channel


class Channel
{
public:
    Channel(WebKitWebView* w)
        : web(w)
    {
        g_object_ref(w);
    }

    virtual ~Channel()
    {
        g_object_unref(web);
    }

    WebKitWebView* web;

    virtual void invoke(const std::string& uid,const std::string& method, Json::Value& params) = 0;
};


template<class T>
struct nargs
{
    constexpr static size_t value = 0;
};

template<class R, class C, class ... Args>
struct nargs<R(C::*)(Args...)>
{
    constexpr static size_t value = sizeof...(Args);
};


template<class T>
struct mem_type
{
    using type = void;
};

template<class R, class C, class ... Args>
struct mem_type<R(C::*)(Args...)>
{
    using type = C;
};

template<size_t I, class Arg, class ... Args>
struct arg_type
{
    using type = typename arg_type<I-1,Args...>::type;
};

template<class Arg, class ... Args>
struct arg_type<0,Arg,Args...>
{
    using type = Arg;
};

template<class T>
class ChannelImpl : public Channel
{
public:
    ChannelImpl(WebKitWebView* w, T* bind)
        : Channel(w), bound(bind)
    {
        g_object_ref(w);
    }

    virtual ~ChannelImpl()
    {
    }

    WebKitWebView* web;
    T* bound;

    void do_get_arg(std::string& s, Json::Value& v)
    {
        s = v.asString(); 
    }

    void do_get_arg(int& i, Json::Value& v)
    {
        i = v.asInt(); 
    }

    void do_get_arg(bool& i, Json::Value& v)
    {
        i = v.asBool(); 
    }

    void do_get_arg(double& i, Json::Value& v)
    {
        i = v.asDouble(); 
    }

    void do_get_arg(Json::Value& i, Json::Value& v)
    {
        i = v; 
    }

    template<class P>
    void do_get_arg(P&, Json::Value)
    {}

    template<size_t I, class R, class C, class ...Args>
    auto get_arg( meta::impl::MemFun<R(C::*)(Args...)>, Json::Value& params)
    {
        using type = typename arg_type<I,Args...>::type;
        type result;
        do_get_arg(result,params[(int)I]);
        return result;
    }

    template<size_t L, size_t I, class M, class ... Args>
    void do_invoke(M& m, const std::string& uid,const std::string& method, Json::Value& params, Args ... args)
    {
        if constexpr( L == I )
        {
            using m_t = typename mem_type<decltype(m.member)>::type;
            using r_t = std::invoke_result_t<decltype(m.member),m_t*,Args...>;

            if constexpr(std::is_void<r_t>::value)
            {
                (bound->*(m.member))(args...);
                send_response(this,uid);
            }
            else
            {
                r_t r = (bound->*(m.member))(args...);
                send_response(this,uid,r);
            }            
        }
        else
        {
            do_invoke<L,I+1>(m,uid,method,params,args..., get_arg<I>(m,params) );
        }
    };

    virtual void invoke(const std::string& uid,const std::string& method, Json::Value& params)
    {
        meta::find(*bound,method.c_str(), [this,uid,&params,&method](auto m)
        {
            constexpr size_t NARGS = nargs<decltype(m.member)>::value;
            do_invoke<NARGS,0>(m,uid,method,params);
        });
    };
};
 

std::map<WebKitWebView*,Channel*>& channels()
{
    static std::map<WebKitWebView*,Channel*> map;
    return map;
};

// 

class ResultFuture 
{
public:
    virtual ~ResultFuture() {}

    virtual void invoke(Json::Value&) = 0;
};

template<class CB>
class ResultFutureImpl;

inline void get_json_val(Json::Value& val, std::string& t)
{
    t = val.asString();
}

inline void get_json_val(Json::Value& val, int& t)
{
    t = val.asInt();
}

inline void get_json_val(Json::Value& val, bool& t)
{
    t = val.asBool();
}

inline void get_json_val(Json::Value& val, double& t)
{
    t = val.asDouble();
}

inline void get_json_val(Json::Value& val, Json::Value& t)
{
    t = val;
}


template<class T>
class ResultFutureImpl<void(T)> : public ResultFuture
{
public:

    std::function<void(T)> cb;

    ResultFutureImpl(const std::function<void(T)>& callback)
        :cb(callback)
    {}

    virtual void invoke(Json::Value& val)
    {
        T t;
        get_json_val(val,t);
        cb( t );
    }
};

template<>
class ResultFutureImpl<void()> : public ResultFuture
{
public:

    std::function<void()> cb;

    ResultFutureImpl(const std::function<void()>& callback)
        :cb(callback)
    {}

    virtual void invoke(Json::Value& val)
    {
        cb();
    }
};

template<class T>
auto result_future( const std::function<void(T)>& fun)
{
    return new ResultFutureImpl<void(T)>(fun);
}

inline auto result_future( const std::function<void(void)>& fun)
{
    return new ResultFutureImpl<void()>(fun);
}

class Responses
{
public:

    void add(const char* uid, ResultFuture* rf)
    {
        pending_.insert( std::make_pair(std::string(uid),rf) );
        //g_print (PROG "responses add %s\n", uid );
    }

    ResultFuture* get(const char* uid)
    {
        //g_print (PROG "responses get %s\n", uid );
        if ( pending_.count(std::string(uid)) == 0)
        {
            return 0;
        }
        ResultFuture* res = pending_[std::string(uid)];
        pending_.erase(uid);
        return res;
    }

private:

    std::map<std::string,ResultFuture*> pending_;
};

Responses& responses()
{
    static Responses r;
    return r;
}


static void response_handler(GVariant* params);
static void signal_handler(WebKitWebView *web, GVariant* params);

//static void send_response( Channel* channel, const gchar* uid,  PyObject* val, const char* ex = NULL );
//static void send_request( Channel* channel, const gchar* uid,  std::string m, PyObject* params);

#define PROG "[mtkWebKit] "

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

template<class T>
void send_response( Channel* channel, const std::string& uid,  T& t, const char* ex  )
{
    Json::Value dict(Json::objectValue);
    dict["response"] = uid;
    dict["result"] = Json::Value(t);
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

template<class CB, class ... Args>
static void send_request( PywebkitWebview* web, const CB& cb, std::string m, Args ... args)
{
    gchar* uid = g_dbus_generate_guid();

    std::vector<Json::Value> params{ Json::Value(args)... };

    Json::Value array(Json::arrayValue);
    for( auto& p : params)
    {
        array.append(p);
    }

    Json::Value request(Json::objectValue);
    request["request"] = uid;
    request["method"] = m;
    request["parameters"] = array;

    std::string json = JSON::flatten(request);

    GVariant* msg = g_variant_new_string(json.c_str());
    WebKitUserMessage* message = webkit_user_message_new( "request", msg);

    webkit_web_view_send_message_to_page( (WebKitWebView*) web, message, NULL, NULL, NULL);
    
    responses().add( uid, result_future(cb) );

    g_free(uid);

}

template<class ... Args>
static void send_request( PywebkitWebview* web, std::string m, Args ... args)
{
    gchar* uid = g_dbus_generate_guid();

    std::vector<Json::Value> params{ Json::Value(args)... };

    Json::Value array(Json::arrayValue);
    for( auto& p : params)
    {
        array.append(p);
    }

    Json::Value request(Json::objectValue);
    request["request"] = uid;
    request["method"] = m;
    request["parameters"] = array;

    std::string json = JSON::flatten(request);

    GVariant* msg = g_variant_new_string(json.c_str());
    WebKitUserMessage* message = webkit_user_message_new( "request", msg);

    webkit_web_view_send_message_to_page( (WebKitWebView*) web, message, NULL, NULL, NULL);
    
    std::function<void(void)> fun([](){});
    responses().add( uid, result_future(fun));

    g_free(uid);

}


///////////////////////////////////

static void response_handler(GVariant* message)
{
    gvar msg(message);

    std::string json = msg.str();

    Json::Value dict = JSON::parse(json);

    std::string uid = dict["response"].asString();
    Json::Value result = dict["result"];
    std::string exception = dict["exception"].asString();

    ResultFuture* rf = responses().get( uid.c_str() );

    rf->invoke(result);

    delete rf;
}

static gboolean user_msg_received(
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

template<class C>
void webkit_bind( PywebkitWebview* webview, C* controller)
{
    WebKitWebView* web = (WebKitWebView*)webview;

    Channel* channel = new ChannelImpl( web, controller );

    channels().insert( std::make_pair( web, channel) );

    g_signal_connect(G_OBJECT(web), "user-message-received", G_CALLBACK (user_msg_received),  NULL);
}


class Gui 
{
public:
    Gui(){}

    ~Gui()
    {
        if(builder)
        {
            //g_object_unref(builder);
        }
    }

    Gui& load(const std::string& file)
    {
        builder = gtk_builder_new();

        GError* error = 0;
        if ( gtk_builder_add_from_file(builder,file.c_str(), &error) == 0)
        {
            g_printerr ("Error loading file: %s\n", error->message);
            g_clear_error (&error);
            exit(1);
        }

        return *this;
    }

    GObject* get_object(const std::string& name)
    {
        return gtk_builder_get_object(builder,name.c_str());
    }

    template<class T>
    T* get(const std::string& name)
    {
        return (T*)get_object(name);
    }

    Gui& show(const std::string& mainWindow)
    {
        
        auto window = gtk_builder_get_object (builder, mainWindow.c_str() );
        gtk_widget_show_all((GtkWidget*)window);
        return *this;
    }

    template<class T>
    Gui& bind(T* t)
    {   
        GtkBuilderConnectFunc fun = &connector<T>; 
        gtk_builder_connect_signals_full(builder, fun, t);        
        return *this;
    }

    Gui& show()
    {
        return show("mainWindow");
    }

    template<class Arg, class ...Args>
    Gui& register_widgets(Arg arg, Args ... args)
    {
        auto t = arg();
        return register_widgets(args...);
    }

private:

    Gui& register_widgets()
    {
        return *this;
    }


    GtkBuilder* builder = 0;
};

inline Gui& gui()
{
    static Gui ui;
    return ui;
}

#endif

