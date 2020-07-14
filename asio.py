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

class Worker(threading.Thread):

    def __init__(self,msg,loop):
        threading.Thread.__init__(self) 
        self.msg  = msg
        self.loop = loop
        self.future = loop.create_future()

    def launch(msg,loop):
        worker = Worker("HELO",loop)
        worker.worker = worker
        worker.start()
        print("started")
        return worker.future

    def done(self):

        print("done")
        self.future.set_result(self.msg)

    def run(self):

        print("start run")
        time.sleep(1)
        print("wait run")

        GObject.idle_add(self.done)

        #WebKitDBus.View.recvResponse(response)


def onClose(event,s):
    loop.stop()


def glib_update(main_context, loop):
    while main_context.pending():
        main_context.iteration(False)
    loop.call_later(.01, glib_update, main_context, loop)
    

def start2():
    print("start2")

    return _MyStart()

class _MyStart:
    def __init__(self):
        print("mystart init")
        pass

    def __await__(self):
        print("mystart __await__")
        return _MyStartIter2()


class _MyStartIter2:
    def __init__(self):
        self.state = 0
        self.f = Worker.launch("HELO",loop)

    def __iter__(self):  # an iterator has to define __iter__
        return self

    def __next__(self):
        print("__next__ state " + str(self.state))
        if self.state == 0:
            
            #self.f = self.worker.future
            #loop = asyncio.get_event_loop()
            #self.future = loop.create_future()
#            loop.call_later(self.n, self.future.set_result, None)
            self.state = 1
        if self.state == 1:
            if not self.f.done():
                return next(iter(self))
            self.state = 2
        if self.state == 2:
            print(self.f.result())
            #return self.f.result()
            raise StopIteration(self.f.result())
        raise AssertionError("invalid state")



async def start():

    f = Worker.launch("HELO",loop)
    msg = await f
    print("!!!!!!!!!!!!!!!")
    print("awaited " +msg )
    #return worker.future()




future = None
# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.connect("delete-event", onClose)
win.show_all()

# start the GUI event main loop
#GObject.threads_init()
#Gtk.main()

loop = asyncio.SelectorEventLoop()
asyncio.set_event_loop(loop)


try:
    main_context = GLib.MainContext.default()
#    task = loop.create_task(start())
#    loop.call_soon(task)
    #loop.call_soon(start)
    glib_update(main_context, loop)
    #loop.run_until_complete(start())
    loop.run_until_complete(start2())
    loop.run_forever()
finally:
    loop.close()

