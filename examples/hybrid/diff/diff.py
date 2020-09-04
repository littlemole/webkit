import os.path 

import gi
gi.require_versions({
    'Gtk':  '3.0',
#    'Pywebkit': '0.1'
})

from gi.repository import Gtk, Gdk, GObject, GLib, Mtk
from gi.repository.Mtk import WebView, Filetree, GitFile, GitCmd

#from pymtk.WebView import WebView2
from pymtk.future import synced
from pymtk.ui import UI,radio_group
#from pymtk.git import Git, GitFile
from pymtk import webkit2
from pymtk.webkit2 import idle_add
import pymtk

# set base path for local resource files
UI.set_directory(__file__)
#WV = WebView.new()
WebView.class_set_dir( WebView, os.path.dirname(os.path.realpath(__file__)) )

#print ("DIR:" + str(WebView.dir))

class Controller(object):

    def __init__(self,*args):

        self.last_action = self.onViewStatus

        #create UI
        self.ui = UI("diff.ui.xml", win="mainWindow")

        # tree view
        self.ui.tree.add_root( GitFile.new(os.getcwd()), True, ".*" )

        # web view 
        self.ui.web.load_local_uri("diff.html")

        # status bar
        self.ui.status_bar( os.getcwd() )

        # bind event handlers 
        self.ui.bind(self)
        pymtk.webkit2.bind(self.ui.web,self)

        # IPC channel
        self.JavaScript = pymtk.webkit2.JavaScript(self.ui.web)

        self.ui.show()



    def selected_file(self):

        f = self.ui.tree.get_selected_file()
        f = f if not f is None else self.ui.tree.root
        f = f if not f is None else GitFile.new(os.getcwd())
        return f


    def doGit(self, cmd, file=None, action=None, refresh=False, *args,**kargs):

        if file is None:
            file = self.selected_file()

        r,status,content = Mtk.git_cmd(file,cmd)

        if not action is None:
            self.last_action = action

        if refresh == True:
            self.onViewRefresh()

        return (status,content)


    def doGitPlainText(self, cmd, file=None, action=None, refresh=False, *args,**kargs):

        c = self.doGit(cmd,file,action,refresh,*args,**kargs)

        self.JavaScript.setPlainText( *c )


    def onDocumentLoad(self,*args):

        self.doGitPlainText( GitCmd.STATUS, file = GitFile.new(os.getcwd()), action=self.last_action )


    def onFileOpen(self,*args):

        dir = self.ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            self.ui.status_bar( dir )

            self.ui.tree.clear()
            self.ui.tree.add_root( GitFile(dir) )

            self.doGitPlainText( GitCmd.STATUS, file=dir )


    def onExit(self,*args):

        Gtk.main_quit()


    def onGitAdd(self,*args):

        self.doGitPlainText( GitCmd.ADD, refresh=True)


    def onGitRestore(self,*args):

        self.doGitPlainText( GitCmd.RESTORE, refresh=True )


    def onGitRestoreStaged(self,*args):

        self.doGitPlainText( GitCmd.RESTORE_STAGED, refresh=True )


    def onGitRestoreOrigin(self,*args):

        self.doGitPlainText( GitCmd.RESTORE_ORIGIN, refresh=True )


    def onGitPull(self,*args):

        self.doGitPlainText( GitCmd.PULL, refresh=True )


    def onGitPush(self,*args):

        self.doGitPlainText( GitCmd.PUSH, refresh=True )


    def onGitShowBranches(self,*args):

        c = self.doGit( GitCmd.BRANCHES )

        self.JavaScript.setBranches(c[0], c[1].strip().split("\n"))


    def onCreateBranch(self,branch):

        f = self.selected_file()

        r = Mtk.git_create_branch( f, branch )

        self.onGitShowBranches()


    def onDeleteBranch(self,branch):

        f = self.selected_file()

        r = Mtk.git_delete_branch( f, branch )

        self.onGitShowBranches()


    def onGitCommit(self,*args):

        c = self.doGit( GitCmd.DIFF_CACHED )

        self.JavaScript.setCommit( *c )


    def onSubmitCommit(self,msg):
            
        f = self.selected_file()

        r,status,contents = Mtk.git_commit( f, msg )

        self.JavaScript.setPlainText(status,contents)



    def onSelectBranch(self,branch):

        f = self.selected_file()

        r = Mtk.git_switch_branch( f, branch )

        self.onGitShowBranches()


    def onGitDiffOrigin(self,*args):

        c = self.doGit( GitCmd.DIFF_ORIGIN )

        self.JavaScript.setDiff("ORIGIN: " + c[0],c[1])


    def onGitDiffCached(self,*args):

        c = self.doGit( GitCmd.DIFF_CACHED )

        self.JavaScript.setDiff("Indexed but not committed: " + c[0],c[1])


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        c = self.doGit( GitCmd.DIFF, action=self.onViewDiff )

        self.JavaScript.setDiff( *c )


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):

        self.doGitPlainText( GitCmd.STATUS, action=self.onViewStatus )
            

    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        self.doGitPlainText( GitCmd.VIEW_FILE, action=self.onViewFile )


    def onContext(self,treeview, event,*args):

        if event.button == 3: # right click
       
            m = self.ui.GitSubMenu 
            Gtk.Menu.popup_at_pointer(m,event)             

        return False


    def onWebContext(self,web,menue,event,*args):

        m = self.ui.ViewSubMenu       
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

        self.ui.tree.refresh()


    def onHelp(self,*args):

        self.ui.alert("This is the hybrid pymtk diff viewer using webkit2 based HTML rendering.")



#create controller
controller = Controller()        

# start the GUI event main loop
Gtk.main()

        
