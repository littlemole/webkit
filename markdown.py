import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit, GLib 

import os,sys,socket,json,threading,pprint
import pygtk.WebKitDBus as WebKitDBus
from pygtk.worker import Worker
from pygtk.worker import background
from pygtk.bind import bind
from pygtk.bind import synced
import functools
from pathlib import Path


@background       
def request_task(msg,host,port):

    print("request_task")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host,int(port)))

    size = len(msg)
    totalsent = 0
    while totalsent < size:
        sent = sock.send(msg[totalsent:])
        if sent == 0:
            raise RuntimeError("socket connection broken")
        totalsent = totalsent + sent

    response = bytearray() 
    recv = bytearray()
    while True:
        recv = sock.recv(4096)
        if len(recv) == 0 :
            break
        response = response + recv

    # convert response to unicode
    try:
        response = response.decode("UTF-8")
    except:
        response = response.decode("ISO-8859-1")

    return response


def ui(*args,**kargs):

    def wrapper(clazz):

        builder = Gtk.Builder()
        builder.add_from_file(kargs["xml"])

        @functools.wraps(clazz)
        def wrap(*args,**kargs):

            controller = clazz(builder,*args,**kargs)
            builder.connect_signals(controller)
            return controller

        return wrap

    return wrapper


@bind
@ui(xml="markdown.ui.xml")
class Controller(object):

    def __init__(self,ui,html):

        self.gui = ui

        # create html widget
        self.web = Pywebkit.Webview() 
        self.web.connect("context-menu", self.onContext )
        url = "file://" + os.path.dirname(os.path.realpath(__file__)) + html
        self.web.load_uri(url)

        self.gui.get_object("scrollWindow").add(self.web)
        self.gui.get_object("mainWindow").show_all()


    def onFileOpen(self,*args):

        dlg = Gtk.FileChooserDialog(
            title = "Please choose a mardown file",
            parent = self.gui.get_object("mainWindow"),
            action = Gtk.FileChooserAction.OPEN,
            buttons =
            (
                Gtk.STOCK_CANCEL,
                Gtk.ButtonsType.CANCEL,
                Gtk.STOCK_OPEN,
                Gtk.ButtonsType.OK,
            ),
        )

        dlg.set_default_response(Gtk.ButtonsType.OK)
        response = dlg.run()
            
        if(response == Gtk.ButtonsType.OK):
            fn = dlg.get_filename()
            print(fn)
            txt = Path(fn).read_text()
            WebKitDBus.WebView.onFileLoaded(txt)

        dlg.destroy()


    @synced
    async def saveFile(self,fn):

        txt = await WebKitDBus.WebView.onSaveFile()

        with open(fn, "w") as file:
            print(txt, file=file)


    def onFileSave(self,*args):

        dlg = Gtk.FileChooserDialog(
            title = "Please choose a markdown file to save to",
            parent = self.gui.get_object("mainWindow"),
            action = Gtk.FileChooserAction.SAVE,
            buttons =
            (
                Gtk.STOCK_CANCEL,
                Gtk.ButtonsType.CANCEL,
                Gtk.STOCK_SAVE,
                Gtk.ButtonsType.OK,
            ),
        )

        dlg.set_default_response(Gtk.ButtonsType.OK)
        response = dlg.run()
            
        if(response == Gtk.ButtonsType.OK):

            fn = dlg.get_filename()
            self.saveFile(fn)

        dlg.destroy()


    def onContext(self,web,menue,event,hit,*args):

        m = self.gui.get_object("ActionSubMenu")
        Gtk.Menu.popup_at_pointer(m,event) 
        return True

    def onExit(self,*args):
        Gtk.main_quit()

controller = Controller("/markdown.html")        

# create html widget
#web = Pywebkit.Webview() 
#url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/markdown.html"
#web.load_uri(url)

#controller.gui.get_object("scrollWindow").add(web)
#controller.gui.get_object("mainWindow").show_all()

# start the GUI event main loop
Gtk.main()

        
