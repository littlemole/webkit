#include "coro.h"

/////////////////////////////////////////////
// globals

#define PROG "[pydbus]"

static const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_req_prefix = "/org/oha7/webkit/WebKitDBus/controller/request/";
static const std::string dbus_object_path_recv_req_prefix = "/org/oha7/webkit/WebKitDBus/view/request/";
static const std::string dbus_object_path_send_res_prefix = "/org/oha7/webkit/WebKitDBus/controller/response/";
static const std::string dbus_object_path_recv_res_prefix = "/org/oha7/webkit/WebKitDBus/view/response/";

static std::string dbus_object_path_send_req_path;
static std::string dbus_object_path_recv_req_path; 

static std::string dbus_object_path_send_res_path;
static std::string dbus_object_path_recv_res_path; 


static GDBusConnection* dbus = 0;
static std::string sid;
static std::string rsid;
static pyobj_ref cb;

/////////////////////////////////////////////
// forwards

static void send_dbus_signal( GDBusConnection* dbus, gchar* uid, std::string s, GVariant*  params);

class Responses
{
public:

    void add(const char* uid, PyObject* p)
    {
        pyobj(p).incr();
        pending_.insert( std::make_pair(std::string(uid),p) );

        g_print (PROG "responses add %s\n", uid );
    }

    PyObject* get(const char* uid)
    {
        g_print (PROG "responses get %s\n", uid );

        if ( pending_.count(std::string(uid)) == 0)
        {
            return 0;
        }
        PyObject* res = pending_[std::string(uid)];
        pending_.erase(uid);
        return res;
    }

private:

    std::map<std::string,PyObject*> pending_;
};

Responses& responses()
{
    static Responses r;
    return r;
}

/////////////////////////////////////////////
// responseCallback object - function object that 
// emits signal when invoked

typedef struct {
    PyObject_HEAD
    std::string uid;
} responseCB_object;

static int responseCB_object_init(responseCB_object *self, PyObject *args, PyObject *kwds)
{

    pyobj arguments(args);
    if(arguments.length()<1)
    {
        PyErr_SetString(PyExc_RuntimeError,"invalid responseCB_object_init no uid passed to init");
        return -1;        
    }

    
    pyobj_ref uid = arguments.item(0);
    self->uid = std::string(pyobj(uid).str());
    return 0;
}

static void responseCB_object_dealloc(responseCB_object* self)
{
    py_dealloc(self);
}

static void send_dbus_response( GDBusConnection* dbus, const gchar* uid,  PyObject*  value, const char* ex = NULL);

static PyObject* responseCB_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    g_print (PROG "responseCB_object_call \n" );

    responseCB_object* that = (responseCB_object*)self;

    int len = pyobj(args).length();

    g_print (PROG "responseCB_object_call %i \n" ,len);

    if(len==0)
    {
        pyobj_ref none(Py_None);
        none.incr(); // hack
        send_dbus_response(dbus,that->uid.c_str(),none);
    }
    else
    {
        pyobj_ref arg = pyobj(args).item(0);

        pyobj_ref r = pyobj(arg).invoke("result");

        if(py_error())
        {
//            PyErr_Print();
            PyError err;
            pyobj_ref msg = PyObject_Str(err.pvalue);
            send_dbus_response(dbus,that->uid.c_str(),NULL,pyobj(msg).str());
        }
        else
        {
            send_dbus_response(dbus,that->uid.c_str(),r);
        }

        g_print (PROG "done responseCB_object_call %i \n" ,len);
    }
    
    Py_RETURN_NONE;
}

PyTypeObject responseCB_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.responseCB_object", /*tp_name*/
    sizeof(responseCB_object),     /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)responseCB_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    responseCB_object_call,        /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "response callback wrapper objects",  /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)responseCB_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew          /* tp_new */
};


extern "C" PyObject* new_responseCB_object(const char* uid)
{
    responseCB_object* self = py_alloc<responseCB_object>(&responseCB_objectType);

    pyobj_ref id = PyUnicode_FromString(uid);
    pyobj_ref tuple = ptuple(id.ref());

    responseCB_object_init(self,tuple.ref(),(PyObject*)NULL);
    return (PyObject*)self;
}



/////////////////////

/////////////////////////////////////////////
// signal object - function object that 
// emits signal when invoked

typedef struct {
    PyObject_HEAD
    std::string signal_name;
} signal_object;

