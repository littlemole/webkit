
#ifndef __MO_WEBKIT_JS_GLUE_H__
#define __MO_WEBKIT_JS_GLUE_H__

#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

/*
 * deal with Javascript Strings. there are three ways to construct a 
 * String:
 *
 * - from a JStringRef obtained elsewhere. the ref will NOT be released on destruction.
 * - from a C string (char*). the JSStringRef provided will be released on destruction.
 * - from a generic JSValueRef. the JSStringRef provided will be released on destruction.
 *
 * after initialization, jstr provides the referenced char as
 * - ref() which will provide a JStringRef representation of the string
 * - str() which will provide a char* representation of the string
 *
 * allways UTF-8
 */

class jstr
{
public:
    jstr(JSStringRef ref)
        : value_(0), ref_(ref), str_(0)
    {
        get_str();
    }

    jstr(const gchar* str)
        : value_((JSValueRef)1), ref_(0), str_(g_strdup(str))
    {
        ref_ = JSStringCreateWithUTF8CString(str_);
    }

    jstr(JSContextRef context,JSValueRef value)
        :value_(value), ref_(0), str_(0)
    {
        JSValueRef ex = 0;
        ref_ = JSValueToStringCopy(context,value,&ex);
        get_str();
    }

    ~jstr()
    {
        if(value_)
            JSStringRelease(ref_);       
        g_free(str_); 
    }

    gchar* str()
    {
        return str_;
    }

    JSStringRef ref()
    {
        return ref_;
    }

    JSValueRef value()
    {
        return value_;
    }

private:

    void get_str()
    {
        size_t len = JSStringGetMaximumUTF8CStringSize(ref_);
        str_ = g_new(char, len);
        JSStringGetUTF8CString(ref_, str_, len);
    }

    JSValueRef value_;
    JSStringRef ref_;
    gchar* str_;
};


class jnum
{
public:
    jnum(JSContextRef context,JSValueRef value)
    {
        JSValueRef ex = 0;
        value_ = JSValueToNumber(context, value, &ex);           
    }

    int integer()
    {
        return (int)value_;
    }

    double number()
    {
        return value_;
    }

private:
    double value_;
};

class jbool
{
public:
    jbool(JSContextRef context,JSValueRef value)
    {
        value_ = JSValueToBoolean(context, value);           
    }

    bool boolean()
    {
        return value_;
    }

private:
    bool value_ = false;
};

/*
 * deal with custom Javascript Classes. there are three ways to construct a 
 *
 */

class jclass
{
public:

    jclass(const JSClassDefinition* clazz)
    {
        if( map_.count(clazz) == 0 )
        {
            map_.insert( std::make_pair(clazz,JSClassCreate(clazz)) );
        }
        class_ = map_[clazz];
    }

    ~jclass()
    { 
       // no longer free it as we cache it in the map.
       // only one instance per class will be created
       // for the process runtime ever.
       //JSClassRelease(class_);
    }

    JSClassRef ref()
    {
        return class_;
    }

    JSObjectRef create(JSContextRef ctx, void* data)
    {
        return JSObjectMake(ctx, class_, data);
    }

private:
    static std::map<const JSClassDefinition*,JSClassRef> map_;
    JSClassRef class_;
};



inline bool is_js_array(JSContextRef context,JSValueRef jsObject)
{
    jstr scriptJS("return arguments[0].constructor == Array.prototype.constructor");
    JSObjectRef fn = JSObjectMakeFunction(context, NULL, 0, NULL, scriptJS.ref(), NULL, 1, NULL);
    JSValueRef result = JSObjectCallAsFunction(context, fn, NULL, 1, (JSValueRef*)&jsObject, NULL);
   
    bool ret = JSValueToBoolean(context,result);
    return ret;
}

inline bool is_async_function(JSContextRef context,JSValueRef jsObject)
{
    jstr scriptJS("return arguments[0].constructor == Object.getPrototypeOf(async function(){}).constructor");
    JSObjectRef fn = JSObjectMakeFunction(context, NULL, 0, NULL, scriptJS.ref(), NULL, 1, NULL);
    JSValueRef result = JSObjectCallAsFunction(context, fn, NULL, 1, (JSValueRef*)&jsObject, NULL);
   
    bool ret = JSValueToBoolean(context,result);
    return ret;
}

