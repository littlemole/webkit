#ifndef _DEF_GUARD_DEFINE_GPROP_DEF_GUARD_
#define _DEF_GUARD_DEFINE_GPROP_DEF_GUARD_

#include <string>
#include <vector>
#include <memory>

#define PROG "[PywebkitWebview] "

struct GPROP_BASE
{
    GPROP_BASE(GParamSpec* spec)
        : pspec(spec)
    {}

    GParamSpec* pspec;

    virtual ~GPROP_BASE(){}

    virtual void get(GValue* v, void* that) = 0;
    virtual void set(void* that, const GValue* v) = 0;

    void set_value( char* c, const GValue* v)
    {
        g_free(c);
        c = g_value_dup_string(v);
    }

    void set_value( int& i, const GValue* v)
    {
        i = g_value_get_int(v);
    }

    void set_value( bool& c, const GValue* v)
    {
        c = g_value_get_boolean(v);
    }

    void set_value( unsigned int& i, const GValue* v)
    {
        i = g_value_get_uint(v);
    }

    void set_value( double& i, const GValue* v)
    {
        i = g_value_get_double(v);
    }

    void set_value( float& i, const GValue* v)
    {
        i = g_value_get_float(v);
    }

    void get_value( GValue* v, const char* c)
    {
        g_value_set_string( v, c);
    }

    void get_value( GValue* v, int& i)
    {
        g_value_set_int( v, i);
    }

    void get_value( GValue* v, bool& i)
    {
        g_value_set_boolean( v, i);
    }

    void get_value( GValue* v, unsigned int& i)
    {
        g_value_set_uint( v, i);
    }

    void get_value( GValue* v, double& i)
    {
        g_value_set_double( v, i);
    }

    void get_value( GValue* v, float& i)
    {
        g_value_set_float( v, i);
    }

};

template<class T, class M>
struct GPROP : public GPROP_BASE
{
    M T::* member;

    GPROP( M T::* m, GParamSpec* spec)
        : GPROP_BASE(spec), member(m)
    {}

    virtual void get(GValue* v, void* that)
    {
        T* t = (T*) that;
        this->get_value( v, (t->*member) );
    }

    virtual void set(void* that, const GValue* v)
    {
        T* t = (T*) that;
        this->set_value( (t->*member), v);
    }

};


template<class T, class M>
GPROP_BASE* gprop( M T::*member, GParamSpec* spec )
{
    return new GPROP{ member, spec };
}

template<class T>
class gprops 
{
public:

    template<class ... Args>
    gprops(Args ... args)
    {
        std::vector<GPROP_BASE*> v{args...};
        for( auto i : v )
        {
            props_.push_back( std::unique_ptr<GPROP_BASE>(i) );
        }
            }

    void install(T* klass)
    {
        std::vector<GParamSpec*> params;
        params.push_back(NULL);

        for ( auto& i : props_) 
        {
            params.push_back(i->pspec);
        }

        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        object_class->set_property = &set_property;
        object_class->get_property = &get_property;

        g_object_class_install_properties(object_class, params.size(), &params[0] );
    }

private:

    gprops() {}

    static void set_property (GObject* object, guint property_id, const GValue *value, GParamSpec *pspec)
    {
        if( property_id >= props_.size() )
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);      
            return;      
        }

        GPROP_BASE* p = props_[property_id].get();

        p->set(object,value);
    }

    static void get_property (GObject* object, guint property_id, GValue* value, GParamSpec *pspec)
    {
        if( property_id >= props_.size() )
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);      
            return;      
        }

        GPROP_BASE* p = props_[property_id].get();

        p->get(value,object);
    }


    static std::vector<std::unique_ptr<GPROP_BASE>> props_;
};

template<class T>
inline std::vector<std::unique_ptr<GPROP_BASE>> gprops<T>::props_;


///////////////////////////////////////////////////////////////////////////////////////////////////////

class Signals
{
public:
    Signals(GObjectClass* klazz)
        :clazz(klazz)
    {}

    template<class ... Args>
    int install (const std::string& name, Args ... args )
    {
        return install( name,G_TYPE_NONE, (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS), args ...);
    }

    template<class ... Args>
    int install (const std::string& name, GSignalFlags flags, Args ... args )
    {
        return install( name,G_TYPE_NONE, flags, args ...);
    }

    template<class ... Args>
    int install (const std::string& name, GType return_type, GSignalFlags flags, Args ... args )
    {
        std::vector<GType> params{ args... };

        int signalId = g_signal_newv (
            name.c_str(),
            G_TYPE_FROM_CLASS (clazz),
            flags,
            NULL /* closure */,
            NULL /* accumulator */,
            NULL /* accumulator data */,
            NULL, //g_cclosure_marshal_VOID__STRING /* C marshaller */,
            return_type /* return_type */,
            params.size()     /* n_params */,
            &params[0]  /* param_types */
        );

        g_print( PROG "registed changed signal id: %i\n", signalId );

        return signalId;
    }

private:
    GObjectClass* clazz;
};


#endif
