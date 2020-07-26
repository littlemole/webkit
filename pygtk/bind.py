
import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk 

import pygtk.WebKit as WebKit
import functools

uis = []

def bind(*args,**kargs):
    
    ui = False
    web = False

    for arg in args:
        if arg is UI:
            ui = True
        if arg is WebKit:
            web = True

    def wrapper(clazz):

        def wrap(*args,**kargs):

            obj = clazz(*args,*kargs)

            if not ui is None:
                uis.append(obj)

            if not web is None:
                WebKit.callback = obj

            return obj 
        return wrap

    return wrapper


def synced(*args,**kargs):

    result = None
    if "result" in kargs:
        result = kargs["result"]

    def wrapper(func):
        @functools.wraps(func)
        def wrap(*args,**kargs):
            r = func(*args,**kargs)
            WebKit.run_async(r)
            if isinstance(r,WebKit.Future) or isinstance(r,WebKit.Task):
                return result
            return r
        return wrap
    return wrapper

class UI(object):

    def __init__(self,xml):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(xml)
        self.main = None

    def __getitem__(self,key):
        return self.builder.get_object(key)


    def show(self,mainWindow):
        for ui in uis:
            self.bind(ui)

        objs = self.builder.get_objects()

        if not WebKit.callback is None:

            for obj in objs:

                clazzName = type(obj).__module__ + "." + type(obj).__qualname__

                if clazzName == "gi.repository.Pywebkit.Webview":

                    print("binding WebKit controller")
                    WebKit.bind(obj,WebKit.callback)

        self.main = self.builder.get_object(mainWindow)
        self.main.show_all()


    def bind(self,controller):
        self.builder.connect_signals(controller)


    def showFileDialog(self,action,title):

        actionButton = Gtk.STOCK_OPEN if action == Gtk.FileChooserAction.OPEN else Gtk.STOCK_CLOSE

        dlg = Gtk.FileChooserDialog(
            title = title,
            parent = self.main,
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


    def alert(self,msg,**kargs):

        if not "buttons" in kargs:
            kargs["buttons"] = Gtk.ButtonsType.OK

        messagedialog = Gtk.MessageDialog( self.main, message_format=msg, **kargs )
        messagedialog.set_default_response(Gtk.ButtonsType.OK)
        response = messagedialog.run()
        messagedialog.hide()
        return response

#messagedialog = Gtk.MessageDialog(None,
#    flags=Gtk.DialogFlags.MODAL,
#    type=Gtk.MessageType.WARNING,
#    buttons=Gtk.ButtonsType.OK_CANCEL,
#    message_format="This action will cause the universe to stop existing.")


