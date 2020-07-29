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
from pygtk.ui import UI,DirectoryTree,radio_group
from pygtk import WebKit#, ui
import pygtk
import traceback

dir = os.path.dirname(os.path.realpath(__file__))


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

            ui.statusBar( "statusBar", dir )

            tree.clear()
            tree.add_dir(dir)

            r = subprocess.run(["bash", "-c", "cd " + dir + " && git status ."], capture_output=True) 
            WebKit.JavaScript(web).setPlainText(r.stdout.decode())


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        f = tree.get_selection()
        r = None
        if os.path.isdir(f):  
            r = subprocess.run(["bash", "-c", "cd " + f + " && git diff ."], capture_output=True)
        else:
            d = os.path.dirname(f)
            n = os.path.basename(f)
            r = subprocess.run(["bash", "-c", "cd " + d + " && git diff " + n], capture_output=True)

        c = r.stdout.decode()

        if not c :
            c = ""
            
        WebKit.JavaScript(web).setDiff(c)

        self.last_action = self.onViewDiff


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):
            
        f = tree.get_selection()
        r = None
        if os.path.isdir(f):  
            r = subprocess.run(["bash", "-c", "cd " + f + " && git status ."], capture_output=True)
        else:
            d = os.path.dirname(f)
            n = os.path.basename(f)
            r = subprocess.run(["bash", "-c", "cd " + d + " && git status " + n], capture_output=True)

        WebKit.JavaScript(web).setPlainText(r.stdout.decode())

        self.last_action = self.onViewStatus


    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        f = tree.get_selection()
        txt = None

        if os.path.isdir(f):  
            r = subprocess.run(["bash", "-c", "cd " + f + " && ls -lah"], capture_output=True)
            txt = r.stdout.decode()
        else:
            txt = Path(f).read_text()
            
        WebKit.JavaScript(web).setPlainText(txt)

        self.last_action = self.onViewFile


    def onContext(self,*args):

        event = pygtk.ui.event(args)
        m = ui["ViewSubMenu"]
        
        Gtk.Menu.popup_at_pointer(m,event)             
        return True

    def onSelect(self,*args):

        print("last action hero")
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

# status bar
ui.statusBar( "statusBar", os.getcwd() )

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
