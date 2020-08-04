import os, os.path, subprocess, tempfile, time, shlex
from pathlib import Path

import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk, Gdk, GObject, GLib
import pygtk


###########################################################


class Git(object):

    def __init__(self,path):

        self.path = path
        self.is_dir = os.path.isdir(path)

        self.filename = os.path.basename(path)
        self.dirname = os.path.dirname(path)

        self.cd = path if self.is_dir else self.dirname
        self.target = "." if self.is_dir else self.filename


    def branches(self):

        result = {
            "current" : "",
            "branches" : []
        }

        txt = self.bash( self.cmd("git branch --no-color") )

        lines = txt.split("\n")
        for line in lines:
            if line[0:1] == "*":
                result["current"] = line[2:]
            else:
                path = line[2:].strip()
                if path != "":
                    result["branches"].append( line[2:] )
         
        print("BRANCHES: " + str(result) )
        return result


    def select_branch(self,branch):

        txt = self.bash( self.cmd("git checkout " + branch) )

        return ["selected branch", txt]


    def has_local_commits(self):

        txt = self.bash( self.cmd("git log \"origin/$(git branch --show-current)..HEAD\" ") )


        print("local commits: (" + txt + ")")
        if txt == "":
            return False
        return True


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


    def diff_origin(self):

        txt = self.bash( self.cmd_target( "git rev-parse --show-toplevel && git diff origin/$(git branch --show-current) -- ") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def add(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git add") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def origin_status(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git diff --name-status origin/$(git branch --show-current) -- " ) )

        result = {}
        lines = txt.split("\n")

        l = len(lines)

        gitroot = lines[0]

        for i in range(1,l):

            line = lines[i]

            status = line[0:1]
            path = line[2:]

            path = gitroot + "/" + path

            if path.startswith(self.cd):
                path = path[len(self.cd)+1:]

            result[path] = status

        return result


    def porcelain(self):

        data = {}

        cmd = self.cmd_target("git rev-parse --show-toplevel && git status --porcelain -uall --ignored ") 
        txt = self.bash( cmd )

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


    def restore(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git restore ") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def restore_staged(self):

        txt = self.bash( self.cmd_target("git rev-parse --show-toplevel && git restore --staged ") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def restore_origin(self):
 
        if self.has_local_commits() == False:
            return ["no local commits","reset aborted." ]

        txt = self.bash( self.cmd("git rev-parse --show-toplevel && git reset HEAD~  ") )
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
            return ["error",""]

        line = r.split("\n")[0]
        body = r[len(line)+1:]
        return [ line, body ]


    def pull(self):

        txt = self.bash( self.cmd( "git rev-parse --show-toplevel && GIT_ASKPASS=true git pull") )
        line = txt.split("\n")[0]
        body = txt[len(line)+1:]
        return [ line, body ]


    def commit(self,msg):

        c = ""
        try:
            msg = shlex.quote(msg)

            cmd = "cd " + self.cd + " && git rev-parse --show-toplevel && git commit -m" + msg + " "
            print(cmd)
            r = subprocess.run( cmd, shell=True,capture_output=True)

            if r.stdout:
                c = r.stdout.decode()
            else:
                c = r.stderr.decode()        
            
            print(c)
        except BaseException as e:
            print(e)

        if c == "":
            return self.status()

        line = c.split("\n")[0]
        body = c[len(line)+1:]
        return [ line, body ]


    def push(self):

        r = ""
        r = self.bash( self.cmd( "git rev-parse --show-toplevel && GIT_ASKPASS=true git push") )
        if r == "" :
            return self.status()

        line = r.split("\n")[0]
        body = r[len(line)+1:]
        return [ line, body ]


    def cmd(self,cmd):

        return "cd " + self.cd + " && " + cmd + " "


    def cmd_target(self,cmd):

        return self.cmd(cmd) + self.target


    def bash(self,cmd):

        print(cmd)
        r = subprocess.run(["bash", "-c", cmd], capture_output=True)

        c = ""
        if r.stdout:
            c = r.stdout.decode()
        if r.stderr:
            c = r.stderr.decode() + "\n" + c
            print("ERR:" + r.stderr.decode()  )

        return c


########################################################


class GitFile(pygtk.ui.File):

    def __init__(self, dirname, status = "", place_holder=False, directory=True, root=None, empty=False):

        super().__init__(dirname, place_holder, directory, root,empty)
        self.status = status


    def get_status_data(self):

        colors = {
            "black" : "#000000",
            "green" : "#17901B",
            "ref"   : "#FF0000",
            "orange" : "#FF7D4B",
            "blue" : "#5A8DF3",
            "gray" : "#AAAAAA"
        }

        X = self.status[0:1]
        Y = self.status[1:2]

        stock = Gtk.STOCK_FILE
        if self.directory:
            stock = Gtk.STOCK_OPEN

        if self.status == "OO" :
            return [ colors["green"], Gtk.STOCK_APPLY ]

        if self.status == "??" :
            return [ colors["black"], Gtk.STOCK_DIALOG_QUESTION ]

        if self.status == "!!" :
            return [ colors["gray"], stock ]

        if self.status == "DD" or self.status == "UD":
            return [ colors["red"], Gtk.STOCK_DIALOG_ERROR ]

        if self.status == "UA" or self.status == "AA":
            return [ colors["red"], Gtk.STOCK_DIALOG_ERROR ]

        if Y == "U":
            return [ colors["red"], Gtk.STOCK_DIALOG_ERROR ]

        if Y == "M" or Y == "D" or Y == "R" or Y == "C":
            return [ colors["orange"], Gtk.STOCK_EDIT ]

        if Y == "A":
            return [ colors["blue"], Gtk.STOCK_DIALOG_INFO ]


        if X == "M" or Y == "D" or Y == "R" or Y == "C":
            return [ colors["green"], Gtk.STOCK_GO_UP ]

        if Y == " ":
            return [ colors["black"], stock ]

        return [ colors["black"], stock ]


    def tree_cell_render_pix(self,col, renderer, model, tree_iter, user_data):

        if self.empty:
            renderer.set_property('stock_id', None)
        else:
            renderer.set_property('stock-id', self.get_status_data()[1] )


    def tree_cell_render_file(self,col, renderer, model, tree_iter, user_data):

        label = os.path.basename(self.file_name)
        hidden = os.path.basename(self.file_name)[0] == '.'
        label = GLib.markup_escape_text(label)
        if hidden:
            label = '<i>' + label + '</i>'

        renderer.set_property('foreground', self.get_status_data()[0] )

        renderer.set_property('markup', label)


    def get_children(self, tree_iter):

        result = []

        git_paths = Git(self.file_name).porcelain()
        origin = Git(self.file_name).origin_status()

        paths = os.listdir(self.file_name)
        paths.sort()

        if len(paths) > 0:
            for child_path in paths:
                status = ""
                if child_path in git_paths:
                    status = git_paths[child_path]
                elif child_path in origin:
                    status = "OO"

                
                target = os.path.join(self.file_name, child_path)
                is_dir = os.path.isdir( target )
                result.append( GitFile( target, status,directory=is_dir, root=tree_iter) )

        return result
