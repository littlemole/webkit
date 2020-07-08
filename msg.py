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
#import dbus
#import dbus.mainloop.glib
import pywebkit
import json

#def onSendData(data):
#    pywebkit.send_signal("recvData", data)

def catchall_signal_handler(*args, **kwargs):

    print("-----------------------------------------")
    for arg in args:
        pprint.pprint(arg)
    for key in kwargs:
        print("   %s: %s" % (key, kwargs[key]))
    print("-----------------------------------------")



class Controller(object):

    def onSendData(self,data):
        txt = json.dumps(data)
        print(txt)
        msg = json.loads(txt)
        pywebkit.send_signal("recvData", msg)

#dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
#bus = dbus.SessionBus()

# global main.controller accessed from javascript
controller = Controller()        
print(dir(controller))

#bus.add_signal_receiver(controller.onSendData, dbus_interface = "com.example.TestService", signal_name = "sendData")
#bus.add_signal_receiver(catchall_signal_handler, dbus_interface = "com.example.TestService", member_keyword="signal_name")

# create html widget
web = Py.WebKit() #WebKit2.WebView()#Py.WebKit() 
url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/signal.html"
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

pywebkit.on_signal("sendData",controller.onSendData)

# start the GUI event main loop
GObject.threads_init()
Gtk.main()

        
