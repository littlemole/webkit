#include "pyglue.h"
#include "marshal.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <gio/gio.h>
#include <glib.h>
#include <gdk/gdk.h>

/////////////////////////////////////////////
// globals

#define PROG "[pycoro]"


extern "C" PyObject* new_future_iter_object(PyObject* future);

/////////////////////////////////////////////
// Future object 

typedef struct {
    PyObject_HEAD
    bool done;
    PyObject* value;
    PyObject* ex;
    PyObject* cb;
    PyObject* _asyncio_future_blocking;
} future_object;

static int future_object_init(future_object *self, PyObject *args, PyObject *kwds)
{
    self->value = Py_None;
    self->ex = Py_None;
    self->cb = Py_None;
    self->_asyncio_future_blocking = PyBool_FromLong(TRUE);

    Py_INCREF(self->value);
    Py_INCREF(self->ex);
    Py_INCREF(self->cb);
    return 0;
}

static void future_object_dealloc(future_object* self)
{
    Py_XDECREF(self->value);
    Py_XDECREF(self->ex);
    Py_XDECREF(self->cb);
    Py_XDECREF(self->_asyncio_future_blocking);
    py_dealloc(self);
}


static PyObject* future_done(future_object* self, PyObject* args)
{
    return PyBool_FromLong(self->done);
}

static PyObject* future_set_result(future_object* self, PyObject* args)
{
    Py_XDECREF(self->value);

    int len = length(args);
    if ( len < 1 )
    {
        self->value = Py_None;
        Py_INCREF(self->value);
    }
    else
    {
        self->value = item(args,0);
    }

    self->done = true;

    if( self->cb != Py_None )
    {
        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,(PyObject*)self);

        PyObjectRef ret = PyObject_CallObject(self->cb,tuple);
    }

    Py_RETURN_NONE;
}

static PyObject* future_set_exception(future_object* self, PyObject* args)
{
    Py_XDECREF(self->ex);

    int len = length(args);
    if ( len < 1 )
    {
        self->ex = Py_None;
        Py_INCREF(self->ex);
    }
    else
    {
        self->ex = item(args,0);
    }

    self->done = true;

    if( self->cb != Py_None )
    {
        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,(PyObject*)self);

        PyObjectRef ret = PyObject_CallObject(self->cb,tuple);
    }

    Py_RETURN_NONE;
}

static PyObject* future_add_done_callback(future_object* self, PyObject* args)
{
    Py_XDECREF(self->cb);

    int len = length(args);
    if ( len < 1 )
    {
        self->cb = Py_None;
        Py_INCREF(self->cb);
    }
    else
    {
        self->cb = item(args,0);
    }

    if(self->done)
    {
        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,(PyObject*)self);
        PyObjectRef ret = PyObject_CallObject(self->cb,tuple);
    }

    Py_RETURN_NONE;
}

static PyObject* future_result(future_object* self, PyObject* args)
{
    if(self->ex != Py_None)
    {
        PyErr_SetNone(self->ex);
        return NULL;
    }
    Py_INCREF(self->value);
    return self->value;
}


static PyMemberDef future_members[] = {
    {"_asyncio_future_blocking", T_OBJECT_EX, offsetof(future_object, _asyncio_future_blocking), 0, "is blocking future" },
    {NULL}  /* Sentinel */
};

static PyMethodDef future_methods[] = {
    {"done", (PyCFunction) future_done, METH_VARARGS, "check if future is done" },
    {"set_result", (PyCFunction) future_set_result, METH_VARARGS, "check if future is done" },
    {"set_exception", (PyCFunction) future_set_exception, METH_VARARGS, "check if future is done" },
    {"add_done_callback", (PyCFunction) future_add_done_callback, METH_VARARGS, "check if future is done" },
    {"result", (PyCFunction) future_result, METH_VARARGS, "check if future is done" },
    {NULL}  /* Sentinel */
};

static PyObject* future_await(PyObject* self)
{
    return new_future_iter_object(self);
}

PyAsyncMethods future_AsyncMethods = {
    future_await,
    NULL,
    NULL
};

