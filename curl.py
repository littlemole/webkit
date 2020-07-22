import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit, GLib

import os,sys,socket,json,threading,pprint
#import WebKitDBus
import pygtk.WebKitDBus as WebKitDBus
from pygtk.worker import Worker
from pygtk.worker import background



@background       
def request_task(msg,host,port):

    print("request_task")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host,int(port)))

    size = len(msg)
    totalsent = 0
    while totalsent < size:
        sent = sock.send(msg[totalsent:])
        if sent == 0:
            raise RuntimeError("socket connection broken")
        totalsent = totalsent + sent

    response = bytearray() 
    recv = bytearray()
    while True:
        recv = sock.recv(4096)
        if len(recv) == 0 :
            break
        response = response + recv

    # convert response to unicode
    try:
        response = response.decode("UTF-8")
    except:
        response = response.decode("ISO-8859-1")

    return response

def bind(clazz):
    class wrapper():

        def __init__(self,*args):
            print("init")
            self.that = clazz(*args)

        def __call__(self,*args):
            print("call")
            self.that = clazz(*args)

        def __attr__(self,key):
            print("attr")
            return getattr(self.that,key)
    return wrapper

@bind
class Controller(object):

    async def sendRequest(self,req):

        msg = req["payload"]
        # unix2dos for dummies. this is a hack to support proper http
        # headers when using a webkit html textarea to assemble the
        # payload
        msg = msg.replace("\r","")
        msg = msg.replace("\n","\r\n")
        msg = msg.encode()

        host = req["host"]
        port = req["port"]

        # run async on background thread
        #r = await Worker.schedule(request_task(msg,host,port))
        r = await request_task(msg,host,port) #run_in_background(request_task,msg,host,port)
        return r


# global main.controller accessed from javascript
controller = Controller()        
pprint.pprint(controller)
#WebKitDBus.bind(controller)
WebKitDBus.callback = controller

# create html widget
web = Pywebkit.Webview() 
url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/curl.html"
web.load_uri(url)

# make resizable
scrolledwindow = Gtk.ScrolledWindow()
scrolledwindow.add(web)

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(scrolledwindow)
win.connect("delete-event", Gtk.main_quit)
win.show_all()

# start the GUI event main loop
Gtk.main()

        
