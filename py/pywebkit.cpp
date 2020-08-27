#include "coro.h"
#include "pywebkit.h"

/////////////////////////////////////////////
// externs

extern PyTypeObject future_objectType;
extern PyTypeObject task_objectType;
extern "C" PyObject* new_task_object(PyObject* coro);

/////////////////////////////////////////////
// globals

#define PROG "[pyWebKit]"

static PyObject* module;

/////////////////////////////////////////////
// forwards

class Channel;

static PyObject* pywebkit_run_async(
    PyObject* self, 
    PyObject* args
);

static void response_handler(GVariant* params);
static void signal_handler(WebKitWebView *web, GVariant* params);

static void send_response( Channel* channel, const gchar* uid,  PyObject* val, const char* ex = NULL );
static void send_request( Channel* channel, const gchar* uid,  std::string m, PyObject* params);

/////////////////////////////////////////////
// Channel

class Channel
{
public:
    Channel(PyObject* w, PyObject* bind)
        : web(w), bound(bind)
    {
        Py_XINCREF(bound);
        Py_XINCREF(web);
    }

    PyObject* web;
    PyObject* bound;
};

std::map<WebKitWebView*,Channel*>& channels()
{
    static std::map<WebKitWebView*,Channel*> map;
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
        send_response(that->channel,that->uid.c_str(),none);
    }
    else
    {
        pyobj_ref arg = pyobj(args).item(0);

        pyobj_ref r = pyobj(arg).invoke("result");

        if(py_error())
        {
            PyError err;
            pyobj_ref msg = PyObject_Str(err.pvalue);
            send_response(that->channel,that->uid.c_str(),NULL,pyobj(msg).str());
        }
        else
        {
            send_response(that->channel,that->uid.c_str(),r);
        }
        //g_print (PROG "done responseCB_object_call %i \n" ,len);
    }
    
    Py_RETURN_NONE;
}


PythonTypeObject responseCB_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "pygtk.WebKit.responseCB_object";
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

    gchar* uid = g_dbus_generate_guid();

    send_request( that->channel, uid, that->signal_name, args);
    
    PyObject* future = new_future_object();
    responses().add(uid,future);

    g_free(uid);

    return future;
}

PythonTypeObject signal_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "pygtk.WebKit.SignalObject";
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
    //g_print (PROG " webviewctrl_object_getattr %s\n", name ;

    return new_signal_object(name,self->channel);
}

PythonTypeObject webviewctrl_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "pygtk.WebKit.WebViewCtrl";
    clazz.tp_basicsize = sizeof(webviewctrl_object);
    clazz.tp_init = (initproc)webviewctrl_object_init;
    clazz.tp_dealloc = (destructor)webviewctrl_object_dealloc;
    clazz.tp_getattr = (getattrfunc)webviewctrl_object_getattr;
    clazz.tp_doc = "webview ctrl object";
});

extern "C" PyObject* new_webviewctrl_object(Channel* channel)
{
    //g_print (PROG " new_webviewctrl_object  %i\n",  (void*)channel);

    webviewctrl_object* self = py_alloc<webviewctrl_object>(&webviewctrl_objectType);

    self->channel = channel;

    return (PyObject*)self;
}

/////////////////////
// JavaScript object is a helper to talk to the WebView

typedef struct {
    PyObject_HEAD
} javascript_object;

