import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk, Gfiletree, GLib 

from gi.repository.Gfiletree import Filetree, File, GitFile

import os,sys,socket,json,threading,pprint


class Controller(object):

    def onExit(self,*args):
        Gtk.main_quit()

    def onClick(slef,*args):
        print("click")

        #tree.clear()
        #tree.add_root(tree.root,True,".*")

        out = tree.bash("git status .")
        print(out)

        return False

d = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
print(d)

# create controller
controller = Controller()        

# create html widget
tree = Filetree()
tree.set_visible(True)

f = GitFile.new(d)
tree.add_root(f,True,".*")


# make resizable
scrolledwindow = Gtk.ScrolledWindow()
scrolledwindow.set_vexpand(True)
scrolledwindow.set_hexpand(True)
scrolledwindow.add(tree)

# main window
win = Gtk.Window()     
win.set_default_size(550, 350)   
win.add(scrolledwindow)
win.connect("button-press-event", controller.onClick)
win.connect("delete-event", controller.onExit)
win.show_all()


#f = File.new(d)

#tree.add_root(f,True,".*")

# start the GUI event main loop
Gtk.main()

        
