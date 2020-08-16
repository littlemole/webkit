import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit

import os,sys,socket,json,threading,pprint

from gi.repository.Pywebkit import Webview 
import pygtk.WebKit as WebKit
import pygtk.WebKit as WebKit
from pygtk.bind import synced
from pygtk.menumaker import MenuMaker


def onDone(f):
    print("-------------------------------")
    try:
        print("XXX " + str(f.result()))
    except BaseException as ex:
        print("XXX " + str(ex))


# controller gets signals from webview
class Controller(object):

    def showDlg(self,*args):

        dlg = builder.get_object("HelloWorldDialog")

        dlg.show_all()
        r = dlg.run()
        print(str(r))
        dlg.hide()

    async def sendData(self,data):
        txt = json.dumps(data)
        print(txt)
        msg = json.loads(txt)
        return msg

    def openFile(self,*args):
        print("OPEN")
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

        dlg.set_default_response(Gtk.ButtonsType.OK)
        response = dlg.run()
        #self.text.set_text(dlg.get_filename())
        try:
            if ( response == Gtk.ButtonsType.OK) :
                print("################# dlg closed" + str(response))
                f = WebKit.JavaScript(web).setFilename(dlg.get_filename())
                print("################# set done cb")
                f.add_done_callback( onDone )
        except BaseException as ex:
            print("EX: " + str(ex))

        dlg.destroy()

    @synced()
    async def onActivate(self,event):
        print ("ACtiVE ")
        try :
            #print(event.action_target_value)
            print("\n")
            r = await WebKit.JavaScript(web).setFilename("partytime")
            print("###############+++" + str(r))
        except BaseException as ex:
            print("1111111111ex!!!!!!!!!!!")
            print(str(ex))

    def sendRequest(self,req):
        return json.dumps(req)

    def goCurl(self,w):
        url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/../curl/curl.html"
        print(url)
        web.load_uri(url)

    def goSignal(self,w):
        url = "file://" + os.path.dirname(os.path.realpath(__file__)) + "/msg.html"
        web.load_uri(url)        

    def onContext(self,web,menue,event,hit,*args):
        print("OnContext")
        m = builder.get_object("ActionSubMenu")
        Gtk.Menu.popup_at_pointer(m,event)    
        return True

    def onExit(self,*args):
        Gtk.main_quit()

# instantiate controller and bind signals
controller = Controller()        

path2self = os.path.dirname(os.path.realpath(__file__))
# create html widget
# this is similar to Gtk.WebView
web = Pywebkit.Webview() 
web.connect("context-menu", controller.onContext )
url = "file://" + path2self + "/msg.html"
web.load_uri(url)
print(web.uid)

WebKit.bind(web,controller)

# from here on just standard python gtk
builder = Gtk.Builder()
builder.add_from_file( path2self +"/msg.ui.xml")
builder.connect_signals(controller)

scrollWindow = builder.get_object("scrollWindow")
scrollWindow.add(web)

win = builder.get_object("mainWindow")
win.show_all()

# start the GUI event main loop
Gtk.main()

        