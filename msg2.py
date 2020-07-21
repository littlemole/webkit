import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit

import os
import socket
import threading
import pprint
import sys
import json

import WebKitDBus
from pygtk.menumaker import MenuMaker


def onDone(f):
    print("-------------------------------")
    try:
        print("XXX " + str(f.result()))
    except BaseException as ex:
        print("XXX " + str(ex))


# controller gets signals from webview
class Controller(object):

    async def sendData(self,data):
        txt = json.dumps(data)
        print(txt)
        msg = json.loads(txt)
        r = await WebKitDBus.View.recvData(msg)
        print("/////////////////" + r)
        #return r
        raise RuntimeError(str(r))

    def openFile(self,*args):
        print("OPEN")
#        print(dir(Gtk.FileChooserDialog))
        #dlg = Gtk.FileChooserDialog("Open..", None, Gtk.FILE_CHOOSER_ACTION_OPEN,(Gtk.STOCK_CANCEL, Gtk.RESPONSE_CANCEL, Gtk.STOCK_OPEN, Gtk.RESPONSE_OK))
        dlg = Gtk.FileChooserDialog(
            title = "Please choose a file",
            parent = win,
            action = Gtk.FileChooserAction.OPEN,
            buttons =
            (
                Gtk.STOCK_CANCEL,
                Gtk.ButtonsType.CANCEL,
                Gtk.STOCK_OPEN,
                Gtk.ButtonsType.OK,
            ),
        )

#        print(dir(dlg))
        response = dlg.run()
        #self.text.set_text(dlg.get_filename())
        try:
            print("################# dlg closed")
            f = WebKitDBus.View.recvFilename(dlg.get_filename())
            print("################# set done cb")
            f.add_done_callback( onDone )
        except BaseException as ex:
            print("EX: " + str(ex))

        dlg.destroy()


async def onActivate(event):
    print ("ACtiVE ")
    print(event.action_target_value)
    print("\n")
    #WebKitDBus.send_signal("recvData","partytime").add_done_callback( lambda x: print(x.result()) )
    try :
        #r = await WebKitDBus.send_signal("recvData","partytime")
        r = await WebKitDBus.View.recvData("partytime")

        print("###############" + str(r))
    except BaseException as ex:
        print("1111111111ex!!!!!!!!!!!")
        print(str(ex))

# instantiate controller and bind signals
controller = Controller()        
WebKitDBus.bind(controller)

# create html widget
# this is the same as Gtk.WebView
web = Pywebkit.Webview() 
url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/signal.html"
web.load_uri(url)
print(web.uid)

# form here on just standard python gtk
# make resizable
scrolledwindow = Gtk.ScrolledWindow()
scrolledwindow.add(web)


#menu_data = {
#    "File" : [
#        [ "New", "onNewFile" ],
#        [ "Open", "onOpenFile" ],
#    ]
#}

menu_data = {
    "File" : [
        [ "New", lambda w : WebKitDBus.run_async(onActivate(w)) ],
        [ "Open", controller.openFile ],
    ]
}

mm = MenuMaker(menu_data)
#mm.connect("onNewFile",  )
#mm.connect("onOpenFile",  )

#menuBar.append(file)

vbox = Gtk.VBox(False, 2)
vbox.pack_start(mm.menuBar, False, False, 0)
vbox.pack_start(scrolledwindow, True, True, 0)


# main window
win = Gtk.Window()     
win.set_default_size(550, 450)   
#win.add(scrolledwindow)
win.add(vbox)
win.connect("delete-event", Gtk.main_quit)
win.show_all()

# start the GUI event main loop
Gtk.main()

        
