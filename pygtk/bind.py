
import pygtk.WebKitDBus as WebKitDBus
import functools

def bind(clazz):

    @functools.wraps(clazz)
    def wrapper(*args,**kargs):
        obj = clazz(*args,**kargs)
        WebKitDBus.bind(obj)
        return obj

    return wrapper

def synced(func):

    @functools.wraps(func)
    def wrapper(*args,**kargs):
        r = func(*args,**kargs)
        WebKitDBus.run_async(r)
        if isinstance(r,WebKitDBus.Future) or isinstance(r,WebKitDBus.Task):
            return False
        return r
    return wrapper