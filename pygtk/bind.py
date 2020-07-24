
import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk 

import pygtk.WebKitDBus as WebKitDBus
import functools

uis = []
webs = []

def bind(*args,**kargs):
    
    ui = False
#    if 'ui' in kargs:

#        ui = True

    web = False
#    if 'web' in kargs:

 #       web = True
    for arg in args:
        if arg is UI:
            ui = True
        if arg is WebKitDBus:
            web = True

    def wrapper(clazz):

        def wrap(*args,**kargs):

            obj = clazz(*args,*kargs)

            if not ui is None:
                uis.append(obj)

            if not web is None:
                WebKitDBus.callback = obj

            return obj 
        return wrap

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

    def __init__(self,xml):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(xml)


    def __getitem__(self,key):
        return self.builder.get_object(key)


    def show(self,mainWindow):
        for ui in uis:
            self.bind(ui)

#        for web in webs:
#            WebKitDBus.bind(WebKitDBus.callback,web)

        self.builder.get_object(mainWindow).show_all()


    def bind(self,controller):
        self.builder.connect_signals(controller)


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

            controller = object.__new__(clazz, *args, **kargs)

            WebKitDBus.callback = controller

            controller.__init__( UI(builder), *args, **kargs)

            builder.connect_signals(controller)

            return controller

        return wrap

    return wrapper
