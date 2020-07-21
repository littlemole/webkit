import threading

import gi
from gi.repository import GLib
#import WebKitDBus
import pygtk.WebKitDBus as WebKitDBus

class Worker(threading.Thread):

    def __init__(self,future,task):
        threading.Thread.__init__(self) 
        self.future = future
        self.task = task

    @staticmethod
    def schedule(task):

        future = WebKitDBus.Future()
        worker = Worker(future,task)
        worker.start()
        return future

    def run(self):

        r = self.task()
        GLib.idle_add(self.done,r)

    def done(self,r):
        self.future.set_result(r)