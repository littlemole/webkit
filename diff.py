import os, os.path, subprocess, tempfile, time, shlex
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

    def diff_cached(self):

        return self.bash( self.cmd_target("git diff --cached") )

    def add(self):

        self.bash( self.cmd_target("git add") )
        return self.status()


    def porcelain(self):

        data = {}

        cmd = self.cmd_target("git rev-parse --show-toplevel && git status --porcelain -uall ") 
        print(cmd)
        txt = self.bash( cmd )

        print(txt)
        lines = txt.split("\n")
        l = len(lines)

        gitroot = lines[0]

        for i in range(1,l):

            line = lines[i]
            status = line[0:2]
            path = line[3:]

            items = path.split(" ")
            if len(items) > 0:
                path = items[0]

            path = gitroot + "/" + path

            if path.startswith(self.cd):
                path = path[len(self.cd)+1:]

            i = path
            if "/" in i:
                i = i.split("/")[0]

            if i in data:
                s = data[i]
                if status != "!!" and status != "??":
                    data[ i ] = status
            else:                  
                data[ i ] = status

        print(data)

        return data


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


    def commit(self,msg):

        try:
            msg = shlex.quote(msg)

            cmd = "cd " + self.cd + " &&  git commit -m'" + msg + "' "

            r = subprocess.run( cmd, shell=True,capture_output=True)

            c = ""
            if r.stdout:
                c = r.stdout.decode()
            else:
                c = r.stderr.decode()        
            
            print(c)
        except BaseException as e:
            print(e)

        return c if c != "" else self.status()


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
        #cmd = "bash -c 'cd " + self.cd + " && gnome-terminal --wait -- git commit'"


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


class GitFile(pygtk.ui.File):

    def __init__(self, dirname, status, *args, **kargs):
        super().__init__(dirname, *args,**kargs)
        self.status = status #kargs["status"] if "status" in kargs else "" 

class GitTree(pygtk.ui.DirectoryTree):

    def __init__(self, *args, **kargs):
        super().__init__(*args,**kargs)

    def get_status_color(self,file):

        colors = {
            "black" : "#000000",
            "green" : "#17901B",
            "ref"   : "#FF0000",
            "orange" : "#FF7D4B",
            "blue" : "#5A8DF3"
        }

        X = file.status[0:1]
        Y = file.status[1:2]

        if file.status == "??" :
            return colors["blue"]

        if file.status == "DD" or file.status == "UD":
            return colors["red"]

        if file.status == "UA" or file.status == "AA":
            return colors["red"]

        if Y == "U":
            return colors["red"]

        if Y == "M" or Y == "D" or Y == "R" or Y == "C":
            return colors["orange"]

        if Y == "A":
            return colors["blue"]


        if X == "M" or Y == "D" or Y == "R" or Y == "C":
            return colors["green"]

        if Y == " ":
            return colors["black"]


    def tree_cell_render_file(self,col, renderer, model, tree_iter, user_data):
        _file = model[tree_iter][0]
        label = _file.status + " " + os.path.basename(_file.file_name)
        hidden = os.path.basename(_file.file_name)[0] == '.'
        label = GLib.markup_escape_text(label)
        if hidden:
            label = '<i>' + label + '</i>'

        renderer.set_property('foreground', self.get_status_color(_file))

        renderer.set_property('markup', label)

    def onFileTreeViewExpand(self,widget, tree_iter, path):

        print("GitTree.onFileTreeViewExpand")
        current_dir = self.treeModel[tree_iter][0]
        print(str(current_dir.directory) + ":" + current_dir.file_name)

        place_holder_iter = self.treeModel.iter_children(tree_iter)
        if not self.treeModel[place_holder_iter][0].place_holder:
            return

        git_paths = Git(current_dir.file_name).porcelain()

        paths = os.listdir(current_dir.file_name)
        paths.sort()

        if len(paths) > 0:
            for child_path in paths:
                status = ""
                if child_path in git_paths:
                    status = git_paths[child_path]
                self.add_dir(os.path.join(current_dir.file_name, child_path),status, tree_iter)
        else:
            self.treeModel.append(tree_iter, [DirectoryTree.EMPTY_DIR])

        self.treeModel.remove(place_holder_iter)


    def add_dir(self, dir_name,status="", root=None):

        is_root = root == None
        if(is_root):
            self.root = dir_name
            GLib.idle_add(self.tree.expand_row,Gtk.TreePath.new_first(), False)

        if os.path.isdir(dir_name):
            tree_iter = self.treeModel.append(root, [GitFile(dir_name, status, root=is_root,)])
            self.treeModel.append(tree_iter, [DirectoryTree.PLACE_HOLDER])
        else:
            self.treeModel.append(root, [GitFile(dir_name, status, root=is_root, directory=False)])



@bind(UI,WebKit)
class Controller(object):

    def __init__(self,*args):

        self.last_action = self.onViewStatus


    def onDocumentLoad(self,*args):

        f = os.getcwd()

        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( c )

        self.last_action = self.onViewStatus

    def onViewRefresh(self,*args):

        dir = tree.root
        print("--------------------------->"+dir)
        tree.clear()
        tree.add_dir(dir)


    def onGitAdd(self,*args):

        f = tree.get_selection()

        c = Git(f).add() 

        WebKit.JavaScript(web).setPlainText(c)

        self.onViewRefresh()


    def onGitCheckout(self,*args):

        f = tree.get_selection()

        c = Git(f).checkout() 

        WebKit.JavaScript(web).setPlainText(c)

        self.onViewRefresh()


    @synced()
    async def onGitPull(self,*args):

        WebKit.JavaScript(web).setPlainText("..running pull..")

        f = tree.get_selection()
        txt = await Git(f).pull()

        WebKit.JavaScript(web).setPlainText(txt)

        self.onViewRefresh()


    @synced()
    async def onGitPush(self,*args):

        WebKit.JavaScript(web).setPlainText("..running push..")

        f = tree.get_selection()
        txt = await Git(f).push()

        WebKit.JavaScript(web).setPlainText(txt)

        self.onViewRefresh()


    def onGitCommit(self,*args):

        c = Git(tree.root).diff_cached() 

        WebKit.JavaScript(web).setCommit(c)

       # WebKit.JavaScript(web).setPlainText("..running commit..")

       # f = tree.get_selection()
       # txt = Git(f).commit()

       # WebKit.JavaScript(web).setPlainText(txt)

    def onSubmitCommit(self,msg):

        c = Git(tree.root).commit(msg)

       # WebKit.JavaScript(web).setPlainText("..running commit..")

       # f = tree.get_selection()
       # txt = Git(f).commit()

        WebKit.JavaScript(web).setPlainText(c)

        self.onViewRefresh()


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
tree = GitTree( ui["fileTreeView"] )
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

        
