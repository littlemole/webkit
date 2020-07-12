
#ifndef __MO_WEBKIT_GV_GLUE_H__
#define __MO_WEBKIT_GV_GLUE_H__

#include <gio/gio.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>


class gvar_builder;

class gvar
{
public:

    gvar(GVariant* v)
        :v_(v)
    {}

    bool isNumber()
    {
        return gvariant_is_number(v_);
    }

    bool isString()
    {
        const GVariantType * t = g_variant_get_type(v_);     
        return g_variant_type_equal(t,G_VARIANT_TYPE_STRING);
    }

    bool isVariant()
    {
        const GVariantType * t = g_variant_get_type(v_);     
        return g_variant_type_equal (t,G_VARIANT_TYPE_VARIANT);
    }

    bool isTuple()
    {
        const GVariantType * t = g_variant_get_type(v_);     
        return g_variant_type_is_tuple (t) ;
    }

    bool isMaybe()
    {
        const GVariantType * t = g_variant_get_type(v_);     
        return g_variant_type_equal(t,G_VARIANT_TYPE_MAYBE);
    }

    bool isBoolean()
    {
        const GVariantType * t = g_variant_get_type(v_);
        return g_variant_type_equal(t,G_VARIANT_TYPE_BOOLEAN);
    }

    bool isDict()
    {
        const GVariantType * t = g_variant_get_type(v_);
        return g_variant_type_equal(t,G_VARIANT_TYPE_VARDICT);
    }

    bool isArray()
    {   
        const GVariantType * t = g_variant_get_type(v_);
        return g_variant_type_equal(t,G_VARIANT_TYPE_ARRAY);        
    }

    double number()
    {
        return gvariant_get_number(v_);
    }

    const char* str()
    {
         return g_variant_get_string (v_,NULL);
    }

    GVariant* variant()
    {
        return g_variant_get_variant(v_);
    }

    GVariant* maybe()
    {
        return g_variant_get_maybe(v_);
    }

    bool boolean()
    {
        return g_variant_get_boolean(v_);  
    }

    GVariant* var()
    {
        return v_;
    }

    int length()
    {
        return g_variant_n_children(v_);        
    }

    GVariant* item(int index)
    {
        return g_variant_get_child_value(v_,index);
    }

    void for_each(std::function<void(gchar*,GVariant*)> fun)
    {
        GVariantIter iter;
        GVariant *value;
        gchar *key;

        g_variant_iter_init (&iter, v_);
        while (g_variant_iter_next (&iter, "{sv}", &key, &value))
        {
            fun(key,value);

            /* must free data for ourselves */
            g_variant_unref (value);
            g_free (key);
        }        
    }

    void for_each(std::function<void(int,GVariant*)> fun)
    {
        int len = length();
        for( int i = 0; i < len; i++)
        {
            GVariant* v = item(i);
            fun(i,v);
            g_variant_unref(v);
        }    
    }    


    static bool gvariant_is_number(GVariant* parameter)
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

    static double gvariant_get_number(GVariant* parameter)
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

private:

    GVariant* v_;
};

class gvar_builder
{
public:
    gvar_builder(const GVariantType* t)
    {
        builder_ = g_variant_builder_new(t);
    }

    gvar_builder& add(const gchar* key, GVariant* v)
    {
        GVariant* dict = g_variant_new("{sv}", key,v);
        g_variant_builder_add_value(builder_,dict);        
        return *this;
    }

    gvar_builder& add(GVariant* v)
    {
        g_variant_builder_add_value(builder_,v);        
        return *this;
    }

    GVariant* build()
    {
        return g_variant_builder_end(builder_);
    }

private:
    GVariantBuilder* builder_;
};

inline GVariant* gnull()
{
    return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
}


inline gvar_builder gtuple()
{
    return gvar_builder(G_VARIANT_TYPE_TUPLE);
}

inline gvar_builder garray()
{
    return gvar_builder(G_VARIANT_TYPE_ARRAY);
}

#endif
