import gi
gi.require_versions({
    'Gtk':  '3.0',
#    'Pywebkit': '0.1'
})

from gi.repository import Gtk, GObject, GLib #Pywebkit, WebKit2, GLib

import os
import socket
import threading
import pprint
import sys
import time
import types
#import asyncio
#from asyncio import coroutines
#import exceptions
import WebKitDBus

class Worker(threading.Thread):

    def __init__(self,msg):
        threading.Thread.__init__(self) 
        self.msg  = msg
        #self.future = Future()# loop.create_future()#asyncio.Future(loop=loop)
        use_own_future = True#loop is None
        self.future = WebKitDBus.Future()

    def __del__(self):
        print("delete Worker")

    #async 
    def begin(self):

        print("begin")
        #await my_sleep(1)
        self.start()
        return self.future
        #return r
        #yield self.future


    def run(self):

        time.sleep(1)
        print("run: "  + self.msg)
        GLib.idle_add(self.done)

    def done(self):
        print("done: "  + self.msg)
        self.future.set_result(self.msg)
        #self.future.done()
        
 

class Controller(object):


    def onCreate(self,event):

        #self.worker = Worker("HELO")
        #await self.worker.begin()
        print("onCreate")


def onClose(e,w):
    Gtk.main_quit()


class InvalidStateError(Exception):
    def __init__(self,value):
        self.value = value

class Task(WebKitDBus.Future):

    def __init__(self,coro):

        print("init task")
        super().__init__()

        print("init task 2")

        self.coro_ = coro

        print("init task 3")

        if not isinstance(coro, (types.CoroutineType) ):
#        if not coroutines.iscoroutine(coro):
            print("NOT A CORO")
            pprint.pprint(coro)
            super().set_result(coro)
        else:
            self.step()

    def __del__(self):
        print("delete Task")

    def step(self):
        print("step")
        GLib.idle_add(self._step, self.exc_)

    def _step(self,exc):
        print("__stepping__")
        if self.done():
            raise InvalidStateError("Task already done!")

        try:
        
            if self.exc_ is None:
                result = self.coro_.send(None)
            else:
                result = self.coro_.throw(self.exc_)

        except StopIteration as exc:
            super().set_result(exc.value)
        except BaseException as exc:
            super().set_exception(exc)

        else:
            blocking = getattr(result, '_asyncio_future_blocking', None)
            if blocking is not None:
                if blocking:
                    result._asyncio_future_blocking = False
                    result.add_done_callback(self._wakeup)
            elif result is None:
                GLib.idle_add(self._step,None)


    def _wakeup(self, f):
        print("wakeup")
        try:
            f.result()
        except BaseException as exc:
            # This may also be a cancellation.
            self._step(exc)
        else:
            self._step(None)




def run_gtk_main():
    Gtk.main()

def run_task(coro,*args):
    print("run task")
    #task = Task(coro)
    task = WebKitDBus.Task(coro)

    pprint.pprint(task.step)
    pprint.pprint(task.result)
    
    if len(args) > 0:
        cb = args[0]
        if(task.done()):
            print("task done");
            GLib.idle_add(cb,task)
        else:
            print("task asynx");
            task.add_done_callback(cb)

    print("return task");

    return task

async def goAsync():

    print("goAsync")
    worker =  Worker("HELO")
    f =  worker.begin()
    #f.add_done_callback(doneCB)
    r = await f
    print("awakening" + str(r))

    return r

def goSync():

    print("goSync")
    return "SYNCED"

def onGoAsyncDone(f):
#    pprint.pprint(f)
    print("ASNYC DONE: " + f.result())

def onGoSyncDone(f):
    pprint.pprint(f)
    print("SYNC DONE: " + f.result())

def onClickAsync(w):
    print("clicked async")
    #task = run_task(goAsync())
    #task.add_done_callback(onClickDone)
    run_task(goAsync(),onGoAsyncDone)

def onClickSync(w):
    print("clicked sync")
    #task = run_task(goAsync())
    #task.add_done_callback(onClickDone)
    run_task(goSync(),onGoSyncDone)

# global main.controller accessed from javascript
controller = Controller()        

buttAsync = Gtk.Button(label="Click Async")
buttAsync.connect("clicked", onClickAsync)
buttSync = Gtk.Button(label="Click Sync")
buttSync.connect("clicked", onClickSync)

hbox = Gtk.HBox()
hbox.add(buttAsync)
hbox.add(buttSync)
# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(hbox)
win.connect("delete-event", onClose)# Gtk.main_quit)
win.connect("realize", controller.onCreate)
win.show_all()

#run_asyncio()
run_gtk_main()


        
