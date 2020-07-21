
#ifndef __MO_WEBKIT_JS_WEBKIT_EXT_COMMON_H__
#define __MO_WEBKIT_JS_WEBKIT_EXT_COMMON_H__

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

extern const JSClassDefinition ResponseCallback_class_def;
extern const JSClassDefinition Signal_class_def;
extern const JSClassDefinition Controller_class_def;

void send_response(
    const gchar* uid,
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex = NULL
); 

void got_dbus (
    GObject *source_object,
    GAsyncResult *res,
    gpointer user_data
    );

extern std::string sid;
extern std::string rsid;

extern const std::string dbus_interface;

extern std::string dbus_object_path_send_req_path;
extern std::string dbus_object_path_recv_req_path; 

extern std::string dbus_object_path_send_res_path;
extern std::string dbus_object_path_recv_res_path; 

extern GDBusConnection* dbuscon;

///////////////////////////////////////////////////

struct ResponseCallbackData
{
    std::string uid;
    bool isExHandler;
};

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

inline Responses& responses()
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

extern DBusCallback theCallback;


#endif
