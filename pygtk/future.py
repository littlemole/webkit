import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import  GLib

import inspect
import threading
import functools

class Future(object):

    def __init__(self,*args,**kargs):

        self.is_done = False
        self.value = None
        self.exception = None
        self._asyncio_future_blocking = True
        self.cb = None

    def done(self):
        return self.is_done

    def set_result(self,r):
        self.value = r
        self.is_done = True
        if not self.cb is None:
            self.cb(self)

    def set_exception(self,ex):
        self.exception = ex
        self.is_done = True
        if not self.cb is None:
            self.cb(self)

    def add_done_callback(self,cb):
        self.cb = cb
        if self.is_done:
            self.cb(self)


    def result(self):
        if not self.exception is None:
            raise self.exception
        return self.value 
    
    def __await__(self):
        return FutureIter(self)


class FutureIter(object):

    def __init__(self,f,*args,**kargs):

        print("FutureIter: __init__")

        self.state = 0
        self.future = f
    
    def __iter__(self):
        print("FutureIter: __iter__")
        return self

    def __next__(self):
        print("FutureIter: __next__")
        if self.state == 0:
            done = self.future.done()
            if done == False:
                return self.future
            else:
                self.state = 1
        if self.state == 1:
            r = self.future.result()
            print("__next__ " + str(r))
            raise StopIteration(r)


class Task(Future):

    def __init__(self,coro,*args,**kargs):

        super().__init__(self) 
        self.coro = coro

        if inspect.iscoroutine(coro) == True:
            print("Task with coro" + str(coro))
            self.step()
        else:
            print("Task no coro" + str(coro))
            self.set_result(coro)

    
    def step(self,*args):

        GLib.idle_add(self.do_step)


    def do_step(self):

        if self.done() :
            raise RuntimeError("task is already done")

        res = None
        try:
            if self.exception is None:
                res = self.coro.send(None)

            else:
                res = self.coro.throw(self.exception)

        except StopIteration as e:
            print("StopIteration: " + str(e))
            self.set_result(e.value)

        except BaseException as e:
            self.set_exception(e)

        else:
            blocking = getattr(res,"_asyncio_future_blocking")
            if blocking:
                setattr(res,"_asyncio_future_blocking", False)
                res.add_done_callback(self._wakeup)

            elif res is None:
                self.step()        
    
    def _wakeup(self,f):

        try:
            f.result()
        except BaseException as e:
            self.step(e)
        else:
            self.step(None)


def run(coro):

    task = Task(coro)
    return task



class Worker(threading.Thread):

    def __init__(self,f,task):
        threading.Thread.__init__(self) 
        self.future = f
        self.task = task

    @staticmethod
    def schedule(task):

        f = Future()
        worker = Worker(f,task)
        worker.start()
        return f

    def run(self):

        r = self.task()
        GLib.idle_add(self.done,r)

    def done(self,r):
        self.future.set_result(r)


def background(func):

    def decorator(*vargs):

        f = Future()

        def set_result(f,r):
            f.set_result(r)

        def wrapper(*args):

            r = func(*args)
            GLib.idle_add(set_result,f,r)

        t = threading.Thread(target=wrapper, args=vargs )
        t.start()

        return f        

    return decorator



def synced(*args,**kargs):

    result = None

    if "result" in kargs:

        result = kargs["result"]

    def wrapper(func,*args):

        @functools.wraps(func)
        def wrap(*args,**kargs):

            r = func(*args,**kargs)
            run(r)

            if isinstance(r,Future) or isinstance(r,Task):

                return result

            return r

        return wrap

    return wrapper
