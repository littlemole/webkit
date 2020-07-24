#include "coro.h"

/////////////////////////////////////////////
// externs

extern PyTypeObject future_objectType;
extern PyTypeObject task_objectType;
extern "C" PyObject* new_task_object(PyObject* coro);

/////////////////////////////////////////////
// globals

#define PROG "[pydbus]"

static const std::string dbus_interface = "org.oha7.webkit.WebKitDBus";
static const std::string dbus_object_path_send_req_prefix = "/org/oha7/webkit/WebKitDBus/controller/request/";
static const std::string dbus_object_path_recv_req_prefix = "/org/oha7/webkit/WebKitDBus/view/request/";
static const std::string dbus_object_path_send_res_prefix = "/org/oha7/webkit/WebKitDBus/controller/response/";
static const std::string dbus_object_path_recv_res_prefix = "/org/oha7/webkit/WebKitDBus/view/response/";
/*
static std::string dbus_object_path_send_req_path;
static std::string dbus_object_path_recv_req_path; 

static std::string dbus_object_path_send_res_path;
static std::string dbus_object_path_recv_res_path; 
*/

static GDBusConnection* dbus = 0;
//static std::string sid;
//static std::string rsid;
//static pyobj_ref cb;
static PyObject* module;

/////////////////////////////////////////////
// forwards

class Channel;

static PyObject* pywebkit_run_async(
    PyObject* self, 
    PyObject* args
);

static void response_handler(
    GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data
);

static void signal_handler(
    GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data
);

static void send_dbus_signal( 
    GDBusConnection* dbus, 
    Channel* channel,
    gchar* uid, 
    std::string s, 
    GVariant*  params
);

static void send_dbus_response( 
    GDBusConnection* dbus, 
    Channel* channel, 
    const gchar* uid,  
    PyObject*  value, 
    const char* ex = NULL
);

/////////////////////////////////////////////
// Channel

class Channel
{
public:
    Channel(const std::string& guid, PyObject* bind)
        : uid(guid), bound(bind)
    {
        Py_XINCREF(bound);

        // assemble channel config
        std::ostringstream oss_send;
        oss_send << dbus_object_path_send_req_prefix << uid;
        dbus_object_path_send_req_path = oss_send.str();

        std::ostringstream oss_recv;
        oss_recv << dbus_object_path_recv_req_prefix << uid;
        dbus_object_path_recv_req_path = oss_recv.str();

        std::ostringstream oss_res_send;
        oss_res_send << dbus_object_path_send_res_prefix << uid;
        dbus_object_path_send_res_path = oss_res_send.str();

        std::ostringstream oss_res_recv;
        oss_res_recv << dbus_object_path_recv_res_prefix << uid;
        dbus_object_path_recv_res_path = oss_res_recv.str();

        g_print (PROG "Interface; %s.\n", dbus_interface.c_str());
        g_print (PROG "Send; %s.\n", dbus_object_path_send_req_path.c_str());
        g_print (PROG "Recv; %s.\n", dbus_object_path_recv_req_path.c_str());

        sid = g_dbus_connection_signal_subscribe (
            dbus, 
            /*sender*/ NULL, 
            dbus_interface.c_str(),
            /*const gchar *member*/ NULL,
            dbus_object_path_recv_req_path.c_str(),
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            &signal_handler,
            this,
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
            this,
            NULL
        );   
    }

    std::string dbus_object_path_send_req_path; 
    std::string dbus_object_path_recv_req_path; 

    std::string dbus_object_path_send_res_path;
    std::string dbus_object_path_recv_res_path; 

    std::string uid;
    std::string sid;
    std::string rsid;

    PyObject* bound;
};

std::map<std::string,Channel*>& channels()
{
    static std::map<std::string,Channel*> map;
    return map;
}

/////////////////////////////////////////////
// forwards


class Responses
{
public:

    void add(const char* uid, PyObject* p)
    {
        pyobj(p).incr();
        pending_.insert( std::make_pair(std::string(uid),p) );

        //g_print (PROG "responses add %s\n", uid );
    }

    PyObject* get(const char* uid)
    {
        //g_print (PROG "responses get %s\n", uid );

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
    Channel* channel;
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
    self->channel = 0;
    return 0;
}

static void responseCB_object_dealloc(responseCB_object* self)
{
    py_dealloc(self);
}


static PyObject* responseCB_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    responseCB_object* that = (responseCB_object*)self;

