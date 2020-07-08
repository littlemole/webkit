
#ifndef __MO_WEBKIT_JS_GLUE_H__
#define __MO_WEBKIT_JS_GLUE_H__

#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <vector>
#include <map>


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


/*
 * deal with custom Javascript Numbers.
 *
 */

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


#endif

