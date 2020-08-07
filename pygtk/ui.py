
import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import Gtk, Gdk, GLib

import pygtk
import pygtk.WebKit as WebKit
import os,sys,traceback,re

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
#        for ui in uis:
#            self.bind(ui)
#
#        objs = self.builder.get_objects()

 #       if not WebKit.callback is None:

  #          for obj in objs:

   #             clazzName = type(obj).__module__ + "." + type(obj).__qualname__

    #            if clazzName == "gi.repository.Pywebkit.Webview":

     #               WebKit.bind(obj,WebKit.callback)

        self.main = self.builder.get_object(mainWindow)
        self.main.show_all()


    def bind(self,controller):

        self.builder.connect_signals(controller)

        objs = self.builder.get_objects()

        for obj in objs:

            clazzName = type(obj).__module__ + "." + type(obj).__qualname__

            if clazzName == "gi.repository.Pywebkit.Webview":

                WebKit.bind(obj,controller)

        return self


    def showFileDialog(self,action,title, dir=None, filter=None):

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

        if not dir is None:
            dlg.set_current_folder(dir)

        if not filter is None:
            dlg.set_filter(filter)

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

        default = Gtk.ButtonsType.OK if not "default" in kargs else kargs["default"]
        if "default" in kargs:
            del kargs["default"]

        messagedialog = Gtk.MessageDialog( self.main, message_format=msg, **kargs )
        messagedialog.set_default_response(default)
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
        self.hidden = os.path.basename(self.file_name)[0] == '.'


    def __str__(self):
        return 'File: name: {}, dir: {}, empty: {}'.\
                format(self.file_name, self.directory, self.empty)


    def get_tooltip(self):

        return self.file_name


    def tree_cell_render_file(self,col, renderer, model, tree_iter, user_data):
        label = os.path.basename(self.file_name)
        label = GLib.markup_escape_text(label)
        if self.hidden:
            label = '<i>' + label + '</i>'
        renderer.set_property('markup', label)


    def tree_cell_render_pix(self,col, renderer, model, tree_iter, user_data):
        if self.empty:
            renderer.set_property('stock_id', None)
        elif self.directory:
            renderer.set_property('stock-id', Gtk.STOCK_OPEN)
        else:
            renderer.set_property('stock-id', Gtk.STOCK_FILE)


    def get_children(self,tree_iter):

        result = []

        paths = os.listdir(self.file_name)
        paths.sort()

        if len(paths) > 0:
            for child_path in paths:
                
                target = os.path.join(self.file_name, child_path)
                is_dir = os.path.isdir( target )
                result.append( File( target, directory=is_dir, root=tree_iter) )

        return result


