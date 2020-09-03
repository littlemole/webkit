import gi
gi.require_versions({
    'Gtk':  '3.0',
})

from gi.repository import GLib
import asyncio

class Loop:
    def __init__(self):
        self.loop = asyncio.SelectorEventLoop()
        asyncio.set_event_loop(self.loop)

    def run(self):
        try:
            self.main_context = GLib.MainContext.default()
            self.glib_update()
            self.loop.run_forever()
        finally:
            self.loop.close()

    def stop(self):
        self.loop.stop()

    def glib_update(self):
        while self.main_context.pending():
            self.main_context.iteration(False)
        self.loop.call_later(.01, self.glib_update)
        
    def schedule(self,task):
        self.loop.create_task(task())
