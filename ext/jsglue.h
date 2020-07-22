
#ifndef __MO_WEBKIT_JS_GLUE_H__
#define __MO_WEBKIT_JS_GLUE_H__

#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

/*
 * deal with Javascript Strings. there are two ways to construct a 
 * String:
 *
 * - from a JStringRef obtained elsewhere. the ref will NOT be released on destruction.
 * - from a C string (char*). the JSStringRef provided will be released on destruction.
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
        : release_ref_(false), ref_(ref), str_(0)
    {
        get_str();
    }

    jstr(const gchar* str)
        : release_ref_(true), ref_(0), str_(g_strdup(str))
    {
        ref_ = JSStringCreateWithUTF8CString(str_);
    }

    jstr(const jstr& rhs)
        : release_ref_(rhs.release_ref_), ref_(0), str_(g_strdup(rhs.str_) )
    {
        if(release_ref_)
        {
            ref_ = JSStringCreateWithUTF8CString(str_);        
        }
        else
        {
            ref_ = rhs.ref_;
        }
    }

    jstr& operator=(const jstr& rhs)
    {
        if(this == &rhs)
            return *this;
        
        release_ref_ = rhs.release_ref_;
        str_ = g_strdup(rhs.str_);

        if(release_ref_)
        {
            ref_ = JSStringCreateWithUTF8CString(str_);        
        }
        else
        {
            ref_ = rhs.ref_;
        }

        return *this;
    }

    ~jstr()
    {
        if(release_ref_)
            JSStringRelease(ref_);       
        if(str_)
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
        if(str_)
        {
            g_free(str_); 
        }
        size_t len = JSStringGetMaximumUTF8CStringSize(ref_);
        str_ = g_new(char, len);
        JSStringGetUTF8CString(ref_, str_, len);
    }

    bool release_ref_;
    JSStringRef ref_;
    gchar* str_;
};


/*
 * deal with custom Javascript Classes. there are three ways to construct a 
 *
 */

inline std::map<const JSClassDefinition*,JSClassRef>&  jsClassMap()
{
    static std::map<const JSClassDefinition*,JSClassRef> map;
    return map;    
}

class jsclassdef : public JSClassDefinition
{
public:
    jsclassdef(std::function<void(JSClassDefinition&)> fun)
    {
        version = 0;
        attributes = kJSClassAttributeNone;
        className = "dummy clazz name";
        parentClass = NULL;
        staticValues = NULL;
        staticFunctions = NULL;
        initialize = NULL;
        finalize = NULL;
        hasProperty = NULL;
        getProperty = NULL;
        setProperty = NULL;
        deleteProperty = NULL;
        getPropertyNames = NULL;
        callAsFunction = NULL;
        callAsConstructor = NULL;
        hasInstance = NULL;
        convertToType = NULL;

        fun(*this);
    }
};

class jclass
{
public:

    jclass(const JSClassDefinition* clazz)
    {
        if( jsClassMap().count(clazz) == 0 )
        {
            jsClassMap().insert( std::make_pair(clazz,JSClassCreate(clazz)) );
        }
        class_ = jsClassMap()[clazz];
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

inline JSValueRef js_to_string(JSContextRef context,JSValueRef jsObject)
{
    jstr scriptJS("return arguments[0].toString()");
    JSObjectRef fn = JSObjectMakeFunction(context, NULL, 0, NULL, scriptJS.ref(), NULL, 1, NULL);
    return JSObjectCallAsFunction(context, fn, NULL, 1, (JSValueRef*)&jsObject, NULL);
   
}


class jsobj;

class jsval 
{
public:

    jsval()
        : context_(0), value_(0)
    {}

    jsval(JSContextRef context,JSValueRef value)
        : context_(context), value_(value)
    {}

    jsval( const jsval& rhs)
        : context_(rhs.context_), value_(rhs.value_)
    {
    }

    jsval& operator=( const jsval& rhs)
    {
        if( this == &rhs )
        {
            return *this;
        }

        context_ = rhs.context_;
        value_ = rhs.value_;
        return *this;
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
        JSValueRef ex = 0;
        JSStringRef ref = JSValueToStringCopy(context_,value_,&ex);
        size_t len = JSStringGetMaximumUTF8CStringSize(ref);
        gchar* str = g_new(char, len);
        JSStringGetUTF8CString(ref, str, len);

        std::string result(str,len);
        JSStringRelease(ref);
        g_free(str);
        return result;
    }

    double number()
    {
        JSValueRef ex = 0;
        double result = JSValueToNumber(context_, value_, &ex);   
        return result;
    }

    int integer()
    {
        JSValueRef ex = 0;
        double result = JSValueToNumber(context_, value_, &ex);   
        return (int)result;
    }

    bool boolean()
    {
        bool value = JSValueToBoolean(context_, value_);     
        return value;
        //return jbool(context_,value_).boolean();
    }


private:
    JSContextRef context_;
    JSValueRef value_;
};

class jskeys
{
public:

    jskeys()
        : context_(0),keys_(0)
    {}

    jskeys(JSContextRef context,JSObjectRef obj)
        : context_(context)
    {
        keys_ = JSObjectCopyPropertyNames(context, obj);        
    }

    jskeys(jsobj& obj);

    jskeys(const jskeys& rhs)
        : context_(rhs.context_),keys_(rhs.keys_)
    {
        if(keys_)
           JSPropertyNameArrayRetain(keys_);
    }

    jskeys& operator=(const jskeys& rhs)
    {
        if(this == &rhs)
        {
            return *this;
        }

        if(keys_)
        {
            JSPropertyNameArrayRelease(keys_);
        }     
        context_ = rhs.context_;

        keys_ = rhs.keys_;
        if(keys_)
        {
            JSPropertyNameArrayRetain(keys_);
        }     
        return *this;
    }

    ~jskeys()
    {
        if(keys_)
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
    JSContextRef context_;
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

        if(ex)
        {
            g_print ("invoke:  ex \n" );
            g_print ("invoke ex:  %s \n", jsval(context_,js_to_string(context_,ex)).str().c_str() );
        }

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

        int len = jsval(context_,length).integer();
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
    context_ = obj.ctx();
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

    jsval undefined()
    {
        return jsval(context_,JSValueMakeUndefined(context_));
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

inline std::string to_json(JSContextRef context,JSValueRef jsObject)
{
    jstr scriptJS("return JSON.stringify(arguments[0]) ");
    JSObjectRef fn = JSObjectMakeFunction(context, NULL, 0, NULL, scriptJS.ref(), NULL, 1, NULL);
    JSValueRef result = JSObjectCallAsFunction(context, fn, NULL, 1, (JSValueRef*)&jsObject, NULL);
   
    jsval s(context,result);
    return s.str();
}

inline jsval from_json(JSContextRef context,const std::string& json)
{
    g_print ("JSON from_json:  %s \n", json.c_str() );

    jstr s(json.c_str());

    JSValueRef str = JSValueMakeString(context,s.ref());

    JSValueRef ex = 0;

    jstr scriptJS("return JSON.parse(arguments[0]) ");
    JSObjectRef fn = JSObjectMakeFunction(context, NULL, 0, NULL, scriptJS.ref(), NULL, 1, &ex);
    JSValueRef ret = JSObjectCallAsFunction(context, fn, NULL, 1, (JSValueRef*)&str, &ex);

    if(ex)
    {
        g_print ("JSON from_json:  ex \n" );
        g_print ("JSON parse ex:  %s \n", jsval(context,js_to_string(context,ex)).str().c_str() );
    }

    return jsval(context,ret);
}


#endif