PyTypeObject future_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.Future",        /*tp_name*/
    sizeof(future_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)future_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,/*tp_getattr*/
    0,/*tp_setattr*/
    &future_AsyncMethods,       /*tp_compare tp_as_async */
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                      /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "future object",       /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    future_methods,                         /* tp_methods */
    future_members,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)future_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew                         /* tp_new */
};

extern "C" PyObject* new_future_object()
{
    future_object* self = py_alloc<future_object>(&future_objectType);
    return (PyObject*)self;
}


/////////////////////////////////////////////
// _FutureIter object 

typedef struct {
    future_object super; // inherit from!
    int state;
    PyObject* future;
} future_iter_object;

static int future_iter_object_init(future_iter_object *self, PyObject *args, PyObject *kwds)
{
    int len = length(args);
    if(len< 1)
    {
        PyErr_SetString(PyExc_RuntimeError,"invalid future iter no arguments passed to init");
        return -1;
    }

    self->state = 0;
    self->future = item(args,0);

    return 0;
}

static void future_iter_object_dealloc(future_iter_object* self)
{
    Py_XDECREF(self->future);
    py_dealloc(self);
}


static PyObject* future_iter_iter(PyObject* self)
{
    Py_INCREF(self);
    return self;
}

static PyObject* future_iter_next(PyObject* myself)
{
    future_iter_object* self = (future_iter_object*)myself;
    if(self->state == 0)
    {
        PyObjectRef name = PyUnicode_FromString("done");
        PyObjectRef done = PyObject_CallMethodObjArgs(self->future,name);
        if(!done.isValid())
            return NULL;

        if( done.isBoolean())
        {
            if( done.boolean() == false)
            {
                Py_INCREF(self->future);
                return self->future;
            }
            else
            {
                self->state = 1;
            }
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError,"invalid future, done is not a bool");
            return NULL;
        }
    }
    if (self->state == 1)
    {
        PyObjectRef name = PyUnicode_FromString("result");
        PyObjectRef r = PyObject_CallMethodObjArgs(self->future,name);
        if(!r.isValid())
            return NULL;

        PyErr_SetObject(PyExc_StopIteration,r);
        return NULL;
    }
    
    PyErr_SetString(PyExc_RuntimeError,"invalid future iter state");
    return NULL;
}

/* 
static PyMemberDef future_members[] = {
    {"_asyncio_future_blocking", T_OBJECT_EX, offsetof(future_object, _asyncio_future_blocking), 0, "is blocking future" },
    {NULL}  /* Sentinel * /
};

static PyMethodDef future_methods[] = {
    {"done", (PyCFunction) future_done, METH_VARARGS, "check if future is done" },
    {"set_result", (PyCFunction) future_set_result, METH_VARARGS, "check if future is done" },
    {"set_exception", (PyCFunction) future_set_exception, METH_VARARGS, "check if future is done" },
    {"add_done_callback", (PyCFunction) future_add_done_callback, METH_VARARGS, "check if future is done" },
    {"result", (PyCFunction) future_result, METH_VARARGS, "check if future is done" },
    {NULL}  /* Sentinel * /
};
*/
/*
static PyObject* future_await(PyObject* self)
{
    return new_future_iter_object(self);
}

PyAsyncMethods future_AsyncMethods = {
    future_await,
    NULL,
    NULL
};
*/
PyTypeObject future_iter_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus._FutureIter",        /*tp_name*/
    sizeof(future_iter_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)future_iter_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,/*tp_getattr*/
    0,/*tp_setattr*/
    0,//future_AsyncMethods,       /*tp_compare tp_as_async */
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                      /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "future iterator object",       /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    future_iter_iter,		                   /* tp_iter */
    future_iter_next,		                   /* tp_iternext */
    0,//future_methods,                         /* tp_methods */
    0,//future_members,                         /* tp_members */
    0,                         /* tp_getset */
    &future_iter_objectType,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)future_iter_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew                         /* tp_new */
};

extern "C" PyObject* new_future_iter_object(PyObject* future)
{
    future_iter_object* self = py_alloc<future_iter_object>(&future_iter_objectType);
    self->future = future;
    Py_INCREF(future);
    return (PyObject*)self;
}

