#ifndef _MOL_DEF_GUARD_DEFINE_MTK_WEBKIT_H_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MTK_WEBKIT_H_DEF_GUARD_

#include <type_traits>
#include <map>
#include <memory>
#include <functional>
#include <typeindex>

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <metacpp/meta.h>

#include "glue/gvglue.h"
#include "mtkcpp/connector.h"
#include "mtk/mtkwebview.h"

#include "glue/common.h"

/////////////////////////////////////////////
// forwards

class Channel;

template<class T>
void send_response( Channel* channel, const std::string& uid,  T& t, const char* ex = NULL );
void send_response( Channel* channel, const std::string& uid, const char* ex = NULL );

gboolean user_msg_received(
    WebKitWebView      *web,
    WebKitUserMessage *message,
    gpointer           user_data
);

/////////////////////////////////////////////
// Channel


class Channel
{
public:

    Channel(WebKitWebView* w);

    virtual ~Channel();

    WebKitWebView* web;

    virtual void invoke(const std::string& uid,const std::string& method, Json::Value& params) = 0;
};

/////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////


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

private:

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

public:

    virtual void invoke(const std::string& uid,const std::string& method, Json::Value& params)
    {
        meta::find(*bound,method.c_str(), [this,uid,&params,&method](auto m)
        {
            constexpr size_t NARGS = nargs<decltype(m.member)>::value;
            do_invoke<NARGS,0>(m,uid,method,params);
        });
    };
};
 

std::map<WebKitWebView*,Channel*>& channels();

/////////////////////////////////////////////////////////////

class ResultPromise 
{
public:
    virtual ~ResultPromise() {}

    virtual void invoke(Json::Value&, const char* ex ) = 0;
};

template<class CB>
class ResultPromiseImpl;

void get_json_val(Json::Value& val, std::string& t);
void get_json_val(Json::Value& val, int& t);
void get_json_val(Json::Value& val, bool& t);
void get_json_val(Json::Value& val, double& t);
void get_json_val(Json::Value& val, Json::Value& t);


class ResultFutureEx : public std::exception
{
public:

    ResultFutureEx() {};

    ResultFutureEx(const std::string& msg) : msg_(msg) {};

    virtual const char* what() const noexcept 
    {
        return msg_.c_str();
    }

    std::string msg_;
};

template<class T>
struct ResultFuture
{
    T value;
    const char* ex;

    T& get()
    {
        if(ex)
        {
            throw ResultFutureEx(ex);
        }
        return value;
    }
};

template<>
struct ResultFuture<void>
{
    const char* ex;

    void get()
    {
        if(ex)
        {
            throw ResultFutureEx(ex);
        }
    }
};

template<class T>
class ResultPromiseImpl<void(T)> : public ResultPromise
{
public:

    std::function<void(ResultFuture<T>)> cb;

    ResultPromiseImpl(std::function<void(ResultFuture<T>)> callback)
        :cb(callback)
    {}

    virtual void invoke(Json::Value& val, const char* ex)
    {
        T t;
        get_json_val(val,t);
        cb( ResultFuture<T>{t,ex} );
    }
};

template<>
class ResultPromiseImpl<void()> : public ResultPromise
{
public:

    std::function<void( ResultFuture<void> )> cb;

    ResultPromiseImpl(std::function<void(ResultFuture<void>)> callback)
        :cb(callback)
    {}

    virtual void invoke(Json::Value& val, const char* ex)
    {
        cb( ResultFuture<void>{ex} );
    }
};

template<class T>
auto result_promise( std::function<void(ResultFuture<T>)> fun)
{
    return new ResultPromiseImpl<void(T)>(fun);
}

inline auto result_promise( std::function<void(ResultFuture<void>)> fun)
{
    return new ResultPromiseImpl<void()>(fun);
}

/////////////////////////////////////////////////////////////

class Responses
{
public:

    void add(const char* uid, ResultPromise* rf);

    ResultPromise* get(const char* uid);

private:

    std::map<std::string,ResultPromise*> pending_;
};


Responses& responses();


/////////////////////////////////////////////////////////////


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

void send_response( Channel* channel, const std::string& uid, const char* ex  );


/////////////////////////////////////////////////////////////

struct RequestFuture
{
    std::string uid;

    template<class T>
    struct deducer;
    
    template<class R,class C,class ...Args>
    struct deducer<R(C::*)(Args...) const>
    {
        using func_t = std::function<R(Args...)>;

        template<class F>
        deducer( F f )
            : fun(f)
        {}   

        func_t fun;
    };

    template<class T>
    void then( T f)
    {
        deducer<decltype(& T::operator())> d( f );
        responses().add( uid.c_str(), result_promise(d.fun) );
    }
};


template<class ... Args>
RequestFuture send_request( MtkWebView* web, std::string m, Args ... args)
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
    
    RequestFuture f{ uid };
    g_free(uid);
    return f;
}


/////////////////////////////////////////////////////////////


