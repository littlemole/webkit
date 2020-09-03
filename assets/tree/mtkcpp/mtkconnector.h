#ifndef _MOL_DEF_GUARD_DEFINE_G_CONNEtOR_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_G_CONNEtOR_DEF_GUARD_


template<class T, class R, class ... Args>
struct bound_method{
    T* that;
    R(T::*fun)(Args ...);
};

template<class S, class T, class ... Args>
void connect(S* source, const char* signal, void(T::*fun)(Args ...), void* t)
{
    auto bm = new bound_method<T,void,Args...>{ (T*)t, fun };

    auto handler = []( Args ... args, gpointer user_data) -> void
    {
        auto bm = (bound_method<T,void,Args...>*)user_data;
        T* t = (T*) bm->that;
        void(T::*fun)(Args ...) = bm->fun;
        (t->*fun)(args...);
        //delete bm; # dont do this
    };

    void(*ptr)(Args...,gpointer) = handler;

    auto h = reinterpret_cast<void(*)()>(ptr);
    g_signal_connect (source, signal, h, bm);
    
}

template<class S, class R, class T, class ... Args>
void connect(S* source, const char* signal, R(T::*fun)(Args ...), void* t)
{
    auto bm = new bound_method<T,R,Args...>{ (T*)t, fun };

    auto handler = []( Args ... args, gpointer user_data) -> R
    {
        auto bm = (bound_method<T,R,Args...>*)user_data;
        T* t = (T*) bm->that;
        R(T::*fun)(Args ...) = bm->fun;
        return (t->*fun)(args...);
    };

    R(*ptr)(Args...,gpointer) = handler;

    auto h = reinterpret_cast<void(*)()>(ptr);
    g_signal_connect (source, signal, h, bm);    
}

template<class C>
void connector(
    GtkBuilder* builder,
    GObject* object,
    const gchar* signal_name,
    const gchar* handler_name,
    GObject* connect_object,
    GConnectFlags flags,
    void* controller )
{
    meta::find<C>(handler_name, [object,signal_name,controller](auto m)
    {
        g_print("  found: %s  \n", m.name);
        connect(object,signal_name, m.member, (C*)controller);
    });
}


#endif