static int javascript_object_init(javascript_object *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void javascript_object_dealloc(javascript_object* self)
{
   py_dealloc(self);
}


static PyObject* javascript_object_call(PyObject* self, PyObject* args, PyObject* kargs)
{
    //javascript_object* that = (javascript_object*)self;

    pyobj arguments(args);
    if(arguments.length()<1)
    {
        PyErr_SetString(PyExc_RuntimeError,"javascript_object_call excpets a single webview param");
        return NULL;
    }

    pyobj_ref web = arguments.item(0);
    //pyobj_ref uid = pyobj(web).attr("uid");

    //g_print (PROG " javascript_object_call uid: %s\n", pyobj(uid).str().c_str() );

    //const char* id = pyobj(uid).str();

    WebKitWebView* nativeweb = (WebKitWebView*) gobject(web);

    if(channels().count( nativeweb) == 0)
    {
        Py_RETURN_NONE;
    }

    Channel* channel = channels()[nativeweb];

    //g_print (PROG " javascript_object_call %i\n", (void*)channel);

    return new_webviewctrl_object(channel);
}

PythonTypeObject javascript_objectType( [](PyTypeObject& clazz)
{
    clazz.tp_name = "pygtk.WebKit.javascript";
    clazz.tp_basicsize = sizeof(javascript_object);
    clazz.tp_init = (initproc)javascript_object_init;
    clazz.tp_dealloc = (destructor)javascript_object_dealloc;
    clazz.tp_call = (ternaryfunc)javascript_object_call;    
    clazz.tp_doc = "webview object";
});

extern "C" PyObject* new_javascript_object()
{
    javascript_object* self = py_alloc<javascript_object>(&javascript_objectType);

    return (PyObject*)self;
}

///////////////////////////////////

static void response_handler(GVariant* message)
{
    //g_print (PROG " received response %s %s\n", signal_name, g_variant_get_type_string (parameters));

    PyGlobalInterpreterLock lock;

    gvar msg(message);

    std::string json = msg.str();

    pyobj_ref dict = from_json(json);

    pyobj_ref uid;
    if( pyobj(dict).hasMember("response") ) 
    {
        uid = pyobj(dict).member("response");
    }

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

    if(!uid.isValid() || pyobj(uid).str() == 0 || strlen(pyobj(uid).str()) == 0)
    {
        g_print (PROG " received invalid response no uid ");
        return;
    }

    pyobj_ref future = responses().get( pyobj(uid).str() );

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
        g_print (PROG " received invalid response no futur %s \n",  pyobj(uid).str());

    }
}


///////////////////////////////////

static void signal_handler(WebKitWebView *web, GVariant* message)
{
    g_print (PROG " received signal \n");

    PyGlobalInterpreterLock lock;

    gvar msg(message);

    // parse JSON

    std::string json = msg.str();
    pyobj_ref dict = from_json(json);

    // extract data
    pyobj_ref uid;
    pyobj_ref method;
    pyobj_ref params;

    if ( pyobj(dict).hasMember("request") )
    {
        uid = pyobj(dict).member("request");
    }
    if ( pyobj(dict).hasMember("method") )
    {
        method = pyobj(dict).member("method");
    }
    if ( pyobj(dict).hasMember("parameters") )
    {
        params = pyobj(dict).member("parameters");
    }    

    // validate data
    if(!uid.isValid() || pyobj(uid).str() == 0 || strlen(pyobj(uid).str()) == 0 )
    {
        g_print (PROG " received invalid signal empty uid \n");
        return;
    }

    if(!method.isValid() || pyobj(method).str() == 0 || strlen( pyobj(method).str()) == 0 )
    {
        g_print (PROG " received invalid signal empty method \n");
        return;
    }

   if(!params.isValid()  )
    {
        g_print (PROG " received invalid signal empty params \n");
        return;
    }

    // get channel and call callback

    if(channels().count(web) == 0) 
    {
        g_print (PROG " received signal without channel \n");
        return;
    }

    Channel* channel = channels()[web];

    pyobj_ref result = pyobj(channel->bound).invoke_with_tuple( pyobj(method).str(), params);

    // check for error, otherwise start async coro (if it is a coro)
    if(!py_error())
    {
        pyobj_ref tuple = ptuple(result.ref());
        result = pywebkit_run_async(NULL,tuple);
    }

    if(py_error())
    {
        PyError err;
        pyobj_ref msg = PyObject_Str(err.pvalue);
        send_response(channel,pyobj(uid).str(),NULL,pyobj(msg).str() );

    }
    else
    if(result.isValid())
    {
        // check if a Future was returned
        if( (Py_TYPE(result) == &future_objectType) || (Py_TYPE(result) == &task_objectType) )
        {
            pyobj_ref handler = new_responseCB_object( pyobj(uid).str(),channel);
            pyobj_ref ret = pyobj(result).invoke("add_done_callback", handler.ref() );

            if(PyErr_Occurred())
            {
                PyErr_PrintEx(0);          
            }             
        }
        else
        {
             send_response(channel, pyobj(uid).str(),result);
        }
    }
}

