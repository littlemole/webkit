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


class Controller(object):

    def __init__(self,dir,*args):

        self.last_action = self.onViewStatus

        #create UI
        self.ui = UI(dir + "/diff.ui.xml")

        # tree view
        self.tree = DirectoryTree( self.ui["fileTreeView"] ) #, filter=".*\\.py" )
        self.tree.add_root( GitFile(os.getcwd()) )

        # web view 
        self.web = self.ui["web"]
        self.web.load_uri("file://" + dir + "/diff.html")

        # status bar
        self.status_bar( os.getcwd() )

        # bind event handlers 
        self.ui.bind(self)

        # IPC channel
        self.JavaScript = WebKit.JavaScript(self.web)

        self.ui.show("mainWindow")


    def status_bar(self,status):

        self.ui.statusBar( "statusBar", status )


    def selected_file(self):

        f = self.tree.get_selection().file_name
        f = f if not f is None else self.tree.root.file_name
        f = f if not f is None else os.getcwd()
        return f


    def doGit(self, cmd, file=None, param=None, action=None, refresh=False, *args,**kargs):

        if file is None:
            file = self.selected_file()

        git = Git(file)

        params = () if param is None else (param,)

        try:
            c = cmd(git,*params)
        except BaseException as e:
            print(e)

        if not action is None:
            self.last_action = action

        if refresh == True:
            self.onViewRefresh()

        return c


    def doGitPlainText(self, cmd, file=None, param=None,action=None, refresh=False, *args,**kargs):

        c = self.doGit(cmd,file,param,action,refresh,*args,**kargs)

        self.JavaScript.setPlainText( *c )


    def onDocumentLoad(self,*args):

        self.doGitPlainText( Git.status, file = os.getcwd(), action=self.last_action )


    def onFileOpen(self,*args):

        dir = self.ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            self.status_bar( dir )

            self.tree.clear()
            self.tree.add_root( GitFile(dir) )

            self.doGitPlainText( Git.status, file=dir )


    def onExit(self,*args):

        Gtk.main_quit()


    def onGitAdd(self,*args):

        self.doGitPlainText( Git.add, refresh=True)


    def onGitRestore(self,*args):

        self.doGitPlainText( Git.restore, refresh=True )


    def onGitRestoreStaged(self,*args):

        self.doGitPlainText( Git.restore_staged, refresh=True )


    def onGitRestoreOrigin(self,*args):

        self.doGitPlainText( Git.restore_origin, refresh=True )


    def onGitPull(self,*args):

        self.doGitPlainText( Git.pull, refresh=True )


    def onGitPush(self,*args):

        self.doGitPlainText( Git.push, refresh=True )


    def onGitShowBranches(self,*args):

        c = self.doGit( Git.branches )

        self.JavaScript.setBranches(c["current"], c["branches"])


    def onGitCommit(self,*args):

        c = self.doGit( Git.diff_cached )

        self.JavaScript.setCommit( *c )


    def onSubmitCommit(self,msg):
            
        self.doGitPlainText( Git.commit, param=msg, refresh=True)


    def onSelectBranch(self,branch):

        self.doGitPlainText( Git.select_branch, param=branch, refresh=True )


    def onGitDiffOrigin(self,*args):

        c = self.doGit( Git.diff_origin )

        self.JavaScript.setDiff("ORIGIN: " + c[0],c[1])


    def onGitDiffCached(self,*args):

        c = self.doGit( Git.diff_cached )

        self.JavaScript.setDiff("Indexed but not committed: " + c[0],c[1])


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        c = self.doGit( Git.diff, action=self.onViewDiff )

        self.JavaScript.setDiff( *c )


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):

        self.doGitPlainText( Git.status, action=self.onViewStatus )
            

    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        self.doGitPlainText( Git.view_file, action=self.onViewFile )


    def onContext(self,treeview, event,*args):

        if event.button == 3: # right click
       
            m = self.ui["GitSubMenu"] 
            Gtk.Menu.popup_at_pointer(m,event)             

        return False


    def onWebContext(self,web,menue,event,*args):

        m = self.ui["ViewSubMenu"]        
        Gtk.Menu.popup_at_pointer(m,event)             

        # suppress standard webview context menue
        return True 
        

    def onSelect(self,*args):

        f = self.last_action
        if not f == None:
            self.last_action = None
            f()
            self.last_action = f


    def onViewRefresh(self,*args):

        self.tree.refresh()


    def onHelp(self,*args):

        self.ui.alert("This is the simple pygtk diff viewer using webkit2 based HTML rendering.")



#create controller
controller = Controller(dir)        

# start the GUI event main loop
Gtk.main()

        
