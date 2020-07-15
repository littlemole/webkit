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
    g_print (PROG "!!!!!!!!!!!!!!!!!!!!!!!future_object_init\n");

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
    g_print (PROG "1 future_set_result\n");

    Py_XDECREF(self->value);

    g_print (PROG "2 future_set_result\n");

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

    g_print (PROG "3 future_set_result\n");

    if( self->cb != Py_None )
    {
        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,(PyObject*)self);

        g_print (PROG "3.6 future_set_result %i\n", (void*)self->cb);

        PyObject_Print(self->cb, stdout,0);
        printf("\n");

        PyObjectRef ret = PyObject_CallObject(self->cb,tuple);
        if(PyErr_Occurred())
        {
            g_print (PROG "ERRRÖR\n");
            PyErr_PrintEx(0);          
        }
    }

    g_print (PROG "4 future_set_result\n");

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
    g_print (PROG "future_add_done_callback %i\n", (void*)args);
        PyObject_Print(args, stdout,0);
        printf("\n");        

    Py_XDECREF(self->cb);


    int len = length(args);
    if ( len < 1 )
    {
        self->cb = Py_None;
        Py_INCREF(self->cb);
    }
    else
    {
        PyObject* cb = item(args,0);
        g_print (PROG "future_add_done_callback %i\n", (void*)cb);
        PyObject_Print(cb, stdout,0);
        printf("\n");        
        self->cb = cb;
    }

    g_print (PROG "-------------->  future_add_done_callback %i\n", (void*)self->cb);

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
    {"exc_", T_OBJECT_EX, offsetof(future_object, ex), 0, "exception" },
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
     g_print (PROG "future_await\n");

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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE ,        /*tp_flags*/
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
    g_print (PROG "*******************future_object_init\n");
    future_object* self = py_alloc<future_object>(&future_objectType);

    self->value = Py_None;
    self->ex = Py_None;
    self->cb = Py_None;
    self->_asyncio_future_blocking = PyBool_FromLong(TRUE);

    Py_INCREF(self->value);
    Py_INCREF(self->ex);
    Py_INCREF(self->cb);

    return (PyObject*)self;
}

void add_future_obj_def(PyObjectRef& m)
{
    if (PyType_Ready(&future_objectType) < 0)
        return;    

    m.addObject("Future",&future_objectType);
}

/////////////////////////////////////////////
// _FutureIter object 

typedef struct {
    PyObject_HEAD //future_object super; // inherit from!
    int state;
    PyObject* future;
} future_iter_object;

static int future_iter_object_init(future_iter_object *self, PyObject *args, PyObject *kwds)
{
         g_print (PROG "!!!!!!!!!!!!!!!!!!!!!!!future_iter_object_init\n");

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
             g_print (PROG "future_iter_object_dealloc\n");


    Py_XDECREF(self->future);
    py_dealloc(self);
}


static PyObject* future_iter_iter(PyObject* self)
{
     g_print (PROG "future_iter_iter\n");
    Py_INCREF(self);
    return self;
}