    int len = pyobj(args).length();

    if(len==0)
    {
        pyobj_ref none(Py_None);
        none.incr(); // hack
        send_dbus_response(dbus,that->channel,that->uid.c_str(),none);
    }
    else
    {
        pyobj_ref arg = pyobj(args).item(0);

        pyobj_ref r = pyobj(arg).invoke("result");

        if(py_error())
        {
            PyError err;
            pyobj_ref msg = PyObject_Str(err.pvalue);
            send_dbus_response(dbus,that->channel,that->uid.c_str(),NULL,pyobj(msg).str());
        }
        else
        {
            send_dbus_response(dbus,that->channel,that->uid.c_str(),r);
        }

        //g_print (PROG "done responseCB_object_call %i \n" ,len);
    }
    
    Py_RETURN_NONE;
}


PythonTypeObject responseCB_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "WebKitDBus.responseCB_object";
    clazz.tp_basicsize = sizeof(responseCB_object);
    clazz.tp_init = (initproc)responseCB_object_init;
    clazz.tp_dealloc = (destructor)responseCB_object_dealloc;
    clazz.tp_call = (ternaryfunc)responseCB_object_call;
    clazz.tp_doc = "response callback wrapper objects";
});



extern "C" PyObject* new_responseCB_object(const char* uid, Channel* channel)
{
    responseCB_object* self = py_alloc<responseCB_object>(&responseCB_objectType);

    pyobj_ref id = PyUnicode_FromString(uid);
    pyobj_ref tuple = ptuple(id.ref());

    responseCB_object_init(self,tuple.ref(),(PyObject*)NULL);
    
    self->channel = channel;

    return (PyObject*)self;
}



/////////////////////

/////////////////////////////////////////////
// signal object - function object that 
// emits signal when invoked

typedef struct {
    PyObject_HEAD
    std::string signal_name;
    Channel* channel;
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
    self->channel = 0;
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
    Channel* c = that->channel;

    send_dbus_signal(dbus,c,uid,that->signal_name,param);
    
    PyObject* future = new_future_object();
    responses().add(uid,future);

    g_free(uid);

    return future;
}

PythonTypeObject signal_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "WebKitDBus.SignalObject";
    clazz.tp_basicsize = sizeof(signal_object);
    clazz.tp_init = (initproc)signal_object_init;
    clazz.tp_dealloc = (destructor)signal_object_dealloc;
    clazz.tp_call = (ternaryfunc)signal_object_call;
    clazz.tp_doc = "signal wrapper objects";
});

extern "C" PyObject* new_signal_object(const char* name, Channel* c)
{
    signal_object* self = py_alloc<signal_object>(&signal_objectType);

    pyobj_ref signal = PyUnicode_FromString(name);
    pyobj_ref tuple = ptuple(signal.ref());

    signal_object_init(self,tuple,NULL);

    self->channel = c;

    return (PyObject*)self;
}

/////////////////////
// WebViewCtrl object is a helper to invoke a signal 

typedef struct {
    PyObject_HEAD
    Channel* channel;
} webviewctrl_object;

static int webviewctrl_object_init(webviewctrl_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void webviewctrl_object_dealloc(webviewctrl_object* self)
{
   py_dealloc(self);
}

static PyObject * webviewctrl_object_getattr(webviewctrl_object* self, char* name)
{
    g_print (PROG " webviewctrl_object_getattr %s %i\n", name, (void*)self->channel);

    return new_signal_object(name,self->channel);
}

PythonTypeObject webviewctrl_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "WebKitDBus.WebViewCtrl";
    clazz.tp_basicsize = sizeof(webviewctrl_object);
    clazz.tp_init = (initproc)webviewctrl_object_init;
    clazz.tp_dealloc = (destructor)webviewctrl_object_dealloc;
    clazz.tp_getattr = (getattrfunc)webviewctrl_object_getattr;
    clazz.tp_doc = "webview ctrl object";
});

extern "C" PyObject* new_webviewctrl_object(Channel* channel)
{
    g_print (PROG " new_webviewctrl_object  %i\n",  (void*)channel);

    webviewctrl_object* self = py_alloc<webviewctrl_object>(&webviewctrl_objectType);

    self->channel = channel;

    return (PyObject*)self;
}