static int signal_object_init(signal_object *self, PyObject *args, PyObject *kwds)
{
    pyobj arguments(args);
    if(arguments.length()<1)
    {
        PyErr_SetString(PyExc_RuntimeError,"invalid signal_object_init no signal name passed to init");
        return -1;        
    }
    
    pyobj_ref signal = arguments.item(0);
    self->signal_name = pyobj(signal).str();
    return 0;
}

static void signal_object_dealloc(signal_object* self)
{
    py_dealloc(self);
}

static PyObject* signal_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    signal_object* that = (signal_object*)self;

    std::string json = to_json(args);
    GVariant* param = g_variant_new_string(json.c_str());

    gchar* uid = g_dbus_generate_guid();

    send_dbus_signal(dbus,uid,that->signal_name,param);
    
    PyObject* future = new_future_object();
    responses().add(uid,future);

    g_free(uid);

    return future;
}

PyTypeObject signal_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.SignalObject", /*tp_name*/
    sizeof(signal_object),     /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)signal_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    signal_object_call,        /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "signal wrapper objects",  /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)signal_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew          /* tp_new */
};


extern "C" PyObject* new_signal_object(const char* name)
{
    signal_object* self = py_alloc<signal_object>(&signal_objectType);

    pyobj_ref signal = PyUnicode_FromString(name);
    pyobj_ref tuple = ptuple(signal.ref());

    signal_object_init(self,tuple,NULL);
    return (PyObject*)self;
}

/////////////////////
// signals object is a helper to invoke signals

typedef struct {
    PyObject_HEAD
} signals_object;

static int signals_object_init(signals_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void signals_object_dealloc(signals_object* self)
{
   py_dealloc(self);
}

static PyObject * signals_object_getattr(signals_object* self, char* name)
{
    return new_signal_object(name);
}
 

PyTypeObject signals_objectType = {
    PyVarObject_HEAD_INIT(NULL,0)
    "WebKitDBus.SignalsObject",/*tp_name*/
    sizeof(signals_object),    /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)signals_object_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    (getattrfunc)signals_object_getattr, /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
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
    "signals objects",         /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)signals_object_init, /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};

extern "C" PyObject* new_signals_object()
{
    signals_object* self = py_alloc<signals_object>(&signals_objectType);

    return (PyObject*)self;
}
///////////////////////////////////

static void response_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    g_print (PROG " received response %s %s\n", signal_name, g_variant_get_type_string (parameters));

    PyGlobalInterpreterLock lock;

    gvar params(parameters);

    int len = params.length();
    if(len < 2)
    {
        g_print (PROG " received invalid response tuple %s %s\n", signal_name, g_variant_get_type_string (parameters));
        return;
    }

    gvar uid = params.item(0);
    gvar args = params.item(1);

    std::string json = args.str();

    g_print (PROG "response_handler has %s %s \n", uid.str(), json.c_str() );

    pyobj_ref dict = from_json(json);

    pyobj_ref result;
    if( pyobj(dict).hasMember("result") ) 
    {
        result = pyobj(dict).member("result");
    }

    pyobj_ref ex;
    if( pyobj(dict).hasMember("exception") )
    {
        ex = pyobj(dict).member("exception");
    }

    pyobj_ref future = responses().get( uid.str() );

    if(future.isValid())
    {
        g_print (PROG "response_handler has result \n" );
        if(ex.isValid() && !pyobj(ex).isNone() )
        {
            g_print (PROG "response_handler has ex \n" );
    PyObject_Print(ex.ref(), stdout,0);
    printf("\n");            
            pyobj_ref ret = pyobj(future).invoke("set_exception",ex.ref());
        }
        else
        {
            g_print (PROG "response_handler has result \n" );
            if(py_error())
            {
                PyErr_Print();
//                PyError err;
  //              g_print (PROG "ex setting result %s \n", pyobj(err.pvalue).str() );
            }            
            pyobj_ref ret = pyobj(future).invoke("set_result",result.ref());
            if(py_error())
            {
                PyErr_Print();
//                PyError err;
  //              g_print (PROG "ex setting result %s \n", pyobj(err.pvalue).str() );
            }
        }
    }
    else
    {
        g_print (PROG " received invalid future %s %s\n", signal_name, g_variant_get_type_string (parameters));

    }
}


///////////////////////////////////

extern PyTypeObject future_objectType;
extern PyTypeObject task_objectType;
static PyObject* pywebkit_run_async(PyObject* self, PyObject* args);

static void signal_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    g_print (PROG " received signal %s %s\n", signal_name, g_variant_get_type_string (parameters));

    PyGlobalInterpreterLock lock;

    gvar params(parameters);
    int len = params.length();
    if(len < 2)
    {
        g_print (PROG " received invalid signal %s %s\n", signal_name, g_variant_get_type_string (parameters));
        return;
    }

    gvar guid = params.item(0);
