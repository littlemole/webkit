#include "marshal.h"

#define PROG "[web_extension.so]"

bool gvariant_is_number(GVariant* parameter)
{
    const GVariantType * t = g_variant_get_type(parameter);      

    if ( g_variant_type_equal (t,G_VARIANT_TYPE_BYTE)    ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT16)   ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT16)  ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT32)   ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT32)  ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT64)   ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT64)  ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_DOUBLE)  ) return true;

    return false;
}

double gvariant_get_number(GVariant* parameter)
{
    const GVariantType * t = g_variant_get_type(parameter);      

    if ( g_variant_type_equal (t,G_VARIANT_TYPE_BYTE)    ) return g_variant_get_byte(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT16)   ) return g_variant_get_int16(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT16)  ) return g_variant_get_uint16(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT32)   ) return g_variant_get_int32(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT32)  ) return g_variant_get_uint32(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT64)   ) return g_variant_get_int64(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT64)  ) return g_variant_get_uint64(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_DOUBLE)  ) return g_variant_get_double(parameter);

    return 0;
}


JSValueRef gvariant_to_js_value(JSContextRef context, GVariant* parameter)
{
    JSValueRef ex = 0;

    if (!parameter)
    {
        return JSValueMakeUndefined(context);
    }

    const GVariantType * t = g_variant_get_type(parameter);      

    // g_print (PROG " gvariant_to_js_value: %s \n", g_variant_get_type_string(parameter));

    if ( g_variant_type_equal (t,G_VARIANT_TYPE_STRING)  ) 
    {
        const char* c = g_variant_get_string (parameter,NULL);
        jstr str(c);

        return JSValueMakeString(context, str.ref());
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_VARIANT) )
    {
        GVariant* v = g_variant_get_variant(parameter);
        return gvariant_to_js_value(context,v);

    }
    else if ( g_variant_type_is_tuple (t) )
    {
        std::vector<JSValueRef> ret;

        gsize s = g_variant_n_children (parameter);
        for( gsize i = 0; i < s; i++)
        {
            GVariant* v = g_variant_get_child_value (parameter,i);
            JSValueRef jsval = gvariant_to_js_value(context,v);
            ret.push_back(jsval);
            g_variant_unref(v);
        }
        return JSObjectMakeArray(context, s, &ret[0], &ex);
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_MAYBE) )
    {
        GVariant* v = g_variant_get_maybe(parameter);
        if(!v)
        {
            return JSValueMakeUndefined(context);
        }
        else
        {
            auto ret = gvariant_to_js_value(context,v);
            g_variant_unref(v);
            return ret;
        }
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_BOOLEAN) )
    {
       return JSValueMakeBoolean(context, g_variant_get_boolean(parameter));
    }    
    else if (gvariant_is_number(parameter) ) 
    {
        return JSValueMakeNumber(context,gvariant_get_number(parameter));
    }
    else if (g_variant_type_equal (t,G_VARIANT_TYPE_VARDICT)) 
    {
        JSObjectRef ret = JSObjectMake(context,0,NULL);

        GVariantIter iter;
        GVariant *value;
        gchar *key;

        g_variant_iter_init (&iter, parameter);
        while (g_variant_iter_next (&iter, "{sv}", &key, &value))
        {
            JSValueRef val = gvariant_to_js_value(context,value);
            jstr str(key);
            JSObjectSetProperty(context, ret, str.ref(), val, kJSPropertyAttributeNone, &ex);            

            /* must free data for ourselves */
            g_variant_unref (value);
            g_free (key);
        }
        return ret;      
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_ARRAY) )
    {
        std::vector<JSValueRef> ret;
        gsize s = g_variant_n_children (parameter);
        for( gsize i = 0; i < s; i++)
        {
            GVariant* v = g_variant_get_child_value (parameter,i);
            JSValueRef jsval = gvariant_to_js_value(context,v);
            ret.push_back(jsval);
            g_variant_unref(v);
        }

        return JSObjectMakeArray(context, s, &ret[0], &ex);
    }    

    return JSValueMakeUndefined(context);
}

std::vector<JSValueRef> gvariant_to_js_values(JSContextRef context, GVariant* parameters)
{
    std::vector<JSValueRef> ret;
    const GVariantType * t = g_variant_get_type(parameters);     

    //g_print (PROG " gvariant_to_js_values %s \n", g_variant_get_type_string (parameters));

    if( !g_variant_type_is_tuple (t) )
    {
        g_print (PROG " not a tuple \n");
        return ret;
    }

    gsize s = g_variant_n_children (parameters);
    for( gsize i = 0; i < s; i++)
    {
        GVariant* v = g_variant_get_child_value (parameters,i);
        JSValueRef jsval = gvariant_to_js_value(context,v);
        ret.push_back(jsval);
    }
    return ret;
}

GVariant* js_object_to_gvariant(JSContextRef context, JSValueRef arg)
{
    JSValueRef ex = 0;
    JSObjectRef obj = JSValueToObject(context, arg, &ex);
    JSPropertyNameArrayRef names = JSObjectCopyPropertyNames(context, obj);

    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    size_t len = JSPropertyNameArrayGetCount(names);
    for ( size_t i = 0; i < len; i++ )
    {
        jstr key = JSPropertyNameArrayGetNameAtIndex(names,i);
        JSValueRef  val = JSObjectGetProperty(context, obj, key.ref(), &ex);

        GVariant* v = make_variant(context,val);

        GVariant* dict = g_variant_new("{sv}", key.str(),v);
        g_variant_builder_add_value(builder,dict);
    }  
    return g_variant_builder_end(builder);
}

GVariant* js_array_to_gvariant(JSContextRef context, JSValueRef arg)
{
    JSValueRef ex = 0;
    JSObjectRef obj = JSValueToObject(context, arg, &ex);
    
    jstr lengthPropertyName("length");
    JSValueRef length = JSObjectGetProperty(context, obj, lengthPropertyName.ref(), &ex);

    int len = jnum(context,length).integer();

    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);

    for ( int i = 0; i < len; i++ )
    {
        JSValueRef  val = JSObjectGetPropertyAtIndex(context, obj, i, &ex);

        g_variant_builder_add(builder, "v", make_variant(context,val));
    }
    
    return g_variant_builder_end(builder);
}

GVariant* make_variant(JSContextRef context, JSValueRef argument)
{    
    JSType jt = JSValueGetType(context, argument);
    if ( jt == kJSTypeUndefined )
    {
        return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
    }
    else if ( jt == kJSTypeNull )
    {
        return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
    }
    else if ( jt ==  kJSTypeString )
    {
        return g_variant_new("s",jstr(context,argument).str());      
    }
    else if ( jt == kJSTypeNumber )
    {
        double num = jnum(context,argument).number();
        return g_variant_new_double(num);
    }
    else if ( jt == kJSTypeBoolean )
    {
        bool b = jbool(context,argument).boolean();
        return g_variant_new_boolean(b);    
    }
    else if ( jt == kJSTypeObject )
    {
        JSValueRef   ex = 0;
        JSObjectRef obj = JSValueToObject(context, argument, &ex);
        //JSObjectRef   t = that ? JSValueToObject(context, that, &ex) : NULL;

        bool isFunction = JSObjectIsFunction(context, obj);
        if(isFunction)
        {
            //return new_js_wrapper_python_object(context,t,obj);
            return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
        }

        bool isArray = is_js_array(context,argument);
        if ( isArray )
        {
            return  js_array_to_gvariant(context,argument);
        }
        else 
        {
            return js_object_to_gvariant(context,argument);
        }
    }

    return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
}

