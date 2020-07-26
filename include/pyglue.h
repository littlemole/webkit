#ifndef _MOL_DEFGUARD_DEFINE_PGLUE_DEF_GUARD_
#define _MOL_DEFGUARD_DEFINE_PGLUE_DEF_GUARD_

#include <Python.h>
#include <structmember.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

class PyGlobalInterpreterLock
{
public:

    PyGlobalInterpreterLock()
    {
        gstate_ = PyGILState_Ensure();
    }

    ~PyGlobalInterpreterLock()
    {
        PyGILState_Release(gstate_);
    }

private:

    PyGILState_STATE gstate_;

};

struct PythonTypeObject : public PyTypeObject 
{
    PythonTypeObject( std::function<void(PyTypeObject&)> fun)
    {

#ifdef Py_TRACE_REFS
        _ob_next = 0;
        _ob_prev = 0;
#endif
        ((PyObject*)(this))->ob_refcnt = 1;
        ((PyObject*)(this))->ob_type = 0;
        ((PyVarObject*)(this))->ob_size = 0;

        tp_name = 0; /* For printing, in format "<module>.<name>" */
        tp_basicsize = 0;
        tp_itemsize = 0; /* For allocation */

        /* Methods to implement standard operations */

        tp_dealloc = 0;
        tp_vectorcall_offset = 0;
        tp_getattr = 0;
        tp_setattr = 0;
        tp_as_async = 0; /* formerly known as tp_compare (Python 2)
                                    or tp_reserved (Python 3) */
        tp_repr = 0;

        /* Method suites for standard classes */

        tp_as_number = 0;
        tp_as_sequence = 0;
        tp_as_mapping = 0;

        /* More standard operations (here for binary compatibility) */

        tp_hash = 0;
        tp_call = 0;
        tp_str = 0;
        tp_getattro = 0;
        tp_setattro = 0;

        /* Functions to access object as input/output buffer */
        tp_as_buffer = 0;

        /* Flags to define presence of optional/expanded features */
        tp_flags = Py_TPFLAGS_DEFAULT;

        tp_doc = ""; /* Documentation string */

        /* call function for all accessible objects */
        tp_traverse = 0;

        /* delete references to contained objects */
        tp_clear = 0;

        /* rich comparisons */
        tp_richcompare = 0;

        /* weak reference enabler */
        tp_weaklistoffset = 0;

        /* Iterators */
        tp_iter = 0;
        tp_iternext = 0;

        /* Attribute descriptor and subclassing stuff */
        tp_methods = 0;
        tp_members = 0;
        tp_getset = 0;
        tp_base = 0;
        tp_dict = 0;
        tp_descr_get = 0;
        tp_descr_set = 0;
        tp_dictoffset = 0;
        tp_init = 0;
        tp_alloc = 0;
        tp_new = PyType_GenericNew;
        tp_free = 0; /* Low-level free-memory routine */
        tp_is_gc = 0; /* For PyObject_IS_GC */
        tp_bases = 0;
        tp_mro = 0; /* method resolution order */
        tp_cache = 0;
        tp_subclasses = 0;
        tp_weaklist = 0;
        tp_del = 0;

        /* Type attribute cache version tag. Added in version 2.6 */
        tp_version_tag = 0;
        tp_finalize = 0;

        fun(*this);
    }
};

struct PythonModuleDef : public PyModuleDef
{
    PythonModuleDef( std::function<void(PyModuleDef&)> fun )
    {
#ifdef Py_TRACE_REFS
        _ob_next = 0;
        _ob_prev = 0;
#endif
        ((PyObject*)(this))->ob_refcnt = 1;
        ((PyObject*)(this))->ob_type = 0;

        m_base.m_init = 0;
        m_base.m_index = 0;
        m_base.m_copy = 0;

        m_name = "dummy name";
        m_doc = "dummy doc";
        m_size = -1;
        m_methods = 0;
        m_slots = 0;
        m_traverse = 0;
        m_clear = 0;
        m_free = 0;

        fun(*this);
    }
};

inline bool py_error()
{
    return PyErr_Occurred() != 0;
}


class pyobj_ref
{
friend class pyobj;
public:

    pyobj_ref()
        : ref_(0)
    {}

    pyobj_ref(PyObject* ref)
        : ref_(ref)
    {}

    pyobj_ref(const pyobj_ref& ref)
        : ref_(ref.ref_)
    {
        if(ref_)
            Py_INCREF(ref_);
    }

    ~pyobj_ref()
    {
        if(ref_)
            Py_DECREF(ref_);
        ref_ = 0;
    }

    PyObject* operator->()
    {
        return ref_;
    }


    pyobj_ref& operator=(const pyobj_ref& rhs)
    {
        if ( this == &rhs )
            return *this;

        if(ref_)
            Py_DECREF(ref_);
        ref_ = rhs.ref_;
        Py_INCREF(ref_);
        return *this;
    }

    pyobj_ref& operator=( PyObject* rhs)
    {
        if ( this->ref_ == rhs )
            return *this;

        if(ref_)
            Py_DECREF(ref_);
        ref_ = rhs;
        return *this;
    }

    operator PyObject* ()
    {
        return ref_;
    }

    operator const PyObject* () const
    {
        return ref_;
    }

    bool isValid()
    {
        return ref_ != 0;
    }

    PyObject* ref()
    {
        return ref_;
    }

    PyObject* incr()
    {
        Py_INCREF(ref_);
        return ref_;
    }

    PyObject* decr()
    {
        Py_DECREF(ref_);
        return ref_;
    }

private:
    PyObject* ref_;

};

class pyobj
{
public:

    pyobj( PyObject* obj)
        : ref_(obj)
    {}

    pyobj( const pyobj_ref& obj)
        : ref_(obj.ref_)
    {}

    bool isValid()
    {
        return ref_ != 0;
    }

    bool isString()
    {
        return PyUnicode_Check(ref_);
    }

    bool isByteArray()
    {
        return PyByteArray_Check(ref_);
    }

    bool isNone()
    {
        return ref_ == Py_None;
    }

    bool isBoolean()
    {
        return PyBool_Check(ref_);
    }


    bool isLong()
    {
        return PyLong_Check(ref_);
    }

    bool isFloat()
    {
        return PyFloat_Check(ref_);
    }

    bool isTuple()
    {
        return PyTuple_CheckExact(ref_);
    }

    bool isList()
    {
        return PyList_CheckExact(ref_);
    }

    bool isDict()
    {
        return PyDict_CheckExact(ref_);
    }

    bool isMethod()
    {
        return PyMethod_Check(ref_);
    }

    bool isFunction()
    {
        return PyFunction_Check(ref_);
    }

    template<class ...Args>
    PyObject* invoke(const char* name, Args* ... args)
    {
        if(!hasAttr(name))
        {
            Py_RETURN_NONE;
        }

        pyobj_ref member = PyUnicode_FromString(name);
        PyObject* ret = PyObject_CallMethodObjArgs(ref_,member,args...,NULL);
        return ret;
    }

    PyObject* invoke_with_tuple(const char* name, PyObject* args)
    {
        if(!hasAttr(name))
        {
            Py_RETURN_NONE;
        }

        int len = pyobj(args).length();

        pyobj_ref tuple = PyTuple_New(len+1);
        pyobj(tuple).item(0,ref_);


        for ( int i = 0; i < len; i++)
        {
            pyobj(tuple).item( i+1, pyobj(args).item(i) );
        }        

        pyobj_ref callable = attr(name);

        // borrowed ref
        PyObject* fun = PyMethod_Function(callable);

        // call python!
        PyObject* ret = PyObject_CallObject(fun,tuple);
        return ret;
    }

    template<class ...Args>
    PyObject* call(Args* ... args)
    {
        std::vector<PyObject*> v{args...};
        int len = v.size();

        pyobj_ref tuple = PyTuple_New(len);
        for( int i = 0; i < len; i++)
        {
            pyobj(tuple).item( i, v[i] );
        }
        // call python!
        PyObject* ret = PyObject_CallObject(ref_,tuple);
        return ret;
    }

    PyObject* call_tuple(PyObject* args)
    {
        // call python!
        PyObject* ret = PyObject_CallObject(ref_,args);
        return ret;
    }

    int length()
    {
        return  PySequence_Size(ref_);
    }

    PyObject* item(int index)
    {
        return PySequence_GetItem(ref_,index);
    }

    void item(int index, PyObject* value)
    {
        Py_INCREF(value); // SetItem steals refs!
        PyTuple_SetItem(ref_,index,value);
    }

    void for_each(std::function<void(int,pyobj_ref&)> fun)
    {
        int len = length();
        for( int i = 0; i < len; i++)
        {
            pyobj_ref value = item(i);
            fun(i,value);
        }
    }

    void for_each(std::function<void(const char*,pyobj_ref&)> fun)
    {
        auto members = keys();
        for( auto& key: members)
        {
            pyobj_ref value = member(key);
            fun( key.c_str(), value);
        }
    }

    long integer() 
    {
        return PyLong_AsLong(ref_); 
    }

    double number()
    {
        return PyFloat_AsDouble(ref_);
    }

    bool boolean()
    {
        return ref_ == Py_True;
    }

    const char* str()
    {
        if(isString())
        {
            return PyUnicode_AsUTF8(ref_);

        }
        else if (isByteArray())
        {
            return PyByteArray_AsString(ref_);
        }

        return "";
    }

