import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk, GLib, Mtk

from gi.repository.Mtk import Filetree, File, GitFile

import os,sys,socket,json,threading,pprint


class Controller(object):

    def onExit(self,*args):
        Gtk.main_quit()

    def onClick(self,*args):
        print("click")

        #tree.clear()
        #tree.add_root(tree.root,True,".*")

        f = tree.get_selected_file()
        #print(dir(Mtk.GitCmd))
        #exit_code,status,content = Mtk.git_cmd(f,Mtk.GitCmd.STATUS)
        #print(exit_code)
        #print(status)
        #print(content)

        Mtk.git_cmd_async(f,Mtk.GitCmd.STATUS,self.onAsyncGit)
        #out,status = Mtk.bash("ls -lah /")
        #print(str(status)+"\n"+out)

        #out = Mtk.bash_async("git status .",self.onAsyncBash)
        #print(out)

        return False

    def onAsyncGit(self,exit_code,status,out):
        print(exit_code)
        print(status)
        print(out)
    
    def onAsyncBash(self,status,out):
        print("status : " + str(status))
        print(out)

d = os.path.dirname(os.path.realpath(__file__))
print(d)

# create controller
controller = Controller()        

f = GitFile.new(d)
print(f)
# create html widget
tree = Filetree()
tree.set_property("directory", f)
#tree.set_visible(True)

#print("is dir: " + str(f.is_directory))
#tree.add_root(f,True, ".*") #False,".*\\.cpp")


# make resizable
scrolledwindow = Gtk.ScrolledWindow()
#scrolledwindow.set_vexpand(True)
#scrolledwindow.set_hexpand(True)
scrolledwindow.add(tree)

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(scrolledwindow)
tree.connect("row-activated", controller.onClick)
win.connect("delete-event", controller.onExit)
win.show_all()


#f = File.new(d)

#tree.add_root(f,True,".*")

# start the GUI event main loop
Gtk.main()

        
