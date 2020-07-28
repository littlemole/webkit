import os, os.path, subprocess
from pathlib import Path

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Gdk, GObject, GLib, Pywebkit
from gi.repository.Pywebkit import Webview 
from pygtk.bind import bind,synced,idle_add
from pygtk.ui import UI,DirectoryTree
from pygtk import WebKit, ui
import pygtk


dir = os.path.dirname(os.path.realpath(__file__))

print(dir)

@bind(UI,WebKit)
class Controller(object):

    def __init__(self,*args):

        self.last_action = self.onViewStatus


    def onDocumentLoad(self,*args):

        r = subprocess.run(["git", "status", "."], capture_output=True)
        WebKit.JavaScript(web).setPlainText(r.stdout.decode())


    def onFileOpen(self,*args):

        dir = ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            statusBar.push(ctx, dir )

            tree.clear()
            tree.add_dir(dir)

            r = subprocess.run(["git", "status", dir], capture_output=True)
            WebKit.JavaScript(web).setPlainText(r.stdout.decode())


    def onViewDiff(self,*args):

        f = tree.get_selection()

        r = subprocess.run(["git", "diff", f], capture_output=True)
        print(r)
        WebKit.JavaScript(web).setDiff(r.stdout.decode())

        self.last_action = self.onViewDiff


    def onViewStatus(self,*args):

        f = tree.get_selection()

        r = subprocess.run(["git", "status", f], capture_output=True)
        print(r)
        WebKit.JavaScript(web).setPlainText(r.stdout.decode())

        self.last_action = self.onViewStatus


    def onViewFile(self,*args):

        f = tree.get_selection()

        txt = Path(f).read_text()
        WebKit.JavaScript(web).setPlainText(txt)

        self.last_action = self.onViewFile


    def onContext(self,*args):

        event = pygtk.ui.event(args)
        m = ui["ViewSubMenu"]
        
        Gtk.Menu.popup_at_pointer(m,event)             
        return True

    def onSelect(self,*args):

        f = self.last_action
        f()
 

    def onHelp(self,*args):

        ui.alert("This is the simple pygtk diff viewer using webkit2 based HTML rendering.")


    def onExit(self,*args):

        Gtk.main_quit()


#create controller
controller = Controller()        

#create UI
ui = UI(dir + "/diff.ui.xml")

# tree view
tree = DirectoryTree( ui["fileTreeView"] )
tree.add_dir( os.getcwd() )
# web view 
web = ui["web"]
web.load_uri("file://" + dir + "/diff.html")

statusBar = ui["statusBar"]
ctx = statusBar.get_context_id("status ctx")
statusBar.push(ctx, os.getcwd() )

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
