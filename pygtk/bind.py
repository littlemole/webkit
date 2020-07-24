
import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk 

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

class UI(object):

    def __init__(self,builder):
        self.builder = builder


    def __getitem__(self,key):
        return self.builder.get_object(key)


    def show(self,mainWindow):
        self.builder.get_object(mainWindow).show_all()


    def showFileDialog(self,action,title):

        actionButton = Gtk.STOCK_OPEN if action == Gtk.FileChooserAction.OPEN else Gtk.STOCK_CLOSE

        dlg = Gtk.FileChooserDialog(
            title = title,
            parent = self.builder.get_object("mainWindow"),
            action = action,
            buttons =
            (
                Gtk.STOCK_CANCEL,
                Gtk.ButtonsType.CANCEL,
                actionButton,
                Gtk.ButtonsType.OK,
            ),
        )

        dlg.set_default_response(Gtk.ButtonsType.OK)
        response = dlg.run()

        result = None
        if(response == Gtk.ButtonsType.OK):
            result = dlg.get_filename()
            print(result)

        dlg.destroy()
        return result

def ui(*args,**kargs):

    def wrapper(clazz):

        builder = Gtk.Builder()
        builder.add_from_file(kargs["xml"])

        @functools.wraps(clazz)
        def wrap(*args,**kargs):

            controller = clazz( UI(builder),*args,**kargs)
            builder.connect_signals(controller)
            return controller

        return wrap

    return wrapper
