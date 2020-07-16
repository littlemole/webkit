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
    self->done = false;
    self->value = Py_None;
    self->ex = Py_None;
    self->cb = Py_None;
    self->_asyncio_future_blocking = PyBool_FromLong(TRUE);

    incr(self->value, self->ex, self->cb);

    return 0;
}

static void future_object_dealloc(future_object* self)
{
    g_print (PROG "++++ NO FUTURE\n");

    decr(self->value, self->ex, self->cb,self->_asyncio_future_blocking);

    py_dealloc(self);
}


static PyObject* future_object_done(future_object* self, PyObject* args)
{
    return PyBool_FromLong(self->done);
}

static PyObject* future_object_set_result(future_object* self, PyObject* args)
{
    Py_XDECREF(self->value);

    int len = pyobj(args).length();
    if ( len < 1 )
    {
        self->value = Py_None;
        Py_INCREF(self->value);
    }
    else
    {
        self->value = pyobj(args).item(0);
    }

    self->done = true;

    if( self->cb != Py_None )
    {
        pyobj_ref ret = pyobj(self->cb).call((PyObject*)self);

        Py_XDECREF(self->cb);
        self->cb = Py_None;
        Py_INCREF(self->cb);
    }

    Py_RETURN_NONE;
}

static PyObject* future_object_set_exception(future_object* self, PyObject* args)
{
    Py_XDECREF(self->ex);

    int len = pyobj(args).length();
    if ( len < 1 )
    {
        self->ex = Py_None;
        Py_INCREF(self->ex);
    }
    else
    {
        self->ex = pyobj(args).item(0);
    }

    self->done = true;

    if( self->cb != Py_None )
    {
        pyobj_ref ret = pyobj(self->cb).call( (PyObject*)self);

        Py_XDECREF(self->cb);
        self->cb = Py_None;
        Py_INCREF(self->cb);
    }

    Py_RETURN_NONE;
}

static PyObject* future_object_add_done_callback(future_object* self, PyObject* args)
{
    int len = pyobj(args).length();

    Py_XDECREF(self->cb);

    if ( len < 1 )
    {
        self->cb = Py_None;
        Py_INCREF(self->cb);
    }
    else
    {
        PyObject* cb = pyobj(args).item(0);
        self->cb = cb;
        
    }

    if(self->done)
    {
        pyobj_ref ret = pyobj(self->cb).call( (PyObject*)self);

        Py_XDECREF(self->cb);
        self->cb = Py_None;
        Py_INCREF(self->cb);
    }

    Py_RETURN_NONE;
}

static PyObject* future_object_result(future_object* self, PyObject* args)
{
    if(self->ex != Py_None)
    {
        pyobj_ref ptype = PyObject_Type(self->ex);
        PyErr_SetObject(ptype,self->ex);
        return NULL;
    }

    Py_INCREF(self->value);
    return self->value;
}


static PyMemberDef future_members[] = {
    {"exc_", T_OBJECT_EX, offsetof(future_object, ex), 0, "exception" },
    {"_asyncio_future_blocking", T_OBJECT_EX, offsetof(future_object, _asyncio_future_blocking), 0, "is blocking future" },
    {NULL}  /* Sentinel */
};

static PyMethodDef future_methods[] = {
    {"done", (PyCFunction) future_object_done, METH_VARARGS, "check if future is done" },
    {"set_result", (PyCFunction) future_object_set_result, METH_VARARGS, "check if future is done" },
    {"set_exception", (PyCFunction) future_object_set_exception, METH_VARARGS, "check if future is done" },
    {"add_done_callback", (PyCFunction) future_object_add_done_callback, METH_VARARGS, "check if future is done" },
    {"result", (PyCFunction) future_object_result, METH_VARARGS, "check if future is done" },
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
    "WebKitDBus.Future",       /*tp_name*/
    sizeof(future_object),     /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)future_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    &future_AsyncMethods,      /*tp_compare tp_as_async */
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE ,        /*tp_flags*/
    "future object",           /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    future_methods,            /* tp_methods */
    future_members,            /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)future_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew          /* tp_new */
};