class jsobj;

class jsval 
{
public:

    jsval(JSContextRef context,JSValueRef value)
        : context_(context), value_(value)
    {}

    jsval( const jsval& rhs)
        : context_(rhs.context_), value_(rhs.value_)
    {
    }


    bool isValid()
    {
        return value_ != 0;
    }

    void protect()
    {
         JSValueProtect(context_,value_);
    }

    void unprotect()
    {
         JSValueUnprotect(context_,value_);
    }

    bool isUndefined() 
    {
        return JSValueIsUndefined(context_,value_);
    }

    bool isNull() 
    {
        return JSValueIsNull(context_,value_);
    }

    bool isBoolean()
    {
        return JSValueIsBoolean(context_,value_);
    }

    bool isNumber()
    {
        return JSValueIsNumber(context_,value_);
    }

    bool isString()
    {
        return JSValueIsString(context_,value_);
    }

    bool isObject()
    {
        return JSValueIsObject(context_,value_);
    }

    bool isArray()
    {
        return JSValueIsArray(context_,value_);
    }

    jsobj obj();

    JSType type()
    {
        return JSValueGetType(context_,value_);
    }

    JSValueRef ref()
    {
        return value_;
    }

    JSContextRef ctx()
    {
        return context_;
    }

    std::string str()
    {
         //return g_variant_new_string(jstr(context,argument).str());      
        return jstr(context_,value_).str();
    }

    double number()
    {
        return jnum(context_,value_).number();
    }

    bool boolean()
    {
        return jbool(context_,value_).boolean();
    }


private:
    JSContextRef context_;
    JSValueRef value_;
};

class jskeys
{
public:

    jskeys(JSContextRef context,JSObjectRef obj)
    {
        keys_ = JSObjectCopyPropertyNames(context, obj);        
    }

    jskeys(jsobj& obj);

    jskeys(const jskeys& rhs)
        : keys_(rhs.keys_)
    {
        JSPropertyNameArrayRetain(keys_);
    }

    ~jskeys()
    {
        JSPropertyNameArrayRelease(keys_);
    }

    JSStringRef item(int index)
    {
        return JSPropertyNameArrayGetNameAtIndex(keys_,index);
    }

    int length()
    {
        return JSPropertyNameArrayGetCount(keys_);
    }

private:
    JSPropertyNameArrayRef keys_;
};


class jsobj 
{
public:

    jsobj()
        : context_(0), value_(0)
    {}

    jsobj(JSContextRef context,JSObjectRef value)
        : context_(context), value_(value)
    {}

    jsobj( const jsobj& rhs)
        : context_(rhs.context_), value_(rhs.value_)
    {}

    bool isValid()
    {
        return value_ != 0;
    }

    void protect()
    {
         JSValueProtect(context_,value_);
    }

    void unprotect()
    {
         JSValueUnprotect(context_,value_);
    }    

    jsval member(const std::string& name)
    {
        JSValueRef ex = 0;
        jstr s(name.c_str());

        return jsval(context_,JSObjectGetProperty(context_, value_,s.ref(),&ex));
    }

    jsval member(JSStringRef name)
    {
        JSValueRef ex = 0;
        return jsval(context_,JSObjectGetProperty(context_, value_,name,&ex));
    }

    bool hasMember(const std::string& name)
    {
        jstr s(name.c_str());

        return JSObjectHasProperty(context_, value_,s.ref());
    }

    bool hasMember(JSStringRef name)
    {
        return JSObjectHasProperty(context_, value_,name);
    }

    jsobj& set(const std::string& name, JSValueRef value,JSPropertyAttributes attributes = kJSPropertyAttributeNone)
    {
        JSValueRef ex = 0;
        jstr s(name.c_str());
        JSObjectSetProperty(context_, value_,s.ref(),value,attributes,&ex);
        return *this;
    }