/////////////////////
// WebView object is a helper to wrap a webview 

typedef struct {
    PyObject_HEAD
} webview_object;

static int webview_object_init(webview_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void webview_object_dealloc(webview_object* self)
{
   py_dealloc(self);
}


static PyObject* webview_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    webview_object* that = (webview_object*)self;

    pyobj arguments(args);
    if(arguments.length()<1)
    {
        PyErr_SetString(PyExc_RuntimeError,"webview_object_call excpets a single webview param");
        return NULL;
    }

    pyobj_ref web = arguments.item(0);
    pyobj_ref uid = pyobj(web).attr("uid");

    g_print (PROG " webview_object_call uid: %s\n", pyobj(uid).str() );

    Channel* channel = channels()[pyobj(uid).str()];

    g_print (PROG " webview_object_call %i\n", (void*)channel);

    return new_webviewctrl_object(channel);
}

PythonTypeObject webview_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "WebKitDBus.WebView";
    clazz.tp_basicsize = sizeof(webview_object);
    clazz.tp_init = (initproc)webview_object_init;
    clazz.tp_dealloc = (destructor)webview_object_dealloc;
    clazz.tp_call = (ternaryfunc)webview_object_call;    
    clazz.tp_doc = "webview object";
});

extern "C" PyObject* new_webview_object()
{
    webview_object* self = py_alloc<webview_object>(&webview_objectType);

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
    Channel* channel = (Channel*)user_data;

    //g_print (PROG " received response %s %s\n", signal_name, g_variant_get_type_string (parameters));

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
        if(ex.isValid() && !pyobj(ex).isNone() )
        {
            pyobj_ref ret = pyobj(future).invoke("set_exception",ex.ref());
        }
        else
        {
            if(py_error())
            {
                PyErr_Print();
            }            

            pyobj_ref ret = pyobj(future).invoke("set_result",result.ref());

            if(py_error())
            {
                PyErr_Print();
            }
        }
    }
    else
    {
        g_print (PROG " received invalid future %s %s\n", signal_name, g_variant_get_type_string (parameters));

    }
}


///////////////////////////////////


static void signal_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    //g_print (PROG " received signal %s %s\n", signal_name, g_variant_get_type_string (parameters));

    Channel* channel = (Channel*)user_data;

    PyGlobalInterpreterLock lock;

    gvar params(parameters);
    int len = params.length();
    if(len < 2)
    {
        g_print (PROG " received invalid signal %s %s\n", signal_name, g_variant_get_type_string (parameters));
        return;
    }

    gvar guid = params.item(0);
    pyobj_ref result;

    gvar gargs = params.item(1);
    std::string data = gargs.str();
    pyobj_ref args = from_json(data);

    //result = pyobj(cb).invoke_with_tuple(signal_name, args);

//    pyobj_ref bound = pyobj(module).attr("callback");

//todo callback from channel
//    pyobj_ref bound = pyobj(PyModule_GetDict(module)).member("callback");

    result = pyobj(channel->bound).invoke_with_tuple(signal_name, args);

    if(!py_error())
    {
        pyobj_ref tuple = ptuple(result.ref());
        result = pywebkit_run_async(NULL,tuple);
    }

    if(py_error())
    {
        PyError err;
        pyobj_ref msg = PyObject_Str(err.pvalue);
        send_dbus_response(dbus,channel,guid.str(),NULL,pyobj(msg).str() );

    }
    else
    if(result.isValid())
    {
        if( (Py_TYPE(result) == &future_objectType) || (Py_TYPE(result) == &task_objectType) )
        {
            pyobj_ref handler = new_responseCB_object(guid.str(),channel);
            pyobj_ref ret = pyobj(result).invoke("add_done_callback", handler.ref() );

            if(PyErr_Occurred())
            {
                PyErr_PrintEx(0);          
            }             
        }
        else
        {
             send_dbus_response(dbus,channel,guid.str(),result);
        }
    }
}

static void got_dbus (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    dbus =  g_bus_get_finish (res, NULL);    

 

}