extern "C" PyObject* new_future_object()
{
    future_object* self = py_alloc<future_object>(&future_objectType);

    future_object_init(self,NULL,NULL);

    return (PyObject*)self;
}

void add_future_obj_def(pyobj_ref& m)
{
    if (PyType_Ready(&future_objectType) < 0)
        return;    

    pyobj(m).addObject("Future",&future_objectType);
}

/////////////////////////////////////////////
// _FutureIter object 

typedef struct {
    PyObject_HEAD 
    int state;
    PyObject* future;
} future_iter_object;

static int future_iter_object_init(future_iter_object *self, PyObject *args, PyObject *kwds)
{
    int len = pyobj(args).length();
    if(len< 1)
    {
        PyErr_SetString(PyExc_RuntimeError,"invalid future iter no arguments passed to init");
        return -1;
    }

    self->state = 0;
    self->future = pyobj(args).item(0);

    return 0;
}

static void future_iter_object_dealloc(future_iter_object* self)
{
    g_print (PROG "++++ NO FUTURE ITER\n");

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
        pyobj_ref tuple = ptuple();
        pyobj_ref done = future_object_done((future_object*)(self->future),tuple);
        if(!done.isValid())
        {
            PyErr_SetString(PyExc_RuntimeError,"future_iter_next invalid done state");
            return NULL;
        }

        if( pyobj(done).isBoolean())
        {
            if( pyobj(done).boolean() == false)
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
            PyErr_SetString(PyExc_RuntimeError,"invalid future, done is not a bool\n");
            return NULL;
        }
    }
    if (self->state == 1)
    {
        pyobj_ref tuple = ptuple();
        pyobj_ref r = future_object_result( (future_object*)self->future, tuple);

        if(!r.isValid())
            return NULL;

        PyErr_SetObject(PyExc_StopIteration,r);
        return NULL;
    }
    
    PyErr_SetString(PyExc_RuntimeError,"XX invalid future iter state\n");
    return NULL;
}


PyTypeObject future_iter_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus._FutureIter",  /*tp_name*/
    sizeof(future_iter_object),/*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)future_iter_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare tp_as_async */
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "future iterator object",  /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    future_iter_iter,		   /* tp_iter */
    future_iter_next,		   /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)future_iter_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew          /* tp_new */
};

extern "C" PyObject* new_future_iter_object(PyObject* future)
{
    future_iter_object* self = py_alloc<future_iter_object>(&future_iter_objectType);

    pyobj_ref tuple = ptuple(future);
    future_iter_object_init(self,tuple,NULL);

    return (PyObject*)self;
}

void add_future_iter_obj_def(pyobj_ref& m)
{
    if (PyType_Ready(&future_iter_objectType) < 0)
        return;    

    pyobj(m).addObject("_FutureIter",&future_iter_objectType);
}


/////////////////////////////////////////////
// Task object 

typedef struct {
    future_object super; // inherit from!
    PyObject* coro;
} task_object;

static PyObject* task_object_step(task_object* self, PyObject* args);

static void task_start_coro(task_object* self)
{
    future_object* super = &self->super;

    bool isCoro = PyCoro_CheckExact(self->coro);

    if(!isCoro)
    {
        pyobj_ref tuple = ptuple(self->coro);
        pyobj_ref ret = future_object_set_result(super,tuple);
    }
    else
    {
        pyobj_ref tuple = ptuple();
        pyobj_ref ret = task_object_step(self,tuple);
    }
}

static int task_object_init(task_object *self, PyObject *args, PyObject *kwds)
{
    future_object* super = &self->super;
    future_object_init(super,NULL,NULL);

    int len = pyobj(args).length();
    if(len< 1)
    {
        PyErr_SetString(PyExc_RuntimeError,"invalid Task no arguments passed to init");
        return -1;
    }

    self->coro = pyobj(args).item(0);

    task_start_coro(self);

    return 0;
}

static void task_object_dealloc(task_object* self)
{
    g_print (PROG "++++ NO TASK\n");

    future_object* super = &self->super;

    decr(self->coro,super->value,super->ex,super->cb,super->_asyncio_future_blocking);

    py_dealloc(self);
}


struct StepStruct
{
    pyobj_ref self;
    pyobj_ref ex;
};


