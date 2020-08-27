import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import  GLib

import inspect

class Future(object):

    def __init__(self,*args,**kargs):

        self.done = False
        self.value = None
        self.exception = None
        self._asyncio_future_blocking = True
        self.cb = None

    def done(self):
        return self.done

    def set_result(self,result):
        self.value = result
        self.done = True
        if not self.cb is None:
            self.cb(self)

    def set_exception(self,ex):
        self.exception = ex
        self.done = True
        if not self.cb is None:
            self.cb(self)

    def add_done_callback(self,cb):
        self.cb = cb
        if self.done:
            self.cb(self)


    def result(self):
        if not self.exception is None:
            raise self.exception
        return self.result 
    

class FutureIter(object):

    def __init__(self,f,*args,**kargs):

        self.state = 0
        self.future = f
    
    def __iter__(self):
        return self

    def __next__(self):
        if self.state == 0:
            done = self.future.done()
            if done == False:
                return self.future
            else:
                self.state = 1
        if self.state == 1:
            r = self.future.result()
            raise StopIteration(r)


class Task(Future):

    def __init__(self,coro,*args,**kargs):

        super().__init__() 
        self.coro = coro

        if inspect.iscoroutinefunction(coro) == True:
            self.step()
        else:
            super.set_result(coro)

    
    def step(self):

        GLib.idle_add(self.do_step)

    def do_step(self).

        if super.done() :
            raise RuntimeError("task is already done")

        res = None
        try:
            if super.exception is None:
                res = self.coro.send(None)

            else:
                res = self.coro.throw(super.exception)

        except StopIteration as e:
            super.set_result(e.value)

        except e:
            super.set_exception(e)

        else:
            blocking = getattr(res,"_asyncio_future_blocking")
            if blocking:
                setattr(res,"_asyncio_future_blocking", False)
                res.add_done_callback(self._wakeup)

            elif res is None:
                self.step()        
    
    def _wakeup(self,future):

        try:
            r = future.result()
        except e:
            self.step(e)
        else:
            self.step(None)


def run(coro):

    task = Task(coro)
    return Task