//    pyobj_ref uid = gvariant_to_py_value(params.item(0));
    g_print (PROG "received signal %s %s\n", signal_name, guid.str() );

//    PyObject* result = 0;
    pyobj_ref result;

    gvar gargs = params.item(1);
    std::string data = gargs.str();
    pyobj_ref args = from_json(data);

    result = pyobj(cb).invoke_with_tuple(signal_name, args);

    if(!py_error())
    {
        pyobj_ref tuple = ptuple(result.ref());
        result = pywebkit_run_async(NULL,tuple);
    }

    if(py_error())
    {
        PyError err;
        pyobj_ref msg = PyObject_Str(err.pvalue);
        send_dbus_response(dbus,guid.str(),NULL,pyobj(msg).str() );

    }
    else
    if(result.isValid())
    {
        if( (Py_TYPE(result) == &future_objectType) || (Py_TYPE(result) == &task_objectType) )
        {
            g_print (PROG "signal handler result is future \n" );
             
            pyobj_ref handler = new_responseCB_object(guid.str());
            pyobj_ref ret = pyobj(result).invoke("add_done_callback", handler.ref() );

            if(PyErr_Occurred())
            {
                g_print (PROG "ERRRÖR\n");
                PyErr_PrintEx(0);          
            }             
        }
        else
        {
             g_print (PROG "signal handler result is not a future\n" );

             send_dbus_response(dbus,guid.str(),result);
        }
    }
}

static void got_dbus (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    dbus =  g_bus_get_finish (res, NULL);    

    sid = g_dbus_connection_signal_subscribe (
        dbus, 
        /*sender*/ NULL, 
         dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
        dbus_object_path_recv_req_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL
    );    

    rsid = g_dbus_connection_signal_subscribe (
        dbus, 
        /*sender*/ NULL, 
         dbus_interface.c_str(),
        /*const gchar *member*/ NULL,
        dbus_object_path_recv_res_path.c_str(),
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &response_handler,
        NULL,
        NULL
    );    

}

static void send_dbus_response( GDBusConnection* dbus, const gchar* uid,  PyObject* val, const char* ex )
{
    pyobj value(val);

    PyObject_Print(value.ref(), stdout,0);
    printf("\n");

    gvar_builder builder = gtuple();
    builder.add(g_variant_new_string(uid));

    pyobj_ref dict = PyDict_New();
    pyobj(dict).member("result",Py_None);
    pyobj(dict).member("exception",Py_None);

    if(value.isValid())
    {
        pyobj(dict).member( "result", value.ref() );
    }
    if(ex != 0)
    {
        pyobj(dict).member("exception",PyUnicode_FromString(ex) );
    }

    std::string json = to_json(dict);
    builder.add( g_variant_new_string( json.c_str() ) );

    GVariant* parameters = builder.build();

    g_print (PROG "send_dbus_response : %s %s\n", uid, g_variant_get_type_string (parameters));

    g_dbus_connection_emit_signal(
        dbus,
        NULL,
        dbus_object_path_send_res_path.c_str(),
        dbus_interface.c_str(),
        "response",
        parameters,
        NULL
    );
}

static void send_dbus_signal( GDBusConnection* dbus, gchar* uid, std::string s, GVariant*  params)
{
    gvar_builder builder = gtuple();
    builder.add(g_variant_new_string(uid));
    if(params)
    {
        builder.add(params);
    }
    GVariant* parameters = builder.build();

    g_dbus_connection_emit_signal(
        dbus,
        NULL,
        dbus_object_path_send_req_path.c_str(),
        dbus_interface.c_str(),
        s.c_str(),
        parameters,
        NULL
    );
}

static PyObject* pywebkit_send_signal(PyObject* self, PyObject* args)
{
    int len = pyobj(args).length();

    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than one arg for call to send_signal!");
        return NULL;
    }
    
    pyobj_ref signal = pyobj(args).item(0);
    const char* signal_name = PyUnicode_AsUTF8(signal);

    pyobj_ref tuple = PyTuple_New(len-1);
    for( int i = 1; i < len; i++)
    {
        pyobj(tuple).item(i-1,pyobj(args).item(i));
    }

    std::string json = to_json(tuple);
    GVariant* param = g_variant_new_string(json.c_str());
    
    gchar* uid = g_dbus_generate_guid();

    send_dbus_signal(dbus,uid,signal_name,param);

    PyObject* future = new_future_object();

    responses().add(uid,future);

    g_free(uid);
    return future;
}

