#ifndef _MOL_DEFGUARD_DEFINE_PGLUE_DEF_GUARD_
#define _MOL_DEFGUARD_DEFINE_PGLUE_DEF_GUARD_

#include <Python.h>
#include <structmember.h>
#include <string>
#include <vector>
#include <map>


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

class PyObjectRef
{
public:

    PyObjectRef()
        : ref_(0)
    {}

    PyObjectRef(PyObject* ref)
        : ref_(ref)
    {}

    ~PyObjectRef()
    {
        if(ref_)
            Py_DECREF(ref_);
        ref_ = 0;
    }

    PyObject* operator->()
    {
        return ref_;
    }


    PyObjectRef& operator=(const PyObjectRef& rhs)
    {
        if ( this == &rhs )
            return *this;

        if(ref_)
            Py_DECREF(ref_);
        ref_ = rhs.ref_;
        Py_INCREF(ref_);
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

private:
    PyObject* ref_;
};


#endif
