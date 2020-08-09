import os.path 

import gi
gi.require_versions({
    'Gtk':  '3.0',
#    'Pywebkit': '0.1'
})
#gi.require_version("GtkSource", "4.0")
from gi.repository import Gio, Gtk, Gdk, GObject, GLib, GtkSource #Pywebkit, 
#from gi.repository.Pywebkit import Webview 
from gi.repository.GtkSource import View
from pygtk.bind import synced,idle_add
from pygtk.ui import UI,DirectoryTree,radio_group
from pygtk.git import Git, GitFile
from pygtk import WebKit
import pygtk

class Editor(GtkSource.View):

    __gtype_name__ = "Editor"

    __gsignals__ = {
        'changed': (GObject.SIGNAL_RUN_FIRST, None,())
    }

    language = GObject.Property(nick="language",type=str, default="")    

    def __init__(self,*args,**kargs):

        GtkSource.View.__init__(self)

        self.path = ""
        self.sourcefile = GtkSource.File()
        self.lang_manager = GtkSource.LanguageManager()

        buffer = self.get_buffer()   
        buffer.connect( "changed", self.onChanged )


    def onChanged(self,*args):

        self.emit("changed")


    def is_modified(self):

        buffer = self.get_buffer()        
        return buffer.get_modified()


    def load(self,file):

        self.path = file
        buffer = self.get_buffer()        
                
        self.sourcefile.set_location(Gio.File.new_for_path(file))

        lang = self.get_property("language")
        if lang:
            buffer.set_language(self.lang_manager.get_language(lang))

        loader = GtkSource.FileLoader.new(buffer, self.sourcefile)
        loader.load_async(0, None, None, None, None, None)

    
    def save(self):

        buffer = self.get_buffer()        

        saver = GtkSource.FileSaver.new(buffer, self.sourcefile)
        saver.save_async(0, None, None, None, None, None)
        buffer.set_modified(False)


    def saveAs(self,file):

        buffer = self.get_buffer()        

        targetfile = Gio.File.new_for_path(file)

        saver = GtkSource.FileSaver.new_with_target(buffer, self.sourcefile, targetfile)
        saver.save_async(0, None, None, None, None, None)


    #def onChanged(self,cb):

       # buffer = self.get_buffer()        
       # buffer.connect( "changed", cb )


    def get_text(self):

        buffer = self.get_buffer()        
        return buffer.get_text( buffer.get_start_iter(), buffer.get_end_iter(),False)


    def set_text(self,txt):

        buffer = self.get_buffer()        
        return buffer.set_text( txt, -1)