static PyObject* pywebkit_bind(PyObject* self, PyObject* args)
{
    int len = pyobj(args).length();

    g_print (PROG "on signal \n");

    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than one args for call to on_signal!");
        return NULL;
    }

    cb = pyobj(args).item(0);

    Py_RETURN_NONE;
}

extern "C" PyObject* new_task_object(PyObject* coro);

struct run_async_cb_struct 
{
    pyobj_ref cb;
    pyobj_ref task;
};

static gboolean run_async_cb(gpointer user_data)
{
    PyGlobalInterpreterLock lock;

    run_async_cb_struct* data = (run_async_cb_struct*)user_data;

    pyobj_ref done = pyobj(data->cb).invoke("done",data->task.ref());

    delete data;
    return false;
}

static PyObject* pywebkit_run_async(PyObject* self, PyObject* args)
{
    int len = pyobj(args).length();

    if(len<1)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than one args for call to pywebkit_run_async!");
        return NULL;
    }

    pyobj_ref coro = pyobj(args).item(0);

    pyobj_ref task = new_task_object(coro);

    if(len>1)
    {
        pyobj_ref done_cb = pyobj(args).item(1);
    
        pyobj_ref done = pyobj(task).invoke("done");

        if(pyobj(done).isValid() && pyobj(done).boolean() == true)
        {
            task.incr();
            done_cb.incr();
            run_async_cb_struct* racs = new run_async_cb_struct{ done_cb, task };
            g_idle_add(run_async_cb,racs);
        }
        else
        {
            pyobj_ref ret = pyobj(task).invoke("add_done_callback", done_cb.ref());
            if(PyErr_Occurred())
            {
                g_print (PROG "ERRRÖR\n");
                PyErr_PrintEx(0);          
            }
        }
    }
    return task.incr();
}

static PyMethodDef pywebkit_module_methods[] = {
    {"send_signal",  pywebkit_send_signal, METH_VARARGS, "Send a signal to webview child process via dbus."},
    {"bind",  pywebkit_bind, METH_VARARGS, "register listening for signal via dbus."},
    {"run_async",  pywebkit_run_async, METH_VARARGS, "run a coroutine."},
    {NULL}  /* Sentinel */
};



static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "WebKitDBus",
    "dbus interface",
    -1,
    pywebkit_module_methods,
    NULL,NULL,NULL,NULL
};

void add_future_obj_def(pyobj_ref& m);
void add_future_iter_obj_def(pyobj_ref& m);
void add_task_obj_def(pyobj_ref& m);

PyMODINIT_FUNC PyInit_WebKitDBus(void) 
{
    // ready guards
    if (PyType_Ready(&signal_objectType) < 0)
        return 0;    

    if (PyType_Ready(&signals_objectType) < 0)
        return 0;    

    if (PyType_Ready(&responseCB_objectType) < 0)
        return 0;    

    // generate guid
    gchar* c = g_dbus_generate_guid();
    sid = std::string(c);
    g_free(c);

    // assemble module config
    std::ostringstream oss_send;
    oss_send << dbus_object_path_send_req_prefix << sid;
    dbus_object_path_send_req_path = oss_send.str();

    std::ostringstream oss_recv;
    oss_recv << dbus_object_path_recv_req_prefix << sid;
    dbus_object_path_recv_req_path = oss_recv.str();

    std::ostringstream oss_res_send;
    oss_res_send << dbus_object_path_send_res_prefix << sid;
    dbus_object_path_send_res_path = oss_res_send.str();

    std::ostringstream oss_res_recv;
    oss_res_recv << dbus_object_path_recv_res_prefix << sid;
    dbus_object_path_recv_res_path = oss_res_recv.str();

    g_print (PROG "Interface; %s.\n", dbus_interface.c_str());
    g_print (PROG "Send; %s.\n", dbus_object_path_send_req_path.c_str());
    g_print (PROG "Recv; %s.\n", dbus_object_path_recv_req_path.c_str());

    // acquire dbus session
    g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    // create and populate module
    pyobj_ref m = PyModule_Create(&moduledef);

    pyobj(m).addString( "uid", sid.c_str());

    pyobj(m).addObject("SignalObject", &signal_objectType);
    pyobj(m).addObject("SignalsObject", &signals_objectType);

    add_future_obj_def(m);
    add_future_iter_obj_def(m);    
    add_task_obj_def(m);
    
    pyobj_ref signalsObject = new_signals_object();
    pyobj(m).addObject("View", signalsObject);

    return m.incr();
}




