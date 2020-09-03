import gi
gi.require_versions({
    'Gtk':  '3.0',
#    'Pywebkit': '0.1'
})

from gi.repository import Gtk, GLib #, Pywebkit

from pymtk.WebView import WebView2
#from gi.repository.Pywebkit import Webview #as Webview
#import pygtk.WebKit as WebKit
import pymtk.webkit2
import pymtk.future 

import os,sys,socket,json,threading,pprint
from pymtk.future import Worker
from pymtk.future import background
from pymtk.future import synced
from pymtk.webkit2 import idle_add
from pymtk.menumaker import MenuMaker


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
        #r = await 
        r = await request_task(msg,host,port) #run_in_background(request_task,msg,host,port)
        return r

    @synced()
    async def onRequest(self,*args):

        pymtk.webkit2.JavaScript(web).onSubmit()

    @idle_add
    def onExit(self,*args):
        Gtk.main_quit()

# create controller
controller = Controller()        

# create html widget
web = WebView2() 
url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/curl.html"
web.load_uri(url)

pymtk.webkit2.bind(web,controller)

# make resizable
scrolledwindow = Gtk.ScrolledWindow()
scrolledwindow.add(web)

# main menu
menu = {
    "Action" : [
        ["Execute", controller.onRequest],
        ["Quit", controller.onExit]
    ]
}

menubar = Gtk.MenuBar()
mm = MenuMaker(menu)
mm.populate(menubar)

vbox = Gtk.VBox(False, 2)
vbox.pack_start(menubar, False, False, 0)
vbox.add(scrolledwindow)

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(vbox)
win.connect("delete-event", controller.onExit)

print("bound")
win.show_all()
print("show")


# start the GUI event main loop
Gtk.main()

        
