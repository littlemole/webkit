from gi.repository import Gtk, GObject, Py

import os
import socket
import threading


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

        response = ''
        recv = 0
        while True:
            recv = self.sock.recv(4096)
            if recv == "" :
                break;
            response = response + recv

        # convert response to unicode
        try:
            response = response.decode("UTF-8")
        except:
            response = response.decode("ISO-8859-1")

        return response




class Controller(object):


    def submit(self,req,onResponse):

        msg = req.payload
        # unix2dos for dummies. this is a hack to support proper http
        # headers when using a webkit html textarea to assemble the
        # payload
        msg = msg.replace("\r","")
        msg = msg.replace("\n","\r\n")
        size = len(msg)
       
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((req.host,int(req.port)))

        self.worker = Worker(sock,msg,size,onResponse)
        self.worker.start()


# global main.controller accessed from javascript
controller = Controller()        

# create html widget
web = Py.WebKit() 
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
GObject.threads_init()
Gtk.main()

        
