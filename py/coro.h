#ifndef _MOL_DEF_DEFINE_GUARD_WEBKIT_CORO_DEF_
#define _MOL_DEF_DEFINE_GUARD_WEBKIT_CORO_DEF_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <map>

#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

#include "pyglue.h"
#include "gvglue.h"

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

/////////////////////////////////////////////
// _FutureIter object 

typedef struct {
    PyObject_HEAD 
    int state;
    PyObject* future;
} future_iter_object;

/////////////////////////////////////////////
// Task object 

typedef struct {
    future_object super; // inherit from!
    PyObject* coro;
} task_object;

/////////////////////////////////////////////

extern "C" PyObject* new_future_object();
extern "C" PyObject* new_future_iter_object(PyObject* future);
extern "C" PyObject* new_task_object(PyObject* coro);

void add_future_obj_def(pyobj_ref& m);
void add_future_iter_obj_def(pyobj_ref& m);
void add_task_obj_def(pyobj_ref& m);


#endif