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

# controller gets signals from webview
class Controller(object):

    def sendData(self,data):
        txt = json.dumps(data)
        print(txt)
        msg = json.loads(txt)
        WebKitDBus.View.recvData(msg)

    def openFile(self):
        print("OPEN")
#        print(dir(Gtk.FileChooserDialog))
        #dlg = Gtk.FileChooserDialog("Open..", None, Gtk.FILE_CHOOSER_ACTION_OPEN,(Gtk.STOCK_CANCEL, Gtk.RESPONSE_CANCEL, Gtk.STOCK_OPEN, Gtk.RESPONSE_OK))
        dlg = Gtk.FileChooserDialog(
            "Please choose a file",
            win,
            Gtk.FileChooserAction.OPEN,
            (
                Gtk.STOCK_CANCEL,
                Gtk.ResponseType.CANCEL,
                Gtk.STOCK_OPEN,
                Gtk.ResponseType.OK,
            ),
        )

#        print(dir(dlg))
        response = dlg.run()
        #self.text.set_text(dlg.get_filename())
        WebKitDBus.View.recvFilename(dlg.get_filename())
        dlg.destroy()

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

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(scrolledwindow)
win.connect("delete-event", Gtk.main_quit)
win.show_all()

# start the GUI event main loop
Gtk.main()

        