static void send_response( Channel* channel, const gchar* uid,  PyObject* val, const char* ex )
{
    pyobj value(val);

    //PyObject_Print(value.ref(), stdout,0);
    //printf("\n");

    pyobj_ref dict = PyDict_New();
    pyobj(dict).member("response", PyUnicode_FromString(uid));
    pyobj(dict).member("result", val ? val : Py_None );
    if(ex)
    {
        pyobj(dict).member("exception", PyUnicode_FromString(ex));
    }
    else
    {
        pyobj(dict).member("exception", Py_None);
    }

    std::string json = to_json(dict);

    GVariant* msg = g_variant_new_string(json.c_str());
    WebKitUserMessage* message = webkit_user_message_new( "response", msg);

    GObject* nativeweb = gobject(channel->web);
    webkit_web_view_send_message_to_page( (WebKitWebView*)nativeweb, message, NULL, NULL, NULL);

}

static void send_request( Channel* channel, const gchar* uid,  std::string m, PyObject* params)
{

    pyobj_ref dict = PyDict_New();
    pyobj(dict).member("request", PyUnicode_FromString(uid) );
    pyobj(dict).member("method", PyUnicode_FromString(m.c_str()) );
    pyobj(dict).member("parameters", params);

    std::string json = to_json(dict);

    GVariant* msg = g_variant_new_string(json.c_str());
    WebKitUserMessage* message = webkit_user_message_new( "request", msg);

    GObject* nativeweb = gobject(channel->web);
    webkit_web_view_send_message_to_page( (WebKitWebView*) nativeweb, message, NULL, NULL, NULL);
}

static gboolean user_msg_received(
    WebKitWebView      *web,
    WebKitUserMessage *message,
    gpointer           user_data
    )
{
        printf("user_msg_received:\n");

    GVariant* params = webkit_user_message_get_parameters(message);

    std::string name = webkit_user_message_get_name(message);

    if( name == "request")
    {
        signal_handler(web,params);
    }
    if( name== "response")
    {
        response_handler(params);
    }

    return TRUE;
}

static PyObject* pywebkit_bind(PyObject* self, PyObject* args)
{
    g_print(PROG "pywebkit_bind ++++++++++++++++++++++++\n");

    int len = pyobj(args).length();

    if(len<2)
    {
        PyErr_SetString(PyExc_RuntimeError, "less than two args for call to on_signal!");
        return NULL;
    }

    pyobj_ref web = pyobj(args).item(0);
    pyobj_ref ctrl = pyobj(args).item(1);
    pyobj_ref uid = pyobj(web).attr("uid");

    Channel* channel = new Channel( web, ctrl );

    WebKitWebView* nativeweb = (WebKitWebView*) gobject(web);
    channels().insert( std::make_pair( nativeweb, channel ));

    g_signal_connect(G_OBJECT(nativeweb), "user-message-received", G_CALLBACK (user_msg_received),  NULL);

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

    printf("run_async_cb:\n");


    run_async_cb_struct* data = (run_async_cb_struct*)user_data;

    pyobj_ref done = pyobj(data->cb).invoke("done",data->task.ref());

    delete data;
    return false;
}

static PyObject* pywebkit_run_async(PyObject* self, PyObject* args)
{
    printf("pywebkit_run_async:\n");

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
    module.m_name = "WebKit";
    module.m_doc = "dbus interface";
    module.m_methods = pywebkit_module_methods;
});


void add_future_obj_def(pyobj_ref& m);
void add_future_iter_obj_def(pyobj_ref& m);
void add_task_obj_def(pyobj_ref& m);

PyMODINIT_FUNC PyInit_WebKit(void) 
{
    printf("PyInit_WebKit:\n");
    // ready guards
    if (PyType_Ready(&signal_objectType) < 0)
        return 0;    

    if (PyType_Ready(&javascript_objectType) < 0)
        return 0;    

    if (PyType_Ready(&responseCB_objectType) < 0)
        return 0;    

    if (PyType_Ready(&webviewctrl_objectType) < 0)
        return 0;    

    // create and populate module
    pyobj_ref m = PyModule_Create(&moduledef);

    add_future_obj_def(m);
    add_future_iter_obj_def(m);    
    add_task_obj_def(m);
    
    pyobj_ref js = new_javascript_object();
    pyobj(m).addObject("JavaScript", js);
    
    pyobj_ref cb = Py_None;
    cb.incr();

    pyobj(m).addObject("callback", cb);

    pyobj_ref json = PyUnicode_FromString("json");
    PyImport_Import(json);

    module = m;

    return m.incr();
}




