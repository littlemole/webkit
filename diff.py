import os, os.path, subprocess, tempfile, time
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

class Git(object):

    def __init__(self,path):

        self.path = path
        self.is_dir = os.path.isdir(path)

        self.filename = os.path.basename(path)
        self.dirname = os.path.dirname(path)

        self.cd = path if self.is_dir else self.dirname
        self.target = "." if self.is_dir else self.filename


    def cmd(self,cmd):

        return "cd " + self.cd + " && " + cmd + " "


    def cmd_target(self,cmd):

        return self.cmd(cmd) + self.target


    def status(self):

        return self.bash( self.cmd_target("git status") )


    def diff(self):

        return self.bash( self.cmd_target("git diff") )


    def add(self):

        self.bash( self.cmd_target("git add") )
        return self.status()



    def checkout(self):

        return self.bash( self.cmd_target("cgit heckout") )


    def view_file(self):

        r = ""
        try:
            if self.is_dir:  
                r = self.bash( self.cmd( "ls -lah" ) )
            else:
                r = Path(self.path).read_text()

        except BaseException:
            pass

        return r


    async def pull(self):

        return await self.bash_async( "git pull" )


    async def commit(self):

        cmd = "cd " + self.cd + " && gnome-terminal -- bash -c 'git commit' "

        print(cmd)

        c = self.bash( cmd )
        return self.status()


    async def push(self):

        r = await self.bash_async( "git push" )
        if r :
            return r
        return self.status()


    def bash(self,cmd):

        r = subprocess.run(["bash", "-c", cmd], capture_output=True)

        c = ""
        if r.stdout:
            c = r.stdout.decode()
        else:
            c = r.stderr.decode()        

        return c


    def tmp_file(self):

        fp = tempfile.NamedTemporaryFile(delete=False,mode="w+b")
        fn = fp.name
        fp.close()
        return fn


    def cmd_async( self, cmd, tmpfile ):

        return "cd " + self.cd + " && gnome-terminal -- bash -c ' stdbuf -o0 " + cmd + " > " + tmpfile + " 2>&1 '"


    def bash_async(self, command):

        tmpfile = self.tmp_file()

        cmd = self.cmd_async(command, tmpfile) 

        self.bash( cmd )

        f = pygtk.WebKit.Future()

        GLib.timeout_add(1000,self._on_bash_async_done,tmpfile,f,cmd)
        return f


    def _on_bash_async_done(self,tmpfile,f,cmd):

        txt = Path(tmpfile).read_text()
        Path(tmpfile).unlink()

        if not txt:
            txt = ""

        f.set_result(txt)


@bind(UI,WebKit)
class Controller(object):

    def __init__(self,*args):

        self.last_action = self.onViewStatus


    def onDocumentLoad(self,*args):

        f = os.getcwd()

        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( c )

        self.last_action = self.onViewStatus


    def onGitAdd(self,*args):

        f = tree.get_selection()

        c = Git(f).add() 

        WebKit.JavaScript(web).setPlainText(c)

        self.last_action = self.onViewDiff

    def onGitCheckout(self,*args):

        f = tree.get_selection()

        c = Git(f).checkout() 

        WebKit.JavaScript(web).setPlainText(c)

        self.last_action = self.onViewDiff

    @synced()
    async def onGitPull(self,*args):

        WebKit.JavaScript(web).setPlainText("..running pull..")

        f = tree.get_selection()
        txt = await Git(f).pull()

        WebKit.JavaScript(web).setPlainText(txt)


    @synced()
    async def onGitPush(self,*args):

        WebKit.JavaScript(web).setPlainText("..running push..")

        f = tree.get_selection()
        txt = await Git(f).push()

        WebKit.JavaScript(web).setPlainText(txt)


    @synced()
    async def onGitCommit(self,*args):

        WebKit.JavaScript(web).setPlainText("..running commit..")

        f = tree.get_selection()
        txt = await Git(f).commit()

        WebKit.JavaScript(web).setPlainText(txt)


    def onFileOpen(self,*args):

        dir = ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            ui.statusBar( "statusBar", dir )

            tree.clear()
            tree.add_dir(dir)

            c = Git(dir).status()

            WebKit.JavaScript(web).setPlainText( c )


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        f = tree.get_selection()

        c = Git(f).diff() 

        WebKit.JavaScript(web).setDiff(c)

        self.last_action = self.onViewDiff


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):
            
        f = tree.get_selection()

        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( c )

        self.last_action = self.onViewStatus


    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        f = tree.get_selection()

        txt = Git(f).view_file()           

        WebKit.JavaScript(web).setPlainText(txt)

        self.last_action = self.onViewFile


    def onContext(self,*args):

        event = pygtk.ui.event(args)
        m = ui["GitSubMenu"]
        
        Gtk.Menu.popup_at_pointer(m,event)             

    def onWebContext(self,*args):

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

# status bar
ui.statusBar( "statusBar", os.getcwd() )

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
