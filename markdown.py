from pathlib import Path

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Pywebkit
from gi.repository.Pywebkit import Webview 
from pygtk.bind import bind,synced,UI
import pygtk.WebKit as WebKit


@bind(UI,WebKit)
class Controller(object):

    def getText(self):

        buffer = ui["textEdit"].get_buffer()
        range = buffer.get_bounds()
        return buffer.get_text(range[0],range[1],False)

    def onFileOpen(self,*args):

        response = ui.showFileDialog(Gtk.FileChooserAction.OPEN,"Please choose a markdown file")

        if not response is None:
            
            txt = Path(response).read_text()
            ui["textEdit"].get_buffer().set_text(txt)
            WebKit.JavaScript(web).setMarkup(txt)


    def onKeyUp(self,*args):

        txt = self.getText()
        WebKit.JavaScript(web).setMarkup(txt)


    def saveFile(self,fn):

        txt = self.getText()

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

#create UI
ui = UI("markdown.ui.xml")
 
web = ui["web"]
web.load_local_uri("markdown.html")

# without @bind we would have to:
# WebKit.bind(web,controller)
# ui.bind(controller)

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
