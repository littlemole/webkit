
#ifndef __MO_WEBKIT_GV_GLUE_H__
#define __MO_WEBKIT_GV_GLUE_H__

#include <gio/gio.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

class gstring 
{
public:

    gstring()
    {}

    gstring(gchar* c)
        : str_(c)
    {}

    gstring(const gstring& rhs)
    {
        str_ = g_strdup(rhs.str_);
    }

    gstring(gstring&& rhs)
    {
        str_ = rhs.str_;
        rhs.str_ = 0;
    }

    gstring& operator=(const gstring& rhs)
    {
        if( &(this->str_) == &(rhs.str_) )
        {
            return *this;
        }

        if(str_)
        {
            g_free(str_);
        }
        str_ = g_strdup(rhs.str_);

        return *this;
    }

    gstring& operator=(gstring&& rhs)
    {
        if( &(this->str_) == &(rhs.str_) )
        {
            return *this;
        }

        if(str_)
        {
            g_free(str_);
        }
        str_ = rhs.str_;
        rhs.str_ = 0;

        return *this;
    }

    gstring(const gchar* c)
        : str_( g_strdup(c) )
    {}

    ~gstring()
    {
        if(str_)
        {
            g_free(str_);
            str_ = 0;
        }
    }

    const gchar* str()
    {
        return str_;
    }

    const gchar* operator*()
    {
        return str_;
    }

    gchar** operator&()
    {
        return &str_;
    }

private:
    gchar* str_ = 0;
};

template<class T = GObject>
class gobj
{
public:
    gobj()
        :obj_(0)
    {}

    gobj(GObject* o)
        : obj_((T*)o)
    {}

    template<class P>
    gobj(P* o)
        : obj_( (T*)o)
    {}

    gobj(const gobj& rhs)
    {
        if( this == &rhs)
            return;

        obj_ = rhs.obj_;
        g_object_ref(obj_);
    }

    template<class P>
    P* as()
    {
        return (P*)obj_;
    }

    gobj& operator=(const gobj& rhs)
    {
        if( this == &rhs)
            return * this;
        
        if(obj_)
        {
            g_object_unref(obj_);
        }

        obj_ = rhs.obj_;
        g_object_ref(obj_);
    }

    gobj& operator=(GObject* rhs)
    {
        if( this->obj_ == (T*)rhs)
            return * this;
        
        if(obj_)
        {
            g_object_unref(obj_);
        }

        obj_ = rhs;
    }    

    ~gobj()
    {
        if(obj_)
        {
            g_object_unref(obj_);
            obj_ = 0;
        }
    }

    T* operator->()
    {
        return obj_;
    }

    T* operator*()
    {
        return obj_;
    }

    GObject* ref()
    {
        g_object_ref(obj_);
        return (GObject*)obj_;
    }

    GObject* unref()
    {
        g_object_unref(obj_);
        return (GObject*)obj_;
    }

private:
    T* obj_;    
};


class gval : public GValue 
{
public:

    gval()
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;
    }

    gval(const gval& rhs)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        if(&rhs == this)
            return;

        g_value_copy(&rhs,this);        
    }

    gval(const GValue* gv)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_copy(gv,this);        
    }

    gval& operator=(const gval& rhs)
    {
        if(&rhs == this)
            return *this;

        g_value_unset(this);
        g_value_copy( &rhs,this);        

        return *this;
    }

    gval& operator=(const GValue* gv)
    {
        if(gv == this)
            return *this;

        g_value_unset(this);
        g_value_copy( gv,this);        

        return *this;
    }

    gval(bool i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_BOOLEAN);
        g_value_set_int(this,i);
    }

    bool get_bool()
    {
        return g_value_get_boolean(this);
    }

    gval(int i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_INT);
        g_value_set_int(this,i);
    }

    int get_int()
    {
        return g_value_get_int(this);
    }

    gval(unsigned int i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_UINT);
        g_value_set_uint(this,i);
    }

    unsigned int get_uint()
    {
        return g_value_get_uint(this);
    }

    gval(long i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_LONG);
        g_value_set_long(this,i);
    }

    long get_long()
    {
        return g_value_get_long(this);
    }

    gval(unsigned long i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_ULONG);
        g_value_set_ulong(this,i);
    }

    unsigned long get_ulong()
    {
        return g_value_get_ulong(this);
    }

    gval(long long i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_INT64);
        g_value_set_int64(this,i);
    }

    long long get_int64()
    {
        return g_value_get_int64(this);
    }

    gval(unsigned long long i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_UINT64);
        g_value_set_uint64(this,i);
    }

    unsigned long long get_uint64()
    {
        return g_value_get_uint64(this);
    }


    gval(float i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_FLOAT);
        g_value_set_float(this,i);
    }

    float get_float()
    {
        return g_value_get_float(this);
    }

    gval(double i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_DOUBLE);
        g_value_set_double(this,i);
    }

    double get_double()
    {
        return g_value_get_double(this);
    }

    gval(char * i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_STRING);
        g_value_set_string(this,i);
    }

    gval(const char * i, bool static_string = false)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_STRING);
        if(!static_string)
        {
            g_value_set_string(this,i);
        }
        else 
        {
            g_value_set_static_string(this,i);
        }
    }

    const char* get_cstring()
    {
        return g_value_get_string(this);
    }

    const char* dup_cstring()
    {
        return g_value_dup_string(this);
    }


    gval(const std::string& i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_STRING);
        g_value_set_string(this,i.c_str());
    }

    std::string get_string()
    {
        return g_value_get_string(this);
    }


    gval(gpointer i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_POINTER);
        g_value_set_pointer(this,i);
    }

    gpointer get_pointer()
    {
        return g_value_get_pointer (this);
    }

    gval(GObject* i)
    {
        g_type = 0;
        data[0].v_int = 0;
        data[1].v_int = 0;

        g_value_init(this,G_TYPE_OBJECT);
        g_value_set_object (this,i);
    }

    GObject* get_object()
    {
        return (GObject*)g_value_get_object(this);
    }

    GObject* dup_object()
    {
        return (GObject*)g_value_dup_object(this);
    }

    ~gval()
    {
        g_value_unset(this);
    }

    void unset()
    {
        g_value_unset(this);
    }
};


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

    bool hasMember(const char* key)
    {
        bool res = false;

        for_each( [&res,key](const char* k, GVariant* item)
        {
            if( strcmp(key,k) == 0)
            {
                res = true;
            }
        });
        return res;
    }

    GVariant* member(const char* key)
    {
        GVariant* res = 0;

        for_each( [&res,key](const char* k, GVariant* item)
        {
            if( strcmp(key,k) == 0)
            {
                res = item;
            }
        });
        return res;
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

    GVariant* value()
    {
        return v_;
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
