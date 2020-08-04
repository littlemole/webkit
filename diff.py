import os.path 

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Gdk, GObject, GLib, Pywebkit
from gi.repository.Pywebkit import Webview 
from pygtk.bind import bind,synced,idle_add
from pygtk.ui import UI,DirectoryTree,radio_group
from pygtk.git import Git, GitFile
from pygtk import WebKit
import pygtk

dir = os.path.dirname(os.path.realpath(__file__))


@bind(UI,WebKit)
class Controller(object):

    def __init__(self,*args):

        self.last_action = self.onViewStatus


    def selected_file(self):

        f = tree.get_selection().file_name
        f = f if not f is None else tree.root.file_name
        f = f if not f is None else os.getcwd()
        return f


    def onDocumentLoad(self,*args):

        f = os.getcwd()

        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( *c ) #c[0], c[1] )

        self.last_action = self.onViewStatus


    def onViewRefresh(self,*args):

        tree.refresh()


    def onGitAdd(self,*args):

        f = self.selected_file()

        c = Git(f).add() 

        WebKit.JavaScript(web).setPlainText( *c ) #c[0],c[1])
 
        self.onViewRefresh()


    def onGitRestore(self,*args):

        f = self.selected_file()

        c = Git(f).restore() 

        WebKit.JavaScript(web).setPlainText( *c )

        self.onViewRefresh()


    def onGitRestoreStaged(self,*args):

        f = self.selected_file()

        c = Git(f).restore_staged() 

        WebKit.JavaScript(web).setPlainText( *c )

        self.onViewRefresh()


    def onGitRestoreOrigin(self,*args):

        f = self.selected_file()

        c = Git(f).restore_origin() 

        WebKit.JavaScript(web).setPlainText( *c )

        self.onViewRefresh()


    @synced()
    async def onGitPull(self,*args):

        f = self.selected_file()

        txt = Git(f).pull()

        WebKit.JavaScript(web).setPlainText( *txt )

        self.onViewRefresh()


    @synced()
    async def onGitPush(self,*args):

        f = self.selected_file()

        txt = Git(f).push()

        WebKit.JavaScript(web).setPlainText( *txt )

        self.onViewRefresh()


    def onGitShowBranches(self,*args):

        f = self.selected_file()

        c = Git(f).branches() 

        WebKit.JavaScript(web).setBranches(c["current"], c["branches"])


    def onGitCommit(self,*args):

        f = self.selected_file()

        c = Git(f).diff_cached() 

        WebKit.JavaScript(web).setCommit( *c )


    def onSubmitCommit(self,msg):

        f = self.selected_file()

        c = Git(f).commit(msg)

        WebKit.JavaScript(web).setPlainText( *c )

        self.onViewRefresh()


    def onSelectBranch(self,branch):

        f = self.selected_file()

        c = Git(f).select_branch(branch)

        WebKit.JavaScript(web).setPlainText( *c )

        self.onViewRefresh()


    def onFileOpen(self,*args):

        dir = ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            ui.statusBar( "statusBar", dir )

            tree.clear()
            tree.add_root(GitFile( dir ) )

            c = Git(dir).status()

            WebKit.JavaScript(web).setPlainText( *c )


    def onGitDiffOrigin(self,*args):

        f = self.selected_file()

        c = Git(f).diff_origin() 

        WebKit.JavaScript(web).setDiff("ORIGIN: " + c[0],c[1])


    def onGitDiffCached(self,*args):

        f = self.selected_file()

        c = Git(f).diff_cached() 

        WebKit.JavaScript(web).setDiff("Indexed but not committed: " + c[0],c[1])


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        f = self.selected_file()

        c = Git(f).diff() 

        WebKit.JavaScript(web).setDiff( *c )

        self.last_action = self.onViewDiff


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):
            
        f = self.selected_file()

        print("STATUS: " + str(f))
        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( *c )

        self.last_action = self.onViewStatus


    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        f = self.selected_file()

        txt = Git(f).view_file()           

        WebKit.JavaScript(web).setPlainText( *txt )

        self.last_action = self.onViewFile


    def onContext(self,treeview, event,*args):

        #event = pygtk.ui.event(args)

        if event.button == 3: # right click
       
            m = ui["GitSubMenu"] 
            Gtk.Menu.popup_at_pointer(m,event)             


    def onWebContext(self,*args):

        event = pygtk.ui.event(args)
        m = ui["ViewSubMenu"]
        
        Gtk.Menu.popup_at_pointer(m,event)             
        return True


    def onSelect(self,*args):

        f = self.last_action
        if not f == None:
            self.last_action = None
            f()
            self.last_action = f
 

    def onHelp(self,*args):

        ui.alert("This is the simple pygtk diff viewer using webkit2 based HTML rendering.")


    def onExit(self,*args):

        Gtk.main_quit()


#create controller
controller = Controller()        

#create UI
ui = UI(dir + "/diff.ui.xml")

# tree view
tree = DirectoryTree( ui["fileTreeView"] ) #, filter=".*\\.py" )
tree.add_root( GitFile(os.getcwd()) )

# web view 
web = ui["web"]
web.load_uri("file://" + dir + "/diff.html")

# status bar
ui.statusBar( "statusBar", os.getcwd() )

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
