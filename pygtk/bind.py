
import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk, GLib

import pygtk
import pygtk.ui as UI
import pygtk.WebKit as WebKit
import functools
import os

def idle_add(func):

    def wrapper(*args):
        GLib.idle_add(func,*args)
        
    return wrapper


def synced(*args,**kargs):

    result = None

    if "result" in kargs:

        result = kargs["result"]

    def wrapper(func,*args):

        @functools.wraps(func)
        def wrap(*args,**kargs):

            r = func(*args,**kargs)
            WebKit.run_async(r)

            if isinstance(r,WebKit.Future) or isinstance(r,WebKit.Task):

                return result

            return r

        return wrap

    return wrapper