static void send_dbus_response( GDBusConnection* dbus, Channel* channel, const gchar* uid,  PyObject* val, const char* ex )
{
    pyobj value(val);

    //PyObject_Print(value.ref(), stdout,0);
    //printf("\n");

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

    // g_print (PROG "send_dbus_response : %s %s\n", uid, g_variant_get_type_string (parameters));

    g_dbus_connection_emit_signal(
        dbus,
        NULL,
        channel->dbus_object_path_send_res_path.c_str(),
        dbus_interface.c_str(),
        "response",
        parameters,
        NULL
    );
}

static void send_dbus_signal( GDBusConnection* dbus, Channel* channel, gchar* uid, std::string s, GVariant*  params)
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
        channel->dbus_object_path_send_req_path.c_str(),
        dbus_interface.c_str(),
        s.c_str(),
        parameters,
        NULL
    );
}



static PyObject* pywebkit_bind(PyObject* self, PyObject* args)
{
    int len = pyobj(args).length();

    //g_print (PROG "on signal \n");

    if(len<2)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than two args for call to on_signal!");
        return NULL;
    }

//    cb = pyobj(args).item(0);

    //pyobj(module).attr("callback",pyobj(args).item(0));
    //pyobj_ref web = pyobj(args).item(0);
    //pyobj_ref ctrl = pyobj(args).item(1);
    pyobj_ref uid = pyobj(args).item(0);
    pyobj_ref ctrl = pyobj(args).item(1);
    
     g_print (PROG "pywebkit_bind: %s %i.\n", pyobj(uid).str(), (void*)ctrl.ref());

//    pyobj_ref uid = pyobj(web).attr("uid");

    Channel* channel = new Channel( pyobj(uid).str(), ctrl );

    channels().insert( std::make_pair( pyobj(uid).str(), channel ));
//    pyobj(PyModule_GetDict(module)).member("callback", pyobj(args).item(0) );

    Py_RETURN_NONE;
}


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
                PyErr_PrintEx(0);          
            }
        }
    }
    return task.incr();
}

static PyMethodDef pywebkit_module_methods[] = {
    {"bind",  pywebkit_bind, METH_VARARGS, "register listening for signal via dbus."},
    {"run_async",  pywebkit_run_async, METH_VARARGS, "run a coroutine."},
    {NULL}  /* Sentinel */
};


static PythonModuleDef moduledef( [](PyModuleDef& module)
{
    module.m_name = "WebKitDBus";
    module.m_doc = "dbus interface";
    module.m_methods = pywebkit_module_methods;
});


void add_future_obj_def(pyobj_ref& m);
void add_future_iter_obj_def(pyobj_ref& m);
void add_task_obj_def(pyobj_ref& m);

PyMODINIT_FUNC PyInit_WebKitDBus(void) 
{
    /*
    // ready guards
    if (PyType_Ready(&signal_objectType) < 0)
        return 0;    

    if (PyType_Ready(&signals_objectType) < 0)
        return 0;    

    if (PyType_Ready(&responseCB_objectType) < 0)
        return 0;    
*/
    if (PyType_Ready(&responseCB_objectType) < 0)
        return 0;    

    if (PyType_Ready(&webviewctrl_objectType) < 0)
        return 0;    

/*
    // generate guid
    gchar* c = g_dbus_generate_guid();
    sid = std::string(c);
    g_free(c);
*/
    // assemble module config

/*    std::ostringstream oss_send;
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
*/
    // acquire dbus session
    //g_bus_get(G_BUS_TYPE_SESSION, NULL, &got_dbus,NULL);

    dbus = g_bus_get_sync( G_BUS_TYPE_SESSION, NULL, NULL);

    // create and populate module
    pyobj_ref m = PyModule_Create(&moduledef);

    //pyobj(m).addString( "uid", sid.c_str());

    pyobj(m).addObject("SignalObject", &signal_objectType);
    pyobj(m).addObject("WebView", &webview_objectType);

    add_future_obj_def(m);
    add_future_iter_obj_def(m);    
    add_task_obj_def(m);
    
    pyobj_ref webview = new_webview_object();
    pyobj(m).addObject("WebView", webview);
    
    pyobj_ref cb = Py_None;
    cb.incr();

    pyobj(m).addObject("callback", cb);

    pyobj_ref json = PyUnicode_FromString("json");
    PyImport_Import(json);

    module = m;
    return m.incr();
}




