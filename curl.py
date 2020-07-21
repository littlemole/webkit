import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit

import os,sys,socket,json
#import WebKitDBus
import pygtk.WebKitDBus as WebKitDBus
from pygtk.worker import Worker

class request_task(object):

    def __init__(self,msg,host,port):
        self.msg = msg
        self.host = host
        self.port = int(port)
        self.size = len(msg)
       
    def __call__(self):

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host,self.port))

        totalsent = 0
        while totalsent < self.size:
            sent = self.sock.send(self.msg[totalsent:])
            if sent == 0:
                raise RuntimeError("socket connection broken")
            totalsent = totalsent + sent

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
        r = await Worker.schedule(request_task(msg,host,port))
        return r


# global main.controller accessed from javascript
controller = Controller()        
WebKitDBus.bind(controller)

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

        
