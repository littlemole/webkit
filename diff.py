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

            statusBar.push(ctx, dir )

            tree.clear()
            tree.add_dir(dir)

            r = subprocess.run(["bash", "-c", "cd " + dir + " && git status ."], capture_output=True) 
            WebKit.JavaScript(web).setPlainText(r.stdout.decode())


    def onViewDiff(self,*args):
        print("onViewDiff")

        ui["tb_status"].set_active(False)
        ui["tb_file"].set_active(False)
        ui["tb_diff"].set_active(True)

        self.last_action = self.onViewDiff


    def onViewStatus(self,*args):
        print("onViewStatus")

        ui["tb_diff"].set_active(False)
        ui["tb_file"].set_active(False)
        ui["tb_status"].set_active(True)

        self.last_action = self.onViewStatus


    def onViewFile(self,*args):
        print("onViewFile")

        ui["tb_diff"].set_active(False)
        ui["tb_status"].set_active(False)
        ui["tb_file"].set_active(True)

        self.last_action = self.onViewFile

    def onTbViewDiff(self,*args):
        print("onTbViewDiff")
        print(traceback.print_stack())
        f = tree.get_selection()

        r = None
        if os.path.isdir(f):  
            r = subprocess.run(["bash", "-c", "cd " + f + " && git diff ."], capture_output=True)
        else:
            d = os.path.dirname(f)
            n = os.path.basename(f)
            r = subprocess.run(["bash", "-c", "cd " + d + " && git diff " + n], capture_output=True)

        WebKit.JavaScript(web).setDiff(r.stdout.decode())

        if ui["ViewDiffMenuItem"].get_active() == False:
            print("toggle")
            ui["ViewDiffMenuItem"].set_active(True)

        ui["tb_status"].set_active(False)
        ui["tb_file"].set_active(False)

        self.last_action = self.onTbViewDiff


    def onTbViewStatus(self,*args):
        print("onTbViewStatus")
        print(traceback.print_stack())

        f = tree.get_selection()
        r = None
        if os.path.isdir(f):  
            r = subprocess.run(["bash", "-c", "cd " + f + " && git status ."], capture_output=True)
        else:
            d = os.path.dirname(f)
            n = os.path.basename(f)
            r = subprocess.run(["bash", "-c", "cd " + d + " && git status " + n], capture_output=True)

        WebKit.JavaScript(web).setPlainText(r.stdout.decode())

        if ui["ViewStatusMenuItem"].get_active() == False:
            print("toggle")
            ui["ViewStatusMenuItem"].set_active(True)

        ui["tb_diff"].set_active(False)
        ui["tb_file"].set_active(False)

        self.last_action = self.onTbViewStatus


    def onTbViewFile(self,*args):
        print("onTbViewFile")
        print(traceback.print_stack())

        f = tree.get_selection()

        txt = Path(f).read_text()
        WebKit.JavaScript(web).setPlainText(txt)

        if ui["ViewFileMenuItem"].get_active() == False:
            print("toggle")
            ui["ViewFileMenuItem"].set_active(True)

        ui["tb_diff"].set_active(False)
        ui["tb_status"].set_active(False)

        self.last_action = self.onTbViewFile




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

statusBar = ui["statusBar"]
ctx = statusBar.get_context_id("status ctx")
statusBar.push(ctx, os.getcwd() )

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
