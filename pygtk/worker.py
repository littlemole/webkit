import threading

import gi
from gi.repository import GLib
import pygtk.WebKit as WebKit

class Worker(threading.Thread):

    def __init__(self,future,task):
        threading.Thread.__init__(self) 
        self.future = future
        self.task = task

    @staticmethod
    def schedule(task):

        future = WebKit.Future()
        worker = Worker(future,task)
        worker.start()
        return future

    def run(self):

        r = self.task()
        GLib.idle_add(self.done,r)

    def done(self,r):
        self.future.set_result(r)


def background(func):

    def decorator(*vargs):

        f = WebKit.Future()

        def set_result(f,r):
            f.set_result(r)

        def wrapper(*args):

            r = func(*args)
            GLib.idle_add(set_result,f,r)

        t = threading.Thread(target=wrapper, args=vargs )
        t.start()

        return f        

    return decorator