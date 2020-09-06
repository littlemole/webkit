import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Mtk': '0.1'
})

from gi.repository import Gtk, Gio, Mtk, WebKit2

import os,sys,socket,json,threading,pprint

from gi.repository.Mtk import WebView 

import pymtk.webkit2
import pymtk.future 

from pymtk.future import Worker
from pymtk.future import background
from pymtk.future import synced
from pymtk.webkit2 import idle_add
from pymtk.menumaker import MenuMaker


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
                f = pymtk.webkit2.JavaScript(web).setFilename(dlg.get_filename())
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
            r = pymtk.webkit2.JavaScript(web).setFilename("partytime")
            print("###############+++" + str(r))
            f = await r
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

    def on_request(self, req):

        path = req.get_path()
        print("PATH:" + path)
        print("URI:" + req.get_uri())
        content = "<html><body><h1>HELLO LOCAL URL</h1></body></html>"
        stream = Gio.MemoryInputStream.new_from_data(content.encode(),None)
        req.finish(stream,len(content),"text/html")


# instantiate controller and bind signals
controller = Controller()        

path2self = os.path.dirname(os.path.realpath(__file__))
# create html widget
# this is similar to Gtk.WebView
web = Mtk.WebView() 
web.connect("context-menu", controller.onContext )
url = "file://" + path2self + "/msg.html"
web.load_uri(url)
print(web.uid)

ctx = WebKit2.WebContext.get_default()
ctx.register_uri_scheme( "local", controller.on_request)

pymtk.webkit2.bind(web,controller)

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

        
