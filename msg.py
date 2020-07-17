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
        WebKitDBus.View.recvFilename(dlg.get_filename())
        dlg.destroy()


async def onActivate(event):
    print ("ACtiVE ")
    print(event.action_target_value)
    print("\n")
    #WebKitDBus.send_signal("recvData","partytime").add_done_callback( lambda x: print(x.result()) )
    r = await WebKitDBus.send_signal("recvData","partytime")
    print(r)

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

menuBar = Gtk.MenuBar()
file = Gtk.MenuItem(label="_File")
menu1 = Gtk.Menu()
file.set_submenu(menu1)

item1 = Gtk.MenuItem(label="New")
item1.action_target_value = "HUBU!"
item2 = Gtk.MenuItem("Open")

menu1.append(item1)
menu1.append(item2)

#item1.connect( "activate" , onActivate )
item1.connect( "activate" , lambda w : WebKitDBus.run_async(onActivate(w)) )

#lambda w : WebKitDBus.run_async(onClickAsync2(w)) 

menuBar.append(file)

vbox = Gtk.VBox(False, 2)
vbox.pack_start(menuBar, False, False, 0)
vbox.pack_start(scrolledwindow, True, True, 0)


# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
#win.add(scrolledwindow)
win.add(vbox)
win.connect("delete-event", Gtk.main_quit)
win.show_all()

# start the GUI event main loop
Gtk.main()

        