static PyObject* future_iter_next(PyObject* myself)
{
    future_iter_object* self = (future_iter_object*)myself;

     g_print (PROG "future_iter_next state: %i\n", self->state);

    if(self->state == 0)
    {
        PyObjectRef name = PyUnicode_FromString("done");
             g_print (PROG "name %i \n", (void*)(self->future));
        PyObjectRef done = PyObject_CallMethodObjArgs(self->future,name,NULL);

             g_print (PROG "done");

        if(!done.isValid())
        {
             g_print (PROG "future_iter_next invalid done state: %i\n", self->state);
            return NULL;
        }

         g_print (PROG "future_iter_next check bool state: %i\n", self->state);

        if( done.isBoolean())
        {
            if( done.boolean() == false)
            {
                g_print (PROG "FALSE future_iter_next check bool state: %i\n", self->state);
                Py_INCREF(self->future);
                return self->future;
            }
            else
            {
                g_print (PROG "TRUE future_iter_next check bool state: %i\n", self->state);
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
//        PyObjectRef name = PyUnicode_FromString("result");
  //      PyObjectRef r = PyObject_CallMethodObjArgs(self->future,name,NULL);
  
        PyObjectRef tuple = PyTuple_New(0);
        PyObjectRef r = future_result( (future_object*)self->future, tuple);

        if(!r.isValid())
            return NULL;

        PyErr_SetObject(PyExc_StopIteration,r);
        return NULL;
    }
    
    PyErr_SetString(PyExc_RuntimeError,"XX invalid future iter state\n");
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
    Py_TPFLAGS_DEFAULT,//|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
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
    0,//&future_objectType,                         /* tp_base */
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
     g_print (PROG "new_future_iter_object; %i.\n", (void*)future);
    future_iter_object* self = py_alloc<future_iter_object>(&future_iter_objectType);
    self->state = 0;
    self->future = future;
    Py_INCREF(future);
     g_print (PROG "2 new_future_iter_object; %i.\n", (void*)future);
    return (PyObject*)self;
}

void add_future_iter_obj_def(PyObjectRef& m)
{
    if (PyType_Ready(&future_iter_objectType) < 0)
        return;    

    m.addObject("_FutureIter",&future_iter_objectType);
}


/////////////////////////////////////////////
// Task object 

typedef struct {
    future_object super; // inherit from!
    PyObject* coro;
} task_object;

static PyObject* task_step(task_object* self, PyObject* args);

static void start_coro(task_object* self)
{
    future_object* super = &self->super;

    bool isCoro = PyCoro_CheckExact(self->coro);

    if(!isCoro)
    {
        g_print (PROG "normal function result was passed to task \n");
        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,self->coro);
        PyObjectRef ret = future_set_result(super,tuple);
    }
    else
    {
        g_print (PROG "starting coroutine \n");
        PyObjectRef tuple = PyTuple_New(0);
        PyObjectRef ret = task_step(self,tuple);
    }
}

static int task_object_init(task_object *self, PyObject *args, PyObject *kwds)
{
    g_print (PROG "task_object_init\n");

    future_object* super = &self->super;

    super->value = Py_None;
    super->ex = Py_None;
    super->cb = Py_None;
    super->_asyncio_future_blocking = PyBool_FromLong(TRUE);

    Py_INCREF(super->value);
    Py_INCREF(super->ex);
    Py_INCREF(super->cb);   

    int len = length(args);
    if(len< 1)
    {
        PyErr_SetString(PyExc_RuntimeError,"invalid Task no arguments passed to init");
        return -1;
    }

    self->coro = item(args,0);

    start_coro(self);

    return 0;
}

static void task_object_dealloc(task_object* self)
{
    Py_XDECREF(self->coro);

    future_object* super = &self->super;

    Py_XDECREF(super->value);
    Py_XDECREF(super->ex);
    Py_XDECREF(super->cb);
    Py_XDECREF(super->_asyncio_future_blocking);

    py_dealloc(self);
}


struct StepStruct
{
    PyObjectRef self;
    PyObjectRef ex;
};


gboolean task_do_step(gpointer user_data)
{
     g_print (PROG "start __stepping__\n");

    // GIL ???
     PyGlobalInterpreterLock lock;

    StepStruct* step = (StepStruct*)user_data;

    PyObject* self = step->self.ref();
    PyObject* ex = step->ex.ref();

    task_object* task = (task_object*)self;
    future_object* super = &task->super;

    g_print (PROG "__stepping__\n");
    if(super->done)
    {
        PyErr_SetString(PyExc_RuntimeError,"task is already done!");
        delete step;
        return NULL;
    }

    PyObject* res = 0; // ?????????? decr

    if(super->ex == Py_None)
    {
        g_print (PROG "__stepping__ SEND %i\n", (void*)task->coro);     

        //PyObjectRef name = PyUnicode_FromString("send");
        //res = PyObject_CallMethodObjArgs(task->coro,name, Py_None, NULL);

        res = _PyGen_Send((PyGenObject*)task->coro,Py_None);
    }
    else
    {
        g_print (PROG "__stepping__ THROW\n");    
        PyObjectRef name = PyUnicode_FromString("throw");
        res = PyObject_CallMethodObjArgs(task->coro,name, super->ex, NULL);
    }

    g_print (PROG "__stepping__ TRYD\n");        

    PyObject* err = PyErr_Occurred(); // borrowed ref
    if(err)
    {
        PyObject* ptype = NULL;
        PyObject* pvalue = NULL;
        PyObject* ptraceback = NULL;

        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        if( PyErr_GivenExceptionMatches(ptype,PyExc_StopIteration))
        {
            g_print (PROG "__stepping__ StopIteration\n");    

            // must get "value" attribute of pvalue ???
            if(pvalue)
            {
                PyObjectRef value = PyObject_GetAttrString(pvalue,"value");
                if(super->value != Py_None)
                {
                    Py_DECREF(super->value);
                }
  //              super->value = value;
//                super->done = true;
                future_set_result( super, value);
               // Py_XDECREF(pvalue);
            }

        }
        else
        {
            g_print (PROG "__stepping__ Exceptionn\n");   
            if(pvalue)
            {
                if(super->ex != Py_None)
                {
                    Py_DECREF(super->ex);
                }
//                super->ex = pvalue;
  //              super->done = true;
                future_set_exception( super, pvalue);

            }            
        }

        g_print (PROG "__stepping__ end try catch\n");   

        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);  
        Py_XDECREF(ptraceback);
    }
    else
    {
        g_print (PROG "__stepping__ SUCCESSn\n"); 

        PyObjectRef blocking = PyObject_GetAttrString(res,"_asyncio_future_blocking");
        if(!blocking.isNone())
        {
            g_print (PROG "__stepping__ blocking\n");    

            if(blocking.boolean())
            {
                g_print (PROG "1 __stepping__ blocking TRUE \n");
                PyObject_SetAttrString(res,"_asyncio_future_blocking", PyBool_FromLong(0));

                PyObjectRef func = PyObject_GetAttrString(self,"_wakeup");
                PyObjectRef method = PyMethod_New(func,self);
                //func.incr();
                //method.incr();

                PyObjectRef tuple = PyTuple_New(1);
                tuple.item(0,method);
                PyObjectRef r = future_add_done_callback((future_object*)res,tuple);

               // PyObjectRef name = PyUnicode_FromString("add_done_callback");
                //PyObjectRef r = PyObject_CallMethodObjArgs(res,name, method,NULL);
            }
            g_print (PROG "__stepping__ done with blocking\n");  
        }
        else if(res == Py_None)
        {
            g_print (PROG "__stepping__ non-blocking\n");
            PyObjectRef tuple = PyTuple_New(0);
            PyObjectRef r = task_step(task,tuple);
        }
    }

    g_print (PROG "__stepping__ END\n");

    //delete step;

    return false;
}

