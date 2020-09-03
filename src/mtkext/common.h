
#ifndef __MO_WEBKIT_JS_WEBKIT_EXT_COMMON_H__
#define __MO_WEBKIT_JS_WEBKIT_EXT_COMMON_H__

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <map>
#include <glib.h>
#include <gio/gio.h>
#include <webkit2/webkit-web-extension.h>

#include "glue/gvglue.h"
#include "glue/jsglue.h"

#define PROG "[mtkext] "

extern const jsclassdef ResponseCallback_class_def;
extern const jsclassdef Signal_class_def;
extern const jsclassdef Controller_class_def;

void response_handler(GVariant* message);
void signal_handler(GVariant* message);

void send_response(
    const gchar* uid,
    JSContextRef ctx, 
    const JSValueRef value,
    const JSValueRef ex = NULL
); 

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
    }

    ResponseData* get(const char* uid)
    {
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

struct BoundController 
{
    BoundController()
    {}

    BoundController(jsobj& o)
        : obj(o)
    {
        obj.protect();
    }

    ~BoundController()
    {
        obj.unprotect();
    }

    BoundController& operator=(const BoundController& rhs)
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

extern BoundController theCallback;

extern WebKitWebPage* thePage;

#endif
