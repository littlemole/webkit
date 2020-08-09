# std python batteries
import os.path ,base64 
from pathlib import Path

# Gtk introspection magic
import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})
from gi.repository import Gio, Gtk, Gdk, GObject, GLib, Pywebkit, GtkSource

# import custom Gtk types explicitely to allow loading from xml
from gi.repository.Pywebkit import Webview 
from gi.repository.GtkSource import View

# pygtk mini framework specific imports
from pygtk.bind import synced,idle_add
from pygtk.ui import UI,DirectoryTree,radio_group,accelerator
from pygtk.git import Git, GitFile
from pygtk.editor import Editor
from pygtk import WebKit
import pygtk

dir = os.path.dirname(os.path.realpath(__file__))

e = Editor()



class Controller(object):

    def __init__(self,dir,*args):

        self.last_action = self.onViewStatus
        self.fullscreen = False

        #create UI
        self.ui = UI(dir + "/text.ui.xml", win="mainWindow")

        # tree view
        self.tree = DirectoryTree( self.ui["fileTreeView"], filter=".*\\.dot", showHidden=False )
        self.tree.add_root( GitFile(os.getcwd()) )

        # web view 
        self.web = self.ui["web"]
        self.web.load_uri("file://" + dir + "/text.html")

        # status bar
        self.status_bar( os.getcwd() )

        # window title
        self.ui.main.set_title( os.getcwd() )

        # editor
        self.editor = self.ui["sourceView"] #Editor( self.ui["sourceView"] )
        #self.editor.set_property("language","dot")
        #self.editor.set_monospace(True)
        #self.editor.onChanged(self.onSourceChanged)

        # bind event handlers 
        self.ui.bind(self)

        # show the UI
        self.ui.show()#"mainWindow")

        # IPC channel
        self.JavaScript = WebKit.JavaScript(self.web)


    # UI helpers

    def status_bar(self,status):

        self.ui.statusBar( "statusBar", status )


    def selected_file(self):

        f = self.tree.get_selection()
        f = f if not f is None else self.tree.root
        f = f if not f is None else GitFile( os.getcwd() )
        return f


    # Git  helpers

    def doGit(self, cmd, file=None, param=None, action=None, refresh=False, *args,**kargs):

        if file is None:
            file = self.selected_file().file_name

        print(file)

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


    # Callbacks for JavaScript

    def onDocumentLoad(self,*args):

        print("DOM")
        try:
            self.doGitPlainText( Git.status, file = os.getcwd(), action=self.last_action )
        except BaseException as e:
            print(e)


    def onSubmitCommit(self,msg):
            
        self.doGitPlainText( Git.commit, param=msg, refresh=True)


    def onSelectBranch(self,branch):

        self.doGitPlainText( Git.select_branch, param=branch, refresh=True )


    def onSaveImage(self,data):

        print(data)
        png_header = "data:image/png;base64,"
        if not data.startswith(png_header):
            return

        data = data[len(png_header):]

        bytes = base64.b64decode(data)

        f = self.ui.showFileDialog(
            Gtk.FileChooserAction.SAVE,
            "Please choose target to save this .png image file", 
            #filter=self.ui["dotFilter"] 
        )

        if not f is None:

            newfile=open(f,'wb')
            newfile.write(bytes)
            newfile.close()
    
    # track User Input

    def onSelect(self,*args):

        if self.editor.is_modified():

            r = self.ui.alert(
               "scrape unsaved changes in editor?", 
               buttons=("OK",Gtk.ButtonsType.OK,"Cancel",Gtk.ButtonsType.CANCEL),
               default=Gtk.ButtonsType.CANCEL
            )
            if r == Gtk.ButtonsType.CANCEL:

                self.onViewFile()
                return

        f = self.selected_file()
        self.status_bar( f.file_name )

        if f.directory:
            self.onViewStatus()
        else:
            self.editor.load(f.file_name)


    def onSourceChanged(self,*args):

        dot = self.editor.get_text()

        if dot:
            try:
                self.JavaScript.setDot( self.editor.path, dot )
            except BaseException as e:
                print(e)


    def onSwitchPage(self,notebook,page,num,*args):

        if num == 0:
            self.ui["ViewFilesMenuItem"].set_active(True)
        else:
            self.ui["ViewEditorMenuItem"].set_active(True)


    def onWindowState(self,*args):

        if len(args) > 1:

            state = args[1]

            if state.new_window_state & Gdk.WindowState.FULLSCREEN:

                self.fullscreen = True
            else:

                self.fullscreen = False


    # context menues

    def onContext(self,treeview, event,*args):

        if event.button == 3: # right click
       
            f = self.tree.file_at_pos(event.x,event.y)

            m = None
            if f.directory: 
                m = self.ui["directoryContextMenu"]        
            else:
                m = self.ui["fileContextMenu"]        
            
            Gtk.Menu.popup_at_pointer(m,event)             

        return False


    def onWebContext(self,web,menue,event,*args):

        m = self.ui["webContextMenu"]        

        Gtk.Menu.popup_at_pointer(m,event)             

        # suppress standard webview context menue
        return True 


    # View Menu and Toolbar Handlers

    def onViewFiles(self,*args):

        self.ui["sidePane"].set_current_page(0)
        self.onViewRefresh()


    def onViewFile(self,*args):

        self.ui["sidePane"].set_current_page(1)


    def onViewRefresh(self,*args):

        self.tree.refresh()


    def onViewFullscreen(self,*args):
        
        if self.fullscreen :

            self.ui["mainWindow"].unfullscreen()

        else:

            self.ui["mainWindow"].fullscreen()


    def onExportGraph(self,*args):

        self.JavaScript.onGetGraphImage()



    # File Menu and Toolbar handlers

    @accelerator("<Control>d")
    def onFileOpenDir(self,*args):

        dir = self.ui.showFileDialog(
            Gtk.FileChooserAction.SELECT_FOLDER,
            "Please choose a folder", 
            filter=self.ui["dotFilter"] 
        )

        if not dir is None:

            self.ui["mainWindow"].set_title( dir )

            self.tree.clear()
            self.tree.add_root( GitFile(dir) )

            self.onViewFiles()
            self.doGitPlainText( Git.status, file=dir )


    def onNewDotfile(self,*args):

        f = self.selected_file()
        d = f.filename if f.direcotry else os.path.dirname(f.file_name)

        r = self.ui.showFileDialog(Gtk.FileChooserAction.SAVE,"path to new .dot file",dir=d)

        if not r is None:

            Path(r).touch()
            self.editor.load(f.file_name)
            self.onViewRefresh()


    @accelerator("<Control>o")
    def onFileOpen(self,*args):

        f = self.ui.showFileDialog(
            Gtk.FileChooserAction.OPEN,
            "Please choose a .dot file", 
            filter=self.ui["dotFilter"] 
        )

        if not f is None:

            self.status_bar(f)

            self.editor.load(f)


    @accelerator("<Control>s")
    def onFileSave(self,*args):

        self.editor.save()
        path = os.path.basename(self.editor.path)
        self.status_bar("file " + path + " saved.")


    @accelerator("<Control><Shift>s")
    def onFileSaveAs(self,*args):

        f = self.ui.showFileDialog(
            Gtk.FileChooserAction.SAVE,
            "Please choose target to save this .dot file", 
            filter=self.ui["dotFilter"] 
        )

        if not f is None:

            self.editor.saveAs(f)
            path = os.path.basename(f)
            self.status_bar("file " + path + " saved.")
            self.onViewRefresh()


    def onExit(self,*args):

        Gtk.main_quit()

    

    # Git Menu handlers


    def onViewStatus(self,*args):

        self.doGitPlainText( Git.status, action=self.onViewStatus )
            

    def onGitPull(self,*args):

        self.doGitPlainText( Git.pull, refresh=True )


    def onGitAdd(self,*args):

        self.doGitPlainText( Git.add, refresh=True)


    def onGitCommit(self,*args):

        c = self.doGit( Git.diff_cached )

        self.JavaScript.setCommit( *c )


    def onGitPush(self,*args):

        self.doGitPlainText( Git.push, refresh=True )


    def onGitShowBranches(self,*args):

        c = self.doGit( Git.branches )

        self.JavaScript.setBranches(c["current"], c["branches"])


    def onGitRestore(self,*args):

        self.doGitPlainText( Git.restore, refresh=True )


    def onGitRestoreStaged(self,*args):

        self.doGitPlainText( Git.restore_staged, refresh=True )


    def onGitRestoreOrigin(self,*args):

        self.doGitPlainText( Git.restore_origin, refresh=True )


    def onGitDiffOrigin(self,*args):

        c = self.doGit( Git.diff_origin )

        self.JavaScript.setDiff("ORIGIN: " + c[0],c[1])


    def onGitDiffCached(self,*args):

        c = self.doGit( Git.diff_cached )

        self.JavaScript.setDiff("Indexed but not committed: " + c[0],c[1])


    def onViewDiff(self,*args):

        c = self.doGit( Git.diff, action=self.onViewDiff )

        self.JavaScript.setDiff( *c )


    # help menu

    def onHelp(self,*args):

        self.ui.alert("This is the simple python dotfile viewer using webkit2 based HTML rendering.")



#create controller
controller = Controller(dir)        

# start the GUI event main loop
Gtk.main()

        
