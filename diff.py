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


    def doGit(self, cmd, file=None, action=None, refresh=False, *args,**kargs):

        if file is None:
            file = self.selected_file()

        git = Git(file)

        print("doGit: " + str(args) )
        c = cmd(git,*args)

        if not action is None:
            self.last_action = action

        if refresh == True:
            self.onViewRefresh()

        return c


    def doGitPlainText(self, cmd, file=None, action=None, refresh=False, *args,**kargs):

        print("doGitPlainText: " + str(args) + " " + "file:" + str(file) )
        c = self.doGit(cmd,file,action,refresh,*args,**kargs)

        WebKit.JavaScript(web).setPlainText( *c )


    def onDocumentLoad(self,*args):

        action = self.last_action if not self.last_action is None else self.onViewStatus

        self.doGitPlainText( Git.status, file = os.getcwd(), action=action )

#        f = os.getcwd()

#        c = Git(f).status()

#        WebKit.JavaScript(web).setPlainText( *c ) #c[0], c[1] )

#        self.last_action = self.onViewStatus


    def onViewRefresh(self,*args):

        tree.refresh()


    def onGitAdd(self,*args):

        self.doGitPlainText( Git.add, refresh=True)


    def onGitRestore(self,*args):

        self.doGitPlainText( Git.restore, refresh=True )


    def onGitRestoreStaged(self,*args):

        self.doGitPlainText( Git.restore_staged, refresh=True )


    def onGitRestoreOrigin(self,*args):

        self.doGitPlainText( Git.restore_origin, refresh=True )


    @synced()
    async def onGitPull(self,*args):

        self.doGitPlainText( Git.pull, refresh=True )


    @synced()
    async def onGitPush(self,*args):

        self.doGitPlainText( Git.push, refresh=True )


    def onGitShowBranches(self,*args):

        c = self.doGit( Git.branches )

        WebKit.JavaScript(web).setBranches(c["current"], c["branches"])


    def onGitCommit(self,*args):

        c = self.doGit( Git.diff_cached )

        WebKit.JavaScript(web).setCommit( *c )


    def onSubmitCommit(self,msg):

        print("COMMIT")
        try:
            self.doGitPlainText( Git.commit, None, None, True, msg)
        except BaseException as e:
            print(e)

#        f = self.selected_file()

#        c = Git(f).commit(msg)

#        WebKit.JavaScript(web).setPlainText( *c )

#        self.onViewRefresh()


    def onSelectBranch(self,branch):

        self.doGitPlainText( Git.select_branch, branch, refresh=True)

#        f = self.selected_file()

#        c = Git(f).select_branch(branch)

#        WebKit.JavaScript(web).setPlainText( *c )

#        self.onViewRefresh()


    def onFileOpen(self,*args):

        dir = ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            ui.statusBar( "statusBar", dir )

            tree.clear()
            tree.add_root( GitFile(dir) )

            self.doGitPlainText( Git.status, file=dir )

#            c = Git(dir).status()

#            WebKit.JavaScript(web).setPlainText( *c )


    def onGitDiffOrigin(self,*args):

        c = self.doGit( Git.diff_origin )

#        f = self.selected_file()

#        c = Git(f).diff_origin() 

        WebKit.JavaScript(web).setDiff("ORIGIN: " + c[0],c[1])


    def onGitDiffCached(self,*args):

        c = self.doGit( Git.diff_cached )

#       f = self.selected_file()

 #       c = Git(f).diff_cached() 

        WebKit.JavaScript(web).setDiff("Indexed but not committed: " + c[0],c[1])


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        c = self.doGit( Git.diff, action=self.onViewDiff )

#        f = self.selected_file()

#        c = Git(f).diff() 

        WebKit.JavaScript(web).setDiff( *c )

#        self.last_action = self.onViewDiff


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):

        self.doGitPlainText( Git.status, action=self.onViewStatus )
            
#        f = self.selected_file()

#        print("STATUS: " + str(f))
#        c = Git(f).status()

#        WebKit.JavaScript(web).setPlainText( *c )

#        self.last_action = self.onViewStatus


    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        self.doGitPlainText( Git.view_file, action=self.onViewFile )

#        f = self.selected_file()

#        txt = Git(f).view_file()           

#        WebKit.JavaScript(web).setPlainText( *txt )

#        self.last_action = self.onViewFile


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

        
