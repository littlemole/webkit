#include "marshal.h"
#include "gvglue.h"

#define PROG "[web_extension.so]"

GVariant* js_object_to_gvariant(JSContextRef context, JSValueRef arg);
GVariant* js_array_to_gvariant(JSContextRef context, JSValueRef arg);


JSValueRef gvariant_to_js_value(JSContextRef context, GVariant* parameter)
{
    jsctx js(context);

    if (!parameter)
    {
        return js.undefined();
    }

    gvar param(parameter);

    // g_print (PROG " gvariant_to_js_value: %s \n", g_variant_get_type_string(parameter));

    if( param.isString())
    {
        const char* c = param.str();
        return js.string(c);
    }
    else if ( param.isVariant())
    {
        return gvariant_to_js_value(context,param.variant());
    }
    else if ( param.isTuple() )
    {
        std::vector<JSValueRef> ret;

        param.for_each( [&ret,&context](int index, GVariant* item)
        {
            JSValueRef jsval = gvariant_to_js_value(context,item);
            ret.push_back(jsval);
        });

        return js.array(ret).ref();
    }
    else if( param.isMaybe() )
    {
        GVariant* v = param.maybe();
        if(!v)
        {
            return js.undefined();
        }
        else
        {
            JSValueRef ret = gvariant_to_js_value(context,v);
            g_variant_unref(v);
            return ret;
        }
    }
    else if ( param.isBoolean())
    {
       return js.boolean(param.boolean());
    }    
    else if (param.isNumber())
    {
        return js.number(param.number());
    }
    else if ( param.isDict())
    {
        jsobj ret = js.object();

        param.for_each( [&ret,&context] (gchar* key, GVariant* value)
        {
            JSValueRef val = gvariant_to_js_value(context,value);
            ret.set(key,val);
        });
        return ret.ref();      
    }
    else if ( param.isArray())
    {
        std::vector<JSValueRef> ret;

        param.for_each( [&ret,&context](int index, GVariant* item)
        {
            JSValueRef jsval = gvariant_to_js_value(context,item);
            ret.push_back(jsval);
        });
       return js.array(ret).ref();
    }    

    return js.undefined();
}

std::vector<JSValueRef> gvariant_to_js_values(JSContextRef context, GVariant* parameters)
{
    std::vector<JSValueRef> ret;

    gvar params(parameters);

    //g_print (PROG " gvariant_to_js_values %s \n", g_variant_get_type_string (parameters));

    if(!params.isTuple())
    {
        g_print (PROG " not a tuple \n");
        return ret;
    }

    params.for_each( [&ret,&context] (int index, GVariant* v)
    {
        JSValueRef jsval = gvariant_to_js_value(context,v);
        ret.push_back(jsval);
    });

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////

GVariant* js_object_to_gvariant(jsobj& obj)
{
    gvar_builder builder = garray();

    obj.for_each( [&builder] ( const char* key, jsval& value )
    {
        GVariant* v = make_variant(value);
        builder.add(key,v);
    });
 
    return builder.build();
}

GVariant* js_array_to_gvariant(jsobj& arr)
{
    gvar_builder builder = gtuple();

    arr.for_each( [&builder] (int index, jsval& item)
    {
        builder.add(make_variant(item));
    });

    return builder.build();
}

GVariant* make_variant(jsval& arg)
{    
    if(arg.isUndefined())
    {
        return gnull();
    }
    else if (arg.isNull()) 
    {
        return gnull();
    }
    else if ( arg.isString())
    {
        return g_variant_new_string(arg.str().c_str());      
    }
    else if ( arg.isNumber())
    {
        return g_variant_new_double(arg.number());
    }
    else if (arg.isBoolean())
    {
        return g_variant_new_boolean(arg.boolean());    
    }
    else if (arg.isObject())
    {
        jsobj obj = arg.obj();

        bool isFunction = obj.isFunction();
        if(isFunction)
        {
            return gnull();
        }

        bool isArray = obj.isArray();
        if ( isArray )
        {
            return  js_array_to_gvariant(obj);
        }
        else 
        {
            return js_object_to_gvariant(obj);
        }
    }

    return gnull();
}

