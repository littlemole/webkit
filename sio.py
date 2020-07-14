import gi
gi.require_versions({
    'Gtk':  '3.0',
#    'Pywebkit': '0.1'
})

from gi.repository import Gtk, GObject, GLib

import os
import socket
import threading
import pprint
import sys
#import dbus
#import dbus.mainloop.glib
#import WebKitDBus
import asyncio
import time

class Future:
    def __init__(self):
        self.done_ = False
        self.value_ = None

    def __iter__(self):  # an iterator has to define __iter__
       # print("Future: __iter__")
        return self

    def __next__(self):        
        pass
       # print("Future: __next__")

    def __await__(self):
        print("mystart __await__")
        return _FutureIter(self)

    def done(self):
        return self.done_

    def then(self,cb):
        self.cb = cb

    def resolve(self,r):
        print("resolve: " + str(r))
        #time.sleep(1)
        self.value_ = r
        self.done_ = True
#        GLib.idle_add(self.cb,r)
#        self.cb(r)

    def get(self):
        return self.value_

class _FutureIter:
    def __init__(self,f):
        self.state = 0
        self.future = f
        self.cnt = 0

    def __iter__(self):  # an iterator has to define __iter__
        return self


    def __next__(self):
        #self.cnt = self.cnt + 1
        #if self.cnt % 100 == 0:
         #   print("__next__ state " + str(self.state) + " " + str(self.cnt))
        if self.state == 0:
            
            self.state = 1
        if self.state == 1:
            if not self.future.done():
                return None#self.future #next(iter(self))
            self.state = 2
        if self.state == 2:
            print(self.future.get())
            #return self.f.result()
            raise StopIteration(self.future.get())
        raise AssertionError("invalid state")    

class Worker(threading.Thread):

    def __init__(self,msg):
        threading.Thread.__init__(self) 
        self.msg  = msg
        self.future = Future()

    def done(self):

        print("done")
        self.future.resolve(self.msg)

    def run(self):

        print("start run")
        time.sleep(1)
        print("wait run")

        #GObject.idle_add(self.done)
        self.future.resolve(self.msg)

        #WebKitDBus.View.recvResponse(response)

#closeFuture = Future()

def onClose(event,s):
    #Gtk.main_quit()
    loop.stop()
    #closeFuture.resolve(None)

def done(r):

    print("done: " + str(r) )

def onClick(event):
    print("click")
    loop.schedule(test.start)

class Test:
    async def start(self):

        print("start")
        worker = Worker("HELO")
        f = worker.future
        worker.start()
        #f.then(done)
        r = await f
        print("started " + r)
    #    await closeFuture
    #    loop.stop()

test = Test()

butt = Gtk.Button("HELLO")
butt.connect("clicked", onClick)

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.connect("delete-event", onClose)
win.add(butt)
win.show_all()

# start the GUI event main loop
#GObject.threads_init()
#asyncio.run(start())
#Gtk.main()

class Loop:
    def __init__(self):
        self.loop = asyncio.SelectorEventLoop()
        asyncio.set_event_loop(self.loop)

    def run(self):
        try:
            self.main_context = GLib.MainContext.default()
            self.glib_update()
            print("rununtil")
            #self.loop.run_until_complete(start())
            print("runforever")
            self.loop.run_forever()
        finally:
            self.loop.close()

    def stop(self):
        self.loop.stop()

    def glib_update(self):
        while self.main_context.pending():
            #print ("ctx")
            self.main_context.iteration(False)
        #print ("loop")
        self.loop.call_later(.01, self.glib_update)
        
    def schedule(self,task):
        #self.loop.run_until_complete(task())        
        self.loop.create_task(task())

loop = Loop()
loop.run()