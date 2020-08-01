
import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk, Gdk, GLib

import pygtk
import pygtk.WebKit as WebKit
import os,sys

uis = []


def event(args):
    for arg in args:
        if isinstance(arg,Gdk.Event):
            return arg
    return None


class UI(object):

    def __init__(self,xml):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(xml)
        self.main = None
        self.ctx = None

    def __getitem__(self,key):
        return self.builder.get_object(key)


    def show(self,mainWindow):
        for ui in uis:
            self.bind(ui)

        objs = self.builder.get_objects()

        if not WebKit.callback is None:

            for obj in objs:

                clazzName = type(obj).__module__ + "." + type(obj).__qualname__

                if clazzName == "gi.repository.Pywebkit.Webview":

                    WebKit.bind(obj,WebKit.callback)

        self.main = self.builder.get_object(mainWindow)
        self.main.show_all()


    def bind(self,controller):
        self.builder.connect_signals(controller)


    def showFileDialog(self,action,title):

        actionButton = Gtk.STOCK_OPEN if (
            action == Gtk.FileChooserAction.OPEN or 
            action == Gtk.FileChooserAction.SELECT_FOLDER ) else (
                Gtk.STOCK_SAVE
            )

        dlg = Gtk.FileChooserDialog(
            title = title,
            parent = self.main,
            action = action,
            buttons =
            (
                Gtk.STOCK_CANCEL,
                Gtk.ButtonsType.CANCEL,
                actionButton,
                Gtk.ButtonsType.OK,
            ),
        )

        dlg.set_default_response(Gtk.ButtonsType.OK)
        response = dlg.run()

        result = None
        if(response == Gtk.ButtonsType.OK):
            result = dlg.get_filename()
            print(result)

        dlg.destroy()
        return result


    def alert(self,msg,**kargs):

        if not "buttons" in kargs:
            kargs["buttons"] = Gtk.ButtonsType.OK

        messagedialog = Gtk.MessageDialog( self.main, message_format=msg, **kargs )
        messagedialog.set_default_response(Gtk.ButtonsType.OK)
        response = messagedialog.run()
        messagedialog.hide()
        return response

    def statusBar(self,id,txt):

        statusBar = self.builder.get_object(id)

        if self.ctx is None:
            self.ctx = statusBar.get_context_id("status ctx")

        statusBar.pop(self.ctx)
        statusBar.push(self.ctx, txt )


#messagedialog = Gtk.MessageDialog(None,
#    flags=Gtk.DialogFlags.MODAL,
#    type=Gtk.MessageType.WARNING,
#    buttons=Gtk.ButtonsType.OK_CANCEL,
#    message_format="This action will cause the universe to stop existing.")


class File(object):

    def __init__(self, file_name, place_holder=False, directory=True, root=False, empty=False):
        self.file_name = file_name
        self.place_holder = place_holder
        self.directory = directory
        self.root = root
        self.empty = empty

    def __str__(self):
        return 'File: name: {}, dir: {}, empty: {}'.\
                format(self.file_name, self.directory, self.empty)



class DirectoryTree:

    PLACE_HOLDER = File('<should never be visible>', place_holder=True)
    EMPTY_DIR = File('<empty>', empty=True)

    def __init__(self,tree,*args,**kargs):

        self.root = None
        self.tree = tree
        self.treeModel = None
        self.contextMenuCB = None
        self.selectCB = None
        self.controller = None

        if "on_context" in kargs:
            self.contextMenuCB = kargs["on_context"]

        if "on_select" in kargs:
            self.selectCB = kargs["on_select"]

        self.compose()


    def set_context_menu(self,cb):

        self.contextMenuCB = cb


    def on_select(self,cb):

        self.selectCB = cb

    def bind(self,controller):
        
        self.controller = controller


    def get_selection(self):

        selection = self.tree.get_selection().get_selected()
        if not selection or selection[1] is None:
            return self.root
        f = self.treeModel.get(selection[1], (0) )
        return f[0].file_name


    def clear(self):

        self.treeModel.clear()


    def compose(self):
        
        self.treeModel = Gtk.TreeStore(object)
        self.tree.set_model(self.treeModel)

        file_name_column = Gtk.TreeViewColumn('file')
        file_name_renderer = Gtk.CellRendererText()
        file_type_renderer = Gtk.CellRendererPixbuf()

        file_name_column.pack_start(file_type_renderer, False)
        file_name_column.pack_start(file_name_renderer, False)

        file_name_column.set_cell_data_func(file_name_renderer, self.tree_cell_render_file)
        file_name_column.set_cell_data_func(file_type_renderer, self.tree_cell_render_pix)
        self.tree.append_column(file_name_column)

        self.tree.connect("row-expanded",self.onFileTreeViewExpand)
        self.tree.connect('button-press-event' , self.button_press_event)

        for ui in pygtk.ui.uis:
            self.bind(ui)

        GLib.idle_add(self.connect_late, self.onSelect)




    def tree_cell_render_file(self,col, renderer, model, tree_iter, user_data):
        _file = model[tree_iter][0]
        label = os.path.basename(_file.file_name)
        hidden = os.path.basename(_file.file_name)[0] == '.'
        label = GLib.markup_escape_text(label)
        if hidden:
            label = '<i>' + label + '</i>'
        renderer.set_property('markup', label)


    def tree_cell_render_pix(self,col, renderer, model, tree_iter, user_data):
        _file = model[tree_iter][0]
        if _file.empty:
            renderer.set_property('stock_id', None)
        elif _file.directory:
            renderer.set_property('stock-id', Gtk.STOCK_OPEN)
        else:
            renderer.set_property('stock-id', Gtk.STOCK_FILE)
            

    def add_dir(self, dir_name, root=None):

        is_root = root == None
        if(is_root):
            self.root = dir_name
            GLib.idle_add(self.tree.expand_row,Gtk.TreePath.new_first(), False)

        if os.path.isdir(dir_name):
            tree_iter = self.treeModel.append(root, [File(dir_name, root=is_root)])
            self.treeModel.append(tree_iter, [DirectoryTree.PLACE_HOLDER])
        else:
            self.treeModel.append(root, [File(dir_name, root=is_root, directory=False)])

    def onFileTreeViewExpand(self,widget, tree_iter, path):

        current_dir = self.treeModel[tree_iter][0]

        place_holder_iter = self.treeModel.iter_children(tree_iter)
        if not self.treeModel[place_holder_iter][0].place_holder:
            return

        paths = os.listdir(current_dir.file_name)
        paths.sort()
        if len(paths) > 0:
            for child_path in paths:
                self.add_dir(os.path.join(current_dir.file_name, child_path), tree_iter)
        else:
            self.treeModel.append(tree_iter, [DirectoryTree.EMPTY_DIR])

        self.treeModel.remove(place_holder_iter)



    def button_press_event(self,treeview, event):

        if event.button == 3: # right click
            #r = self.tree.get_path_at_pos(int(event.x), int(event.y))
            #i = self.treeModel.get_iter(r[0])
            #f = self.treeModel.get(i, (0) )

            if not self.contextMenuCB is None:

                self.contextMenuCB(event) 

            if not self.controller is None:

                if not getattr(self.controller, "onContext") is None:

                    getattr(self.controller, "onContext")(event)



    def connect_late(self,cb):

        self.tree.get_selection().connect("changed", cb)

        #iter = self.treeModel.get_iter_first()
        #self.tree.get_selection().select_iter(iter)


    def onSelect(self,*args):

        if not self.selectCB is None:

            self.selectCB(*args)

        if not self.controller is None:

            if not getattr(self.controller,"onSelect") is None:

                getattr(self.controller,"onSelect")(*args)



def radio_group(**kargs):

    menu = None
    tb = None
    mod = "__main__"

    if "menu" in kargs:
        menu = kargs["menu"]
    if "tb" in kargs:
        tb = kargs["tb"]
    if "mod" in kargs:
        mod = kargs["mod"]
    

    def wrapper(func):

        wrapper.ui = None

        def wrap(*args,**kargs):
        
            if not wrapper.ui:

                main = sys.modules[mod]

                for i in dir(main):

                    t = getattr(main,i)

                    if t is None:
                        continue

                    clazzName = type(t).__module__ + "." + t.__class__.__name__
                    if clazzName == "pygtk.ui.UI":

                        wrapper.ui = t

            if len(args)>1 and ( args[1].get_active() == 0 ):
                return

            if wrapper.ui:
                if len(args)>1 and ( type(args[1]) == gi.repository.Gtk.RadioMenuItem ):
                    if not wrapper.ui[tb].get_active():
                        wrapper.ui[tb].set_active(True)
                    return

            r = func(*args,*kargs)

            if wrapper.ui:
                if wrapper.ui[menu].get_active() == False:
                    wrapper.ui[menu].set_active(True)

            return r

        return wrap

    return wrapper
