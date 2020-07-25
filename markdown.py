from pathlib import Path

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

#import json
from gi.repository import Gtk, Pywebkit
from pygtk.bind import bind,synced,UI
import pygtk.WebKitDBus as WebKitDBus
from gi.repository.Pywebkit import Webview #as Webview

#d = WebKitDBus
#w = Pywebkit.Webview

#messagedialog = Gtk.MessageDialog(None,
#    flags=Gtk.DialogFlags.MODAL,
#    type=Gtk.MessageType.WARNING,
#    buttons=Gtk.ButtonsType.OK_CANCEL,
#    message_format="This action will cause the universe to stop existing.")

# connect the response (of the button clicked) to the function
# dialog_response()
#messagedialog.connect("response", self.dialog_response)
# show the messagedialog
#messagedialog.run()


@bind(UI,WebKitDBus)
class Controller(object):

    def onFileOpen(self,*args):

        response = ui.showFileDialog(Gtk.FileChooserAction.OPEN,"Please choose a markdown file")

        if not response is None:
            
            txt = Path(response).read_text()
            WebKitDBus.WebView(web).onFileLoaded(txt)



    @synced
    async def saveFile(self,fn):

        txt = await WebKitDBus.WebView(web).onSaveFile()

        with open(fn, "w") as file:
            print(txt, file=file)


    def onFileSave(self,*args):

        response = ui.showFileDialog(Gtk.FileChooserAction.SAVE,"Please choose a markdown file to save to")

        if not response is None:
            self.saveFile(response)


    def onContext(self,web,menue,event,hit,*args):

        m = ui["ActionSubMenu"]
        Gtk.Menu.popup_at_pointer(m,event) 
        return True

    def onExit(self,*args):
        Gtk.main_quit()

#create controller
controller = Controller()        

#d = WebKitDBus
#w = Pywebkit.Webview

ui = UI("markdown.ui.xml")
# create html widget
#web = Pywebkit.Webview() 
web = ui["web"]
web.connect("context-menu", controller.onContext )
web.load_local_uri("markdown.html")

#ui["scrollWindow"].add(web)

#WebKitDBus.bind(web.uid,controller)
#ui.bind(controller)

#show main window
ui.show("mainWindow")

ui.alert("Hello World")#,buttons=Gtk.ButtonsType.OK)

# start the GUI event main loop
Gtk.main()

        