template<class C>
void webkit_bind( MtkWebView* webview, C* controller)
{
    WebKitWebView* web = (WebKitWebView*)webview;

    Channel* channel = new ChannelImpl( web, controller );

    channels().insert( std::make_pair( web, channel) );

    g_signal_connect(G_OBJECT(web), "user-message-received", G_CALLBACK (user_msg_received),  NULL);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class Gui 
{
public:

    Gui();
    ~Gui();

    Gui& load(const char* file);
    Gui& load_string(const char* file);
    Gui& show(const char* mainWindow);
    Gui& show();

    GObject* get_object(const char* name);

    template<class T>
    T* get(const char* name)
    {
        return (T*)get_object(name);
    }

    template<class T>
    Gui& bind(T* t)
    {   
        GtkBuilderConnectFunc fun = &connector<T>; 
        gtk_builder_connect_signals_full(builder, fun, t);        
        return *this;
    }

    template<class Arg, class ...Args>
    Gui& register_widgets(Arg arg, Args ... args)
    {
        arg();
        return register_widgets(args...);
    }

    int alert(const std::string& s, int buttons = GTK_BUTTONS_OK, int type = GTK_MESSAGE_INFO, int default_button = GTK_BUTTONS_OK, int flags = GTK_DIALOG_MODAL, const char* parent = "mainWindow" )
    {
        GtkDialog* dlg = (GtkDialog*)gtk_message_dialog_new( 
            (GtkWindow*)(get_object(parent)),
            (GtkDialogFlags)flags,
            (GtkMessageType)type,
            (GtkButtonsType)buttons,
            "%s",
            s.c_str()
        );
        gtk_dialog_set_default_response(dlg,default_button);
        int r = gtk_dialog_run(dlg);
        gtk_widget_hide( (GtkWidget*)dlg );
        return r;
    }

    struct accel_data {
       virtual ~accel_data() {}

       virtual void invoke() = 0; 
    };

    template<class F, class T>
    struct accel_data_impl : public accel_data 
    {

        F callback;
        T* that;

        accel_data_impl(F fun, T* t)
            : callback(fun), that(t)
        {}

        virtual void invoke()
        {
            g_print("accel_data_impl invoke \n");
            (that->*callback)(NULL);
        }
    };

    template<class F, class T>
    Gui& add_accelerator(const char* ac, F fun, T* that)
    {
        guint key = 0;
        GdkModifierType mod = (GdkModifierType)0;
        gtk_accelerator_parse( ac, &key, &mod);

        auto cb = [](GtkAccelGroup *accel_group,
                          GObject *acceleratable,
                          guint keyval,
                          GdkModifierType modifier,
                          gpointer user_data)
        {
            accel_data* ad = (accel_data*)user_data;
            ad->invoke();
            delete ad;
        };

        accel_data* ad = new accel_data_impl( fun, that );

        void(*tmp)(GtkAccelGroup*,GObject*,guint,GdkModifierType,gpointer) = cb;

        GClosure* closure = g_cclosure_new( G_CALLBACK(tmp), ad, 0);
        gtk_accel_group_connect(ag,key,mod,GTK_ACCEL_VISIBLE,closure);

        return *this;
    }

    Gui& status_bar(const char* txt, const char* id = "statusBar")
    {
        GtkStatusbar* bar = get<GtkStatusbar>(id);

        if(!status_ctx)
        {
            status_ctx = gtk_statusbar_get_context_id(bar,"main ui statusbar context");
        }

        gtk_statusbar_pop(bar,status_ctx);
        gtk_statusbar_push(bar,status_ctx,txt);
        return *this;
    }

    std::string showFileDialog(
        GtkFileChooserAction action, 
        const char* title, 
        const char* filter = 0,
        const char* dir = 0,
        const char* parent = "mainWindow" )
    {
        const gchar* button = ( action == GTK_FILE_CHOOSER_ACTION_OPEN || 
                         action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER ) 
                       ? "gtk-open" : "gtk-save";


        GtkDialog* dlg = (GtkDialog*) gtk_file_chooser_dialog_new(
            title,
            get<GtkWindow>(parent),
            action,
            "gtk-cancel",
            GTK_RESPONSE_CANCEL,
            button,
            GTK_RESPONSE_OK,
            NULL
        );

        if(dir)
        {
            gtk_file_chooser_set_current_folder_uri( (GtkFileChooser*) dlg, dir );
        }

        if(filter)
        {
            GtkFileFilter* f = get<GtkFileFilter>(filter);

            if(f)
            {
                gtk_file_chooser_set_filter( (GtkFileChooser*) dlg, f);
            }
        }

        gtk_dialog_set_default_response( dlg, GTK_RESPONSE_OK );

        gint response = gtk_dialog_run(dlg);

        std::string result = "";

        if(response == GTK_RESPONSE_OK)
        {
            gchar* c = gtk_file_chooser_get_filename( (GtkFileChooser*) dlg );
            result = c;
            g_free(c);
        }

        gtk_widget_destroy( (GtkWidget*) dlg );
        return result;
    }


private:

    GtkAccelGroup* ag;
    guint status_ctx = 0;

    Gui& register_widgets()
    {
        return *this;
    }

    GtkBuilder* builder = 0;
};


Gui& gui();

/////////////////////////////////////////////////////////////


#endif

