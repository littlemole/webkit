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

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel ; echo && git status") ).strip()

        if txt == "":
            return [self.filename, self.path + " is not a git directory",]

        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def diff(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git diff") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def diff_cached(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git diff --cached") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def add(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git add") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


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

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git checkout") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def view_file(self):

        r = ""
        try:
            if self.is_dir:  
                r = self.bash( self.cmd( "git rev-parse --show-toplevel 2>&1 ; ls -lah" ) )
            else:
                r = self.bash( self.cmd_target( "git rev-parse --show-toplevel 2>&1 ; cat" ) )
#                r = Path(self.path).read_text()

        except BaseException:
            pass

        if r == "":
            return ["eror",""]

        line = r.split("\n")[0]
        body = r[len(line)+1:]
        return [ line, body ]


    def pull(self):

        txt = self.bash( "git rev-parse --show-toplevel && git pull" )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def commit(self,msg):

        r = ""
        try:
            msg = shlex.quote(msg)

            cmd = "cd " + self.cd + " && git rev-parse --show-toplevel && git commit -m'" + msg + "' "

            r = subprocess.run( cmd, shell=True,capture_output=True)

            c = ""
            if r.stdout:
                c = r.stdout.decode()
            else:
                c = r.stderr.decode()        
            
            print(c)
        except BaseException as e:
            print(e)

        if r == "":
            return self.status()

        line = r.split("\n")[0]
        body = r[len(line)+1:]
        return [ line, body ]


    def push(self):

        r = ""
        r = self.bash( "git rev-parse --show-toplevel && git push" )
        if r == "" :
            return self.status()

        line = r.split("\n")[0]
        body = r[len(line)+1:]
        return [ line, body ]


    def bash(self,cmd):

        r = subprocess.run(["bash", "-c", cmd], capture_output=True)

        c = ""
        if r.stdout:
            c = r.stdout.decode()
        #else:
        #    c = r.stderr.decode()        

        return c



class GitFile(pygtk.ui.File):

    def __init__(self, dirname, status = "", place_holder=False, directory=True, root=None, empty=False):
        super().__init__(dirname, place_holder, directory, root,empty)
        self.status = status #kargs["status"] if "status" in kargs else "" 


    def get_status_color(self):

        colors = {
            "black" : "#000000",
            "green" : "#17901B",
            "ref"   : "#FF0000",
            "orange" : "#FF7D4B",
            "blue" : "#5A8DF3"
        }

        X = self.status[0:1]
        Y = self.status[1:2]

        if self.status == "??" :
            return colors["blue"]

        if self.status == "DD" or self.status == "UD":
            return colors["red"]

        if self.status == "UA" or self.status == "AA":
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

    def get_status_icon(self):

        X = self.status[0:1]
        Y = self.status[1:2]

        if self.status == "??" :
            return Gtk.STOCK_DIALOG_QUESTION

        if self.status == "DD" or self.status == "UD":
            return Gtk.STOCK_DIALOG_ERROR

        if self.status == "UA" or self.status == "AA":
            return Gtk.STOCK_DIALOG_ERROR

        if Y == "U":
            return Gtk.STOCK_DIALOG_ERROR

        if Y == "M" or Y == "D" or Y == "R" or Y == "C":
            return Gtk.STOCK_EDIT

        if Y == "A":
            return Gtk.STOCK_DIALOG_INFO

        if X == "M" or Y == "D" or Y == "R" or Y == "C":
            return Gtk.STOCK_APPLY

        if self.directory:
            return Gtk.STOCK_OPEN
        else:
            return Gtk.STOCK_FILE


    def tree_cell_render_pix(self,col, renderer, model, tree_iter, user_data):

        if self.empty:
            renderer.set_property('stock_id', None)
        else:
            renderer.set_property('stock-id', self.get_status_icon())


    def tree_cell_render_file(self,col, renderer, model, tree_iter, user_data):

        label = self.status + " " + os.path.basename(self.file_name)
        hidden = os.path.basename(self.file_name)[0] == '.'
        label = GLib.markup_escape_text(label)
        if hidden:
            label = '<i>' + label + '</i>'

        renderer.set_property('foreground', self.get_status_color())

        renderer.set_property('markup', label)


    def get_children(self, tree_iter):

        result = []

        git_paths = Git(self.file_name).porcelain()

        paths = os.listdir(self.file_name)
        paths.sort()

        if len(paths) > 0:
            for child_path in paths:
                status = ""
                if child_path in git_paths:
                    status = git_paths[child_path]
                
                target = os.path.join(self.file_name, child_path)
                is_dir = os.path.isdir( target )
                result.append( GitFile( target, status,directory=is_dir, root=tree_iter) )

        return result


#class GitTree(pygtk.ui.DirectoryTree):
#
#    def __init__(self, *args, **kargs):
#        super().__init__(*args,**kargs)
#
#    def onFileTreeViewExpand(self,widget, tree_iter, path):
#
#        current_dir = self.treeModel[tree_iter][0]#
#
#        place_holder_iter = self.treeModel.iter_children(tree_iter)
#        if not self.treeModel[place_holder_iter][0].place_holder:
#            return
#
#        children = self.get_children(current_dir,tree_iter)
#
#        if len(children) > 0:
#            for child in children:
#                self.add_entry( child )
#        else:
#            self.treeModel.append(tree_iter, [DirectoryTree.EMPTY_DIR])#
#
#        self.treeModel.remove(place_holder_iter)


#    def add_entry(self, file ):
#
#        if file.directory :
#            tree_iter = self.treeModel.append( file.root, [file] )
#            self.treeModel.append(tree_iter, [DirectoryTree.PLACE_HOLDER])
#        else:
#            self.treeModel.append( file.root, [file] )


#    def add_dir(self, dir_name):
#
#        self.root = dir_name
#        GLib.idle_add(self.tree.expand_row,Gtk.TreePath.new_first(), False)
#
#        file = GitFile(dir_name, "", root=None)
#        self.add_entry(file)

#        if os.path.isdir(dir_name):
#            tree_iter = self.treeModel.append(None, [GitFile(dir_name, "", root=None,)])
#            self.treeModel.append(tree_iter, [DirectoryTree.PLACE_HOLDER])
 #       else:
  #          self.treeModel.append(None, [GitFile(dir_name, "", root=None, directory=False)])



@bind(UI,WebKit)
class Controller(object):

    def __init__(self,*args):

        self.last_action = self.onViewStatus


    def onDocumentLoad(self,*args):

        f = os.getcwd()

        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( c[0], c[1] )

        self.last_action = self.onViewStatus


    def onViewRefresh(self,*args):

        tree.refresh()


    def onGitAdd(self,*args):

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        c = Git(f).add() 

        WebKit.JavaScript(web).setPlainText(c[0],c[1])

        self.onViewRefresh()


    def onGitCheckout(self,*args):

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        c = Git(f).checkout() 

        WebKit.JavaScript(web).setPlainText(c[0],c[1])

        self.onViewRefresh()


    @synced()
    async def onGitPull(self,*args):

        WebKit.JavaScript(web).setPlainText("..running pull..")

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        txt = await Git(f).pull()

        WebKit.JavaScript(web).setPlainText(txt[0],txt[1])

        self.onViewRefresh()


    @synced()
    async def onGitPush(self,*args):

        WebKit.JavaScript(web).setPlainText("..running push..")

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        txt = await Git(f).push()

        WebKit.JavaScript(web).setPlainText(txt[0],txt[1])

        self.onViewRefresh()


    def onGitCommit(self,*args):

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        c = Git(f).diff_cached() 

        WebKit.JavaScript(web).setCommit(c[0], c[1])


    def onSubmitCommit(self,msg):

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        c = Git(f).commit(msg)

        WebKit.JavaScript(web).setPlainText(c[0],c[1])

        self.onViewRefresh()


    def onFileOpen(self,*args):

        dir = ui.showFileDialog(Gtk.FileChooserAction.SELECT_FOLDER,"Please choose a folder")

        if not dir is None:

            ui.statusBar( "statusBar", dir )

            tree.clear()
            tree.add_root(GitFile( dir ) )

            c = Git(dir).status()

            WebKit.JavaScript(web).setPlainText( c[0], c[1] )


    @radio_group(menu="ViewDiffMenuItem", tb="tb_diff")
    def onViewDiff(self,*args):

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        c = Git(f).diff() 

        WebKit.JavaScript(web).setDiff(c[0],c[1])

        self.last_action = self.onViewDiff


    @radio_group(menu="ViewStatusMenuItem", tb="tb_status")
    def onViewStatus(self,*args):
            
        f = tree.get_selection()
        f = f if f else tree.root.file_name

        c = Git(f).status()

        WebKit.JavaScript(web).setPlainText( c[0], c[1] )

        self.last_action = self.onViewStatus


    @radio_group(menu="ViewFileMenuItem", tb="tb_file")
    def onViewFile(self,*args):

        f = tree.get_selection()
        f = f if f else tree.root.file_name

        txt = Git(f).view_file()           

        WebKit.JavaScript(web).setPlainText(txt[0],txt[1])

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
#tree = GitTree( ui["fileTreeView"] )
tree = DirectoryTree( ui["fileTreeView"] )
#tree.add_dir( os.getcwd() )
tree.add_root(GitFile( os.getcwd() ) )
# web view 
web = ui["web"]
web.load_uri("file://" + dir + "/diff.html")

# status bar
ui.statusBar( "statusBar", os.getcwd() )

#show main window
ui.show("mainWindow")

# start the GUI event main loop
Gtk.main()

        
