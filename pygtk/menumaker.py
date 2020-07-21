
import gi
gi.require_versions({
    'Gtk':  '3.0'
})

from gi.repository import Gtk

class MenuMaker():

    def __init__(self,data):

        self.menuBar = Gtk.MenuBar()

        for key in data:
            mainMenuItem = Gtk.MenuItem(key)

            subMenu = self.add_submenu_items(data[key])
        
            mainMenuItem.set_submenu(subMenu)

            self.menuBar.append(mainMenuItem)

    def add_submenu_items(self,data):

        subMenu = Gtk.Menu()

        for entry in data:

            if callable(entry[1]):

                subMenuItem = Gtk.MenuItem( label=entry[0] )
                subMenuItem.action_target_value = entry[0]
                subMenu.append(subMenuItem)
                subMenuItem.connect("activate",entry[1])
            else:
                subMenuItem = Gtk.MenuItem( label=entry[0] )
                menu = self.add_submenu_items( entry[1] )
                subMenu.append(menu)

        return subMenu        
            
