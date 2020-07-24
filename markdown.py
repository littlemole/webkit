from pathlib import Path

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit
from pygtk.bind import bind,synced,ui
import pygtk.WebKitDBus as WebKitDBus


@bind
@ui(xml="markdown.ui.xml")
class Controller(object):

    def __init__(self,ui):

        # create html widget
        web = Pywebkit.Webview() 
        web.connect("context-menu", self.onContext )
        web.load_local_uri("markdown.html")
        self.web = web

        ui["scrollWindow"].add(web)
        ui.show("mainWindow")
        self.gui = ui


    def onFileOpen(self,*args):

        response = self.gui.showFileDialog(Gtk.FileChooserAction.OPEN,"Please choose a markdown file")

        if not response is None:
            
            txt = Path(response).read_text()
            WebKitDBus.WebView.onFileLoaded(txt)



    @synced
    async def saveFile(self,fn):

        txt = await WebKitDBus.WebView.onSaveFile()

        with open(fn, "w") as file:
            print(txt, file=file)


    def onFileSave(self,*args):

        response = self.gui.showFileDialog(Gtk.FileChooserAction.SAVE,"Please choose a markdown file to save to")

        if not response is None:
            self.saveFile(response)


    def onContext(self,web,menue,event,hit,*args):

        m = self.gui["ActionSubMenu"]
        Gtk.Menu.popup_at_pointer(m,event) 
        return True

    def onExit(self,*args):
        Gtk.main_quit()

#create controller
controller = Controller()        

# start the GUI event main loop
Gtk.main()

        
