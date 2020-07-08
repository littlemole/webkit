import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Py': '0.1'
})

from gi.repository import Gtk, GObject, Py, WebKit2, GLib

import os
import socket
import threading
import pprint
import sys
import dbus
import dbus.mainloop.glib
import pywebkit
class Worker(threading.Thread):

    def __init__(self,sock,msg,size,onResponse):
        threading.Thread.__init__(self) 
        self.sock = sock
        self.msg  = msg
        self.size = size
        self.cb   = onResponse


    def run(self):

        self.send_request()
        response = self.recv_response()

        # call back into javascript on the main thread
        GObject.idle_add(self.cb,response)


    def send_request(self):

        totalsent = 0
        while totalsent < self.size:
            sent = self.sock.send(self.msg[totalsent:])
            if sent == 0:
                raise RuntimeError("socket connection broken")
            totalsent = totalsent + sent


    def recv_response(self):

        response = bytearray() 
        recv = bytearray()
        while True:
            recv = self.sock.recv(4096)
            if len(recv) == 0 :
                break;
            response = response + recv

        # convert response to unicode
        try:
            response = response.decode("UTF-8")
        except:
            response = response.decode("ISO-8859-1")

        return response



def catchall_signal_handler(*args, **kwargs):
    for arg in args:
        pprint.pprint(arg)
#        print ("        " + str(arg))
    for key in kwargs:
        print("   %s: %s" % (key, kwargs[key]))

    #Py.send_signal("on_create", "dummy value")


class Controller(object):


    def submit(self,req,onResponse):

        msg = req.payload
        # unix2dos for dummies. this is a hack to support proper http
        # headers when using a webkit html textarea to assemble the
        # payload
        msg = msg.replace("\r","")
        msg = msg.replace("\n","\r\n")
        msg = msg.encode()
        size = len(msg)
       
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((req.host,int(req.port)))

        self.worker = Worker(sock,msg,size,onResponse)
        self.worker.start()

def on_create(w):
    #pass
    #import pywebkit
   # from pywebkit import pywebkit.PyDBUSObject 
   # print(dir(__main__))
    pywebkit.send_signal("on_create", "dummy value")
   # obj = pywebkit.PyDBUSObject()
   # obj("HelloSignal",{"key":"HELLO","values":[47,11]})
    # pprint.pprint(pywebkit)
    #web.webkit_send_signal("HelloSignal",GLib.Variant("s","World"))

def on_ext():
    print("init extensions!")

def on_show(x,y):
    print("on show")

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
bus.add_signal_receiver(catchall_signal_handler, dbus_interface = "com.example.TestService", signal_name = "HelloSignal" ,member_keyword="signal_name")
bus.add_signal_receiver(catchall_signal_handler, dbus_interface = "com.example.TestService", signal_name = "on_create", member_keyword="signal_name")

# global main.controller accessed from javascript
controller = Controller()        

# create html widget
web = Py.WebKit() #WebKit2.WebView()#Py.WebKit() 
#import pywebkit
#import pywebkit

#pprint.pprint(pywebkit.DBus)

url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/curl.html"
web.load_uri(url)
print(web.uid)
print(pywebkit.uid)

# make resizable
scrolledwindow = Gtk.ScrolledWindow()
scrolledwindow.add(web)

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(scrolledwindow)
win.connect("delete-event", Gtk.main_quit)
win.connect("realize", on_create)
#web.connect("initialize-web-extensions", on_ext)

web.connect("load-changed",on_show)
win.show_all()

# start the GUI event main loop
GObject.threads_init()
Gtk.main()

        