gboolean task_do_step(gpointer user_data)
{
    PyGlobalInterpreterLock lock;

    StepStruct* step = (StepStruct*)user_data;

    PyObject* self = step->self.ref();
    PyObject* ex = step->ex.ref();

    task_object* task = (task_object*)self;
    future_object* super = &task->super;

    if(super->done)
    {
        PyErr_SetString(PyExc_RuntimeError,"task is already done!");
        delete step;
        return NULL;
    }

    PyObject* res = 0; 

    if(super->ex == Py_None)
    {
        res = _PyGen_Send((PyGenObject*)task->coro,Py_None);
    }
    else
    {
        res = pyobj(task->coro).invoke("throw",PyExc_RuntimeError, super->ex,Py_None);
    }

    if(py_error())
    {
        PyError pe;
        if(pe.matches(PyExc_StopIteration))
        {
            if(pe.pvalue)
            {
                pyobj_ref tuple = ptuple(pe.pvalue);
                future_object_set_result( super, tuple);
            }
            else
            {
                pyobj_ref tuple = ptuple();
                future_object_set_result( super, tuple);
            }
        }
        else
        {
            if(pe.pvalue)
            {
                pyobj_ref tuple = ptuple(pe.pvalue);
                future_object_set_exception( super, tuple);
            }            
        }
    }
    else
    {
        pyobj_ref blocking = PyObject_GetAttrString(res,"_asyncio_future_blocking");
        if(!pyobj(blocking).isNone())
        {
            if(pyobj(blocking).boolean())
            {
                PyObject_SetAttrString(res,"_asyncio_future_blocking", PyBool_FromLong(0));

                pyobj_ref func = PyObject_GetAttrString(self,"_wakeup");
                pyobj_ref method = PyMethod_New(func,self);

                pyobj_ref tuple = ptuple(method.ref());
                pyobj_ref r = future_object_add_done_callback((future_object*)res,tuple);
            }
        }
        else if(res == Py_None)
        {
            pyobj_ref tuple = ptuple();
            pyobj_ref r = task_object_step(task,tuple);
        }
    }

    Py_XDECREF(res);

    delete step;
    return false;
}

static PyObject* task_object_step(task_object* self, PyObject* args)
{
    PyObject* ex = self->super.ex;

    incr( (PyObject*)(self),ex);

    StepStruct* step = new StepStruct{ (PyObject*)self,ex };
    g_idle_add(task_do_step,step);

    Py_RETURN_NONE;
}


static PyObject* task_object_wakeup(task_object* self, PyObject* args)
{
    int len = pyobj(args).length();
    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError,"to few arguments in call to_task_wakeup");
        return NULL;
    }

    pyobj_ref arg = pyobj(args).item(0);
    pyobj_ref ret = pyobj(arg).invoke("result");

    if(py_error())
    {
        PyError pe;
        pyobj_ref tuple = ptuple(pe.pvalue);
        pyobj_ref ret = task_object_step(self,tuple);
    }
    else
    {
        pyobj_ref tuple = ptuple(Py_None);
        pyobj_ref ret = task_object_step(self,tuple);
    }

    Py_RETURN_NONE;
}


static PyMethodDef task_methods[] = {
    {"step", (PyCFunction) task_object_step, METH_VARARGS, "start coro stepping" },
    {"_wakeup", (PyCFunction) task_object_wakeup, METH_VARARGS, "wakeup coro stepping" },
    {NULL}  /* Sentinel */
};

PyTypeObject task_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.Task",         /*tp_name*/
    sizeof(task_object),       /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)task_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare tp_as_async */
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "task object",             /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    task_methods,              /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)task_object_init,/* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew          /* tp_new */
};


extern "C" PyObject* new_task_object(PyObject* coro)
{
    task_object* self = py_alloc<task_object>(&task_objectType);

    pyobj_ref tuple = ptuple(coro);
    task_object_init(self,tuple,NULL);

    return (PyObject*)self;
}

void add_task_obj_def(pyobj_ref& m)
{
    task_objectType.tp_base = &future_objectType;

    if (PyType_Ready(&task_objectType) < 0)
        return;    

    pyobj(m).addObject("Task",&task_objectType);
}