    std::vector<std::string> keys()
    {
        std::vector<std::string> ret;
        if(!isDict())
        {
            return ret;
        }

        pyobj_ref keys = PyDict_Keys(ref_);
        Py_ssize_t len = PySequence_Size(keys);

        for(Py_ssize_t i = 0; i < len; i++)
        {
            pyobj_ref key = PySequence_GetItem(keys,i);
            const char* k = PyUnicode_AsUTF8(key);
            ret.push_back(k);
        }  

        return ret;
    }

    bool hasAttr(const std::string& key)
    {
        return PyObject_HasAttrString(ref_, key.c_str() );
    }

    PyObject* attr(const std::string& key)
    {
        return PyObject_GetAttrString(ref_, key.c_str() );
    }

    void attr(const std::string& key, PyObject* value)
    {
        PyObject_SetAttrString(ref_, key.c_str(), value );
    }

    bool hasMember(const std::string& key)
    {
        return PyMapping_HasKeyString(ref_, key.c_str() );
    }

    PyObject* member(const std::string& key)
    {
        return PyMapping_GetItemString(ref_, key.c_str() );
    }

    void member(const std::string& key, PyObject* value)
    {
        PyMapping_SetItemString(ref_, key.c_str(), value );
    }

    PyObject* ref()
    {
        return ref_;
    }

    PyObject* incr()
    {
        Py_INCREF(ref_);
        return ref_;
    }

    PyObject* decr()
    {
        Py_DECREF(ref_);
        return ref_;
    }

    void addObject( const char* name, PyTypeObject* type)
    {
        if (PyType_Ready(type) < 0)
            return;   

        Py_INCREF(type); // AddObject steals if successful
        if(PyModule_AddObject(ref_, name, (PyObject *)type) < 0 )
        {
            Py_DECREF(type);
        }
    }

    void addObject( const char* name, PyObject* obj)
    {
        Py_INCREF(obj); // AddObject steals if successful
        if(PyModule_AddObject(ref_, name, obj) < 0 )
        {
            Py_DECREF(obj);
        }
    }

    void addString( const char* name, const char* value)
    {
        PyModule_AddStringConstant(ref_, name, value );        
    }

private:
    PyObject* ref_;
};

template<class ...Args>
PyObject* ptuple(Args*... args)
{
    std::vector<PyObject*> v{args...};
    int len = v.size();

    pyobj_ref tuple = PyTuple_New(len);
    for( int i = 0; i < len; i++)
    {
        pyobj(tuple).item( i, v[i] );
    }

    return tuple.incr();
}

template<class T>
T* py_alloc(PyTypeObject* clazz)
{   
    return (T *)(clazz)->tp_alloc(clazz, 0);
}


template<class T>
void py_dealloc(T* self)
{   
    Py_TYPE(self)->tp_free((PyObject*)self);    
}


class PyError
{
public:

    PyError()
    {
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    }

    ~PyError()
    {
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);  
        Py_XDECREF(ptraceback);        
    }

    template<class E>
    bool matches(E& ex)
    {
        return PyErr_GivenExceptionMatches(ptype,ex);
    }

    PyObject* ptype = NULL;
    PyObject* pvalue = NULL;
    PyObject* ptraceback = NULL;
};

template<class ...Args>
void incr(Args*... args)
{
    std::vector<PyObject*> v{args...};

    int len = v.size();
    for( int i = 0; i < len; i++)
    {
        Py_XINCREF(v[i]);
    }
}

template<class ...Args>
void decr(Args*... args)
{
    std::vector<PyObject*> v{args...};

    int len = v.size();
    for( int i = 0; i < len; i++)
    {
        Py_XDECREF(v[i]);
    }
}


inline std::string to_json(PyObject* value)
{
    pyobj_ref n = PyUnicode_FromString("json");
    pyobj_ref m = PyImport_GetModule(n);

    pyobj_ref dumps = PyObject_GetAttrString(m,"dumps");
    pyobj_ref json = pyobj(dumps).call(value);

    return pyobj(json).str();
}

inline PyObject* from_json(const std::string& value)
{
    pyobj_ref n = PyUnicode_FromString("json");
    pyobj_ref m = PyImport_GetModule(n);

    pyobj_ref loads = PyObject_GetAttrString(m,"loads");
    pyobj_ref json = PyUnicode_FromString(value.c_str());    
    return pyobj(loads).call(json.ref());
}

extern "C" {

    struct _GObject;
    typedef struct _GObject GObject;
}

inline GObject* gobject(PyObject* obj)
{
    PyObject *capsule = PyObject_GetAttrString(obj, "__gpointer__");
    if(!PyCapsule_CheckExact(capsule))
        return 0;
    const char* capsulename = PyCapsule_GetName(capsule);
    return (GObject*) PyCapsule_GetPointer(capsule, capsulename);
}

#endif
