import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit, GLib 

import os,sys,socket,json,threading,pprint
import pygtk.WebKitDBus as WebKitDBus
from pygtk.worker import Worker
from pygtk.worker import background
from pygtk.bind import bind
from pygtk.bind import synced


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

    @synced
    async def onRequest(self,*args):

        r = await WebKitDBus.WebView.onSubmit()
        print(r)


    @synced
    async def onExit(self,*args):
        Gtk.main_quit()

# global main.controller accessed from javascript
controller = Controller()        
pprint.pprint(controller)
#WebKitDBus.bind(controller)
#WebKitDBus.callback = controller

# create html widget
web = Pywebkit.Webview() 
url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/curl.html"
web.load_uri(url)

# make resizable
#scrolledwindow = Gtk.ScrolledWindow()
#scrolledwindow.add(web)

# main window
#win = Gtk.Window()     
#win.set_default_size(550, 350)   
#win.add(scrolledwindow)
#win.connect("delete-event", Gtk.main_quit)
#win.show_all()

builder = Gtk.Builder()
builder.add_from_file("curl.ui.xml")
builder.connect_signals(controller)

scrollWindow = builder.get_object("scrollWindow")
scrollWindow.add(web)

win = builder.get_object("mainWindow")
win.show_all()

# start the GUI event main loop
Gtk.main()

        