class DirectoryTree:

    PLACE_HOLDER = File('<should never be visible>', place_holder=True)
    EMPTY_DIR = File('<empty>', empty=True)

    def __init__(self,tree,filter=".*",showHidden=True,*args,**kargs):

        self.filter = filter
        self.root = None
        self.cursel = ""
        self.tree = tree
        self.treeModel = None
        self.showHidden = showHidden

        self.compose()


    def refresh(self):

        self.clear()
        self.add_root(self.root)


    def search(self, treeiter, path):

        while treeiter != None:

            if self.treeModel[treeiter][0].file_name == path:
                return treeiter
                break

            if self.treeModel.iter_has_child(treeiter):
                childiter = self.treeModel.iter_children(treeiter)
                ret = self.search( childiter, path)
                if ret is not None:
                    return ret

            treeiter = self.treeModel.iter_next(treeiter)


    def select(self,path):

        iter = self.treeModel.get_iter_first()

        r = self.search(iter,path)

        if r is None:
            return

        selection = self.tree.get_selection()
        paths = selection.get_selected()
        selection.select_iter(r)

        self.tree.scroll_to_cell( self.treeModel.get_path(r))


    def get_selection(self):

        selection = self.tree.get_selection().get_selected()
        if not selection or selection[1] is None:
            return None

        f = self.treeModel.get(selection[1], (0) )
        return f[0]


    def clear(self):

        self.treeModel.clear()


    def compose(self):
        
        self.treeModel = Gtk.TreeStore(object,str)
        self.tree.set_model(self.treeModel)

        file_name_column = Gtk.TreeViewColumn('file')
        file_name_renderer = Gtk.CellRendererText()
        file_type_renderer = Gtk.CellRendererPixbuf()

        file_name_column.pack_start(file_type_renderer, False)
        file_name_column.pack_start(file_name_renderer, False)

        file_name_column.set_cell_data_func(file_name_renderer, self.tree_cell_render_file)
        file_name_column.set_cell_data_func(file_type_renderer, self.tree_cell_render_pix)
        self.tree.append_column(file_name_column)

        self.tree.set_tooltip_column(1)

        self.tree.connect("row-expanded",self.onFileTreeViewExpand)
        self.tree.connect("row-activated", self.onSelect)


    def tree_cell_render_file(self,col, renderer, model, tree_iter, user_data):

        _file = model[tree_iter][0]
        return _file.tree_cell_render_file(col,renderer,model,tree_iter, user_data)


    def tree_cell_render_pix(self,col, renderer, model, tree_iter, user_data):

        _file = model[tree_iter][0]
        return _file.tree_cell_render_pix(col,renderer,model,tree_iter, user_data)


    def add_dir(self, dir_name):

        file = File(dir_name, root=None)
        self.add_root(file)


    def add_root(self, file):

        is_refresh = file == self.root
        self.root = file
        self.add_entry(file)

        if not is_refresh:
            iter = self.treeModel.get_iter_first()
            self.tree.get_selection().select_iter(iter)
            self.tree.expand_row(self.treeModel.get_path(iter), False)            
        

    def add_entry(self, file, ):

        if not self.showHidden:
            if file.hidden :
                return

        if file.directory :

            tree_iter = self.treeModel.append( file.root, [file,file.get_tooltip()] )
            self.treeModel.append(tree_iter, [DirectoryTree.PLACE_HOLDER,""])

            if self.cursel == file.file_name:

                self.tree.get_selection().select_iter(tree_iter)
                #self.tree.scroll_to_cell( self.treeModel.get_path(tree_iter))
                GLib.idle_add(self.tree.scroll_to_cell, self.treeModel.get_path(tree_iter) )

            elif self.cursel.startswith(file.file_name):

                self.tree.expand_row(self.treeModel.get_path(tree_iter), False)            
        else:

            if re.match(self.filter,file.file_name):

                tree_iter = self.treeModel.append( file.root, [file,file.get_tooltip()] )

                if self.cursel == file.file_name:

                    self.tree.get_selection().select_iter(tree_iter)
                    #self.tree.scroll_to_cell( self.treeModel.get_path(tree_iter))
                    GLib.idle_add(self.tree.scroll_to_cell, self.treeModel.get_path(tree_iter) )


    def onFileTreeViewExpand(self,widget, tree_iter, path):

        current_dir = self.treeModel[tree_iter][0]

        place_holder_iter = self.treeModel.iter_children(tree_iter)
        if not self.treeModel[place_holder_iter][0].place_holder:
            return

        children = current_dir.get_children(tree_iter)

        if len(children) > 0:
            for child in children:
                self.add_entry( child )
        else:
            self.treeModel.append(tree_iter, [DirectoryTree.EMPTY_DIR])

        self.treeModel.remove(place_holder_iter)


    def onSelect(self,selection,*args):

        self.cursel = self.get_selection().file_name



######################################################


def radio_group(**kargs):

    menu = None
    tb = None

    if "menu" in kargs:
        menu = kargs["menu"]

    if "tb" in kargs:
        tb = kargs["tb"]
    

    def wrapper(func):

        wrapper.ui = None

        def wrap(*args,**kargs):
        
            if not wrapper.ui:

                if len(args) > 0:
                    controller = args[0]

                for a in dir(controller):

                    clazzName = type(getattr(controller,a)).__module__ + "." 
                    clazz = getattr(controller,a).__class__
                    clazzName = clazzName + getattr(clazz,"__name__","unknown")

                    if clazzName == "pygtk.ui.UI":
                        #print("* " + clazzName)
                        wrapper.ui = getattr(controller,a)
                        break

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
