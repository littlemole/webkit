import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import GLib

import json
import future
from future import Future


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
                r = f.result()

            except BaseException as e:

                send_response(self.web,self.channel,self.uid,None,e)
            else:

                send_response(self.web,self.channel,self.uid,r,None)


class Signal(object):

    def __init__(self,name,web,channel,*args,**kargs):

        self.name = name
        self.web = web
        self.channel = channel


    def __call__(self):

        uid = Gio.dbus_generate_guid()

        send_request(self.web,self.channel,uid,self.name,*args)

        f = Future()
        response[uid] = f


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

    msg = msg.get_string()

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



def signal_handler(web, gmsg): 

    msg = msg.get_string()

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

        send_response(channel,uid,NULL,e)

    else:

        try:
            r = future.run(result)

        except BaseException as e:

            send_response(channel,uid,NULL,e)

        else:

            if r is Future:

                handler = ResponseCB(web,uid,channel)
                r.add_done_callback(handler)

            else:

                send_response(web,channel, uid,r)


def send_response( web,channel, uid,  value, ex = None ):

    hash = {
        "response" : uid,
        "result" : value,
        "exception" : ex
    }

    json = json.dumps(hash)

    msg = GLib.Variant.new_string(json)

    message = WebKit.UserMessage.new("response",msg)

    web.send_message_to_page(message, None, None, None)



def send_request( web,channel, uid,  method, params)

    hash = {
        "request" : uid,
        "method" : method,
        "parameters" : params
    }

    json = json.dumps(hash)

    msg = GLib.Variant.new_string(json)
    message = WebKit.UserMessage.new( "request", msg)

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