    jsobj& set(JSStringRef name, JSValueRef value,JSPropertyAttributes attributes = kJSPropertyAttributeNone)
    {
        JSValueRef ex = 0;
        JSObjectSetProperty(context_, value_,name,value,attributes,&ex);
        return *this;
    }

    bool isFunction()
    {
        return JSObjectIsFunction(context_,value_);
    }

    jsval invoke(std::vector<JSValueRef>& arguments, JSObjectRef that = NULL)
    {
        JSValueRef ex = 0;
        JSValueRef result = JSObjectCallAsFunction(
            context_,
            value_,
            that,  
            arguments.size(), 
            &arguments[0], 
            &ex
        );
        return jsval(context_,result);
    }

    void* private_data() 
    {
        return JSObjectGetPrivate(value_);
    }

    int length()
    {
        JSValueRef ex = 0;
        jstr lengthPropertyName("length");
        JSValueRef length = JSObjectGetProperty(context_, value_, lengthPropertyName.ref(), &ex);

        int len = jnum(context_,length).integer();
        return len;
    }

    jsval item(int index)
    {
        JSValueRef ex = 0;
        return jsval(context_,JSObjectGetPropertyAtIndex(context_, value_, index, &ex));
    }

    JSContextRef ctx()
    {
        return context_;
    }

    JSObjectRef ref()
    {
        return value_;
    }

    bool isArray()
    {
        return JSValueIsArray(context_,value_);
    }

    void for_each( std::function<void(int index, jsval&)> fun)
    {
        int len = length();
        for ( int i = 0; i < len; i++) 
        {
            jsval val = item(i);
            fun(i,val);
        }
    }

    void for_each( std::function<void(const char*, jsval&)> fun)
    {
        jskeys keys(context_,value_);

        int len = keys.length();
        for ( int i = 0; i < len; i++ )
        {
            jstr key = keys.item(i);
            jsval value = member(key.ref());
            fun(key.str(),value);
        }        
    }

private:
    JSContextRef context_;
    JSObjectRef value_;
};


inline jsobj jsval::obj()
{
    JSValueRef ex = 0;
    return jsobj(context_, JSValueToObject(context_,value_,&ex));
}

inline jskeys::jskeys(jsobj& obj)
{
    keys_ = JSObjectCopyPropertyNames(obj.ctx(), obj.ref());        
}


class jsctx
{
public:

    jsctx(JSContextRef context)
        : context_(context)
    {}

    jsobj globalObject()
    {
        return jsobj(context_,JSContextGetGlobalObject(context_));
    }

    template<class F>
    jsobj make_function(const std::string& name, F fun)
    {
        jstr functionName(name.c_str());
        return make_function(functionName.ref(),fun);
    }


    template<class F>
    jsobj make_function(JSStringRef name, F fun)
    {
        JSObjectRef result = JSObjectMakeFunctionWithCallback(context_, name, fun);
        return jsobj(context_,result);
    }

    JSValueRef undefined()
    {
        return JSValueMakeUndefined(context_);
    }

    jsobj array(std::vector<JSValueRef>& v)
    {
        JSValueRef ex = 0;
        return jsobj(context_,JSObjectMakeArray(context_, v.size(), &v[0], &ex));        
    }

    jsobj object()
    {
        return jsobj(context_, JSObjectMake(context_,0,NULL));
    }

    jsobj object(const JSClassDefinition& clazz, void* user_data = NULL )
    {
        return jsobj(context_,jclass(&clazz).create(context_,user_data));
    }

    JSValueRef string(const std::string& s)
    {
        jstr str(s.c_str());
        return JSValueMakeString(context_, str.ref());
    }

    JSValueRef boolean(bool b)
    {
        return JSValueMakeBoolean(context_, b);
    }

    template<class T>
    JSValueRef number(T t)
    {
        return JSValueMakeNumber(context_,(double)t);
    }

    JSContextRef ctx()
    {
        return context_;
    }

private:
    JSContextRef context_;
};


#endif

