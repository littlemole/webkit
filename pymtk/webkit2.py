import gi
gi.require_versions({
    'Gtk':  '3.0',
    'WebKit2' : '4.0'
})

from gi.repository import GLib, WebKit2, Gio

import json
import pymtk.future
from pymtk.future import Future
import functools
import os


channels = {}
responses = {}


class ResponseCB(object):

    def __init__(self,uid,web,channel,*args,**kargs):

        self.uid = uid
        self.web = web
        self.channel = channel


    def __call__(self,*args):

        l = len(args)
        if l == 0:
            send_response(self.web,self.channel,self.uid,None,None)
        else:
            f = args[0]

            try:
                print("f:" + str(f))
                r = f.result()

            except BaseException as e:

                send_response(self.web,self.channel,self.uid,None,e)
            else:
                print("r:" + str(r))
                send_response(self.web,self.channel,self.uid,r,None)


class Signal(object):

    def __init__(self,name,web,channel,*args,**kargs):

        self.name = name
        self.web = web
        self.channel = channel


    def __call__(self,*args):

        uid = Gio.dbus_generate_guid()

        send_request(self.web,self.channel,uid,self.name,*args)

        f = Future()
        responses[uid] = f

        return f


class WebViewCtrl(object):

    def __init__(self,web,channel,*args,**kargs):

        self.web = web
        self.channel = channel


    def __getattr__(self,key):

        return Signal(key,self.web,self.channel)



def JavaScript(web):

    if not web in channels:
        return None

    channel = channels[web]

    return WebViewCtrl(web,channel)


def response_handler(vmsg):

    msg = vmsg.get_string()

    hash = json.loads(msg)

    uid = ""
    if "response" in hash:
        uid = hash["response"]
    
    result = None

    if "result" in hash:
        result = hash["result"]

    ex = None
    if "exception" in hash:
        ex = hash["exception"]

    if not uid in responses:
        return

    f = responses[uid]
    del responses[uid]

    if not ex is None:

        f.set_exception(RuntimeEx(ex))
    else:

        f.set_result(result)



def signal_handler(web, vmsg): 

    msg = vmsg.get_string()

    hash = json.loads(msg)

    uid = ""
    method = ""
    params = ()

    if "request" in hash:
        uid = hash["request"]

    if "method" in hash:
        method = hash["method"]

    if "parameters" in hash:
        params = hash["parameters"]

    if not web in channels:
        return

    channel = channels[web]

    fun = getattr(channel,method)

    try:
        result = fun(*params)

    except BaseException as e:

        send_response(web,channel,uid,NULL,e)

    else:

        try:
            print("RUUUUUUUUUUN")
            r = pymtk.future.run(result)
            print(str(r))

        except BaseException as e:

            send_response(web,channel,uid,NULL,e)

        else:

            if (isinstance(r,Future)) or ( isinstance(r,pymtk.future.Task)):

                handler = ResponseCB(uid,web,channel)
                r.add_done_callback(handler)

            else:

                send_response(web,channel, uid,r)


def send_response( web,channel, uid,  value, ex = None ):

    hash = {
        "response" : uid,
        "result" : value,
        "exception" : ex
    }

    print(hash)
    data = json.dumps(hash)

    msg = GLib.Variant.new_string(data)

    message = WebKit2.UserMessage.new("response",msg)

    web.send_message_to_page(message, None, None, None)



def send_request( web,channel, uid,  method, *params):

    hash = {
        "request" : uid,
        "method" : method,
        "parameters" : params
    }

    data = json.dumps(hash)

    msg = GLib.Variant.new_string(data)
    message = WebKit2.UserMessage.new( "request", msg)

    web.send_message_to_page( message, None,None,None)



def user_msg_received( web, message):

    params = message.get_parameters()

    name = message.get_name()

    if( name == "request"):

        signal_handler(web,params)

    if( name == "response"):

        response_handler(params)

    return True



def bind(web,ctrl):

    channels[web] = ctrl

    web.connect("user-message-received",user_msg_received )


def idle_add(func):

    def wrapper(*args):
        GLib.idle_add(func,*args)
        
    return wrapper