static PyObject* task_step(task_object* self, PyObject* args)
{
    g_print (PROG "_task_step\n");

    PyObject* ex = self->super.ex;

    Py_INCREF(self);
    Py_INCREF(ex);

    StepStruct* step = new StepStruct{ (PyObject*)self,ex };
    g_idle_add(task_do_step,step);

    Py_RETURN_NONE;
}


static PyObject* task_wakeup(task_object* self, PyObject* args)
{
    g_print (PROG "_task_wakeup\n");    

    int len = length(args);
    if(len<1)
    {
        g_print (PROG "to few arguments in call to_task_wakeup\n");
        return NULL;
    }

    PyObjectRef arg = item(args,0);

    PyObjectRef name = PyUnicode_FromString("result");
    PyObjectRef ret = PyObject_CallMethodObjArgs(arg,name, NULL);

    g_print (PROG "wakeup try\n");

    PyObject* err = PyErr_Occurred(); // borrowed ref
    if(err)
    {
        g_print (PROG "wakeup err\n");
        PyObject* ptype = NULL;
        PyObject* pvalue = NULL;
        PyObject* ptraceback = NULL;

        PyErr_Fetch(&ptype, &pvalue, &ptraceback);

        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,pvalue);

        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);  
        Py_XDECREF(ptraceback);

        g_print (PROG "wakeup next step with ex\n");
        PyObjectRef ret = task_step(self,tuple);
    }
    else
    {
        PyObjectRef tuple = PyTuple_New(1);
        tuple.item(0,Py_None);

        g_print (PROG "wakeup next step\n");
        PyObjectRef ret = task_step(self,tuple);
    }

    Py_RETURN_NONE;
}


/*
static PyMemberDef future_members[] = {
    {"_asyncio_future_blocking", T_OBJECT_EX, offsetof(future_object, _asyncio_future_blocking), 0, "is blocking future" },
    {NULL}  /* Sentinel * /
};
*/
static PyMethodDef task_methods[] = {
    {"step", (PyCFunction) task_step, METH_VARARGS, "start coro stepping" },
    {"_wakeup", (PyCFunction) task_wakeup, METH_VARARGS, "wakeup coro stepping" },
//    {"_step", (PyCFunction) task_do_step, METH_VARARGS, "coro stepping impl" },
    {NULL}  /* Sentinel */
};

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
PyTypeObject task_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.Task",        /*tp_name*/
    sizeof(task_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)task_object_dealloc,  /*tp_dealloc*/
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
    "task object",       /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    task_methods,                         /* tp_methods */
    0,//future_members,                         /* tp_members */
    0,                         /* tp_getset */
    0,//&future_objectType,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)task_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew                         /* tp_new */
};


extern "C" PyObject* new_task_object(PyObject* coro)
{
    g_print (PROG "new_task_object; %i.\n", (void*)coro);
    task_object* self = py_alloc<task_object>(&task_objectType);
    self->coro = coro;
    Py_INCREF(coro);

    //super().__init__()
    future_object* super = &self->super;

    super->value = Py_None;
    super->ex = Py_None;
    super->cb = Py_None;
    super->_asyncio_future_blocking = PyBool_FromLong(TRUE);

    Py_INCREF(super->value);
    Py_INCREF(super->ex);
    Py_INCREF(super->cb);

    start_coro(self);

    return (PyObject*)self;
}

void add_task_obj_def(PyObjectRef& m)
{
    task_objectType.tp_base = &future_objectType;
    
    if (PyType_Ready(&task_objectType) < 0)
        return;    

    m.addObject("Task",&task_objectType);
}
