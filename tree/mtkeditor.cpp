#include "common.h"
#include "mtkeditor.h"

#define PROG "[MtkEditor] "

/////////////////////////////////////////////////////////////////

G_DEFINE_TYPE (MtkEditor, mtk_editor, GTK_SOURCE_TYPE_VIEW )

/////////////////////////////////////////////////////////////////

// forwards

static void mtk_editor_finalize(GObject *object);

/////////////////////////////////////////////////////////////////


static void mtk_editor_class_init(MtkEditorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = mtk_editor_finalize;

    if ( !klass)
    {
        g_print( PROG "mtk_editor_class_init: MtkEditorClass is null \n" );
        return;
    }

    gprops<MtkFiletreeClass> props{
        gprop( &MtkFiletree::root, g_param_spec_object ( "directory", "Directory", "widget root directory",  G_TYPE_OBJECT, G_PARAM_READWRITE) )
    };
    props.install(klass);

//    GObjectClass *object_class = G_OBJECT_CLASS (klass);
 //   object_class->init();

   // klass->parent.init(klass);
    /*
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = pywebkit_webview_finalize;

    // GObject properties
    gprops<PywebkitWebviewClass> pywebkitWebviewProperties{
        gprop( &PywebkitWebview::localpath, g_param_spec_string( "local", "Local", "Local file", "index.html", G_PARAM_READWRITE) )
    };

    pywebkitWebviewProperties.install(klass);

    // GObject signals
    Signals signals(klass);
    signals.install("changed", G_TYPE_STRING );


    */
 
}

static void mtk_editor_init(MtkEditor* self)
{
    if ( !self)
    {
        g_print( PROG "mtk_editor_init: MtkEditor is null \n" );
        return;
    }

    self->root = 0;
    self->treeModel = 0;
    self->filter = g_strdup(".*");
    self->cursel = 0;
    self->show_hidden = TRUE;

    self->place_holder = mtk_file_new("<should never be visible>");
    self->place_holder->is_place_holder = TRUE;

    self->empty_dir = mtk_file_new("<empty>");
    self->empty_dir->is_empty = TRUE;


    self->treeModel = gtk_tree_store_new(2, MTK_FILE_TYPE, G_TYPE_STRING);

    GtkTreeViewColumn* column = gtk_tree_view_column_new();

    auto file_name_renderer = gtk_cell_renderer_text_new();
    auto file_type_renderer = gtk_cell_renderer_pixbuf_new();

    gtk_tree_view_column_pack_start(column,file_type_renderer, FALSE);
    gtk_tree_view_column_pack_start(column,file_name_renderer, FALSE);

    gtk_tree_view_column_set_cell_data_func(
        column, 
        file_name_renderer, 
        mtk_filetree_tree_cell_render_file, 
        NULL, 
        [](gpointer data){}
    );

    gtk_tree_view_column_set_cell_data_func(
        column, 
        file_type_renderer, 
        mtk_filetree_tree_cell_render_pix, 
        NULL, 
        [](gpointer data){}
    );

    gtk_tree_view_append_column( (GtkTreeView*)self,column);
    gtk_tree_view_set_tooltip_column((GtkTreeView*)self,1);

    gtk_tree_view_set_model( (GtkTreeView*)self, (GtkTreeModel*)(self->treeModel) );

    g_signal_connect( G_OBJECT (self), "row-expanded", G_CALLBACK(mtk_filetree_expand_row), self);
    g_signal_connect( G_OBJECT (self), "row-activated", G_CALLBACK(mtk_filetree_on_select), self);

    g_signal_connect( G_OBJECT (self), "notify::directory", G_CALLBACK(mtk_filetree_on_root_changed), self);

}

MtkEditor* mtk_editor_new()
{
    MtkEditor* edit;

    edi = (MtkEditor*)g_object_new (MTK_EDITOR_TYPE, NULL);
    return edit;
}


 
static void mtk_editor_finalize(GObject *object)
{
    MtkEditor* self = (MtkEditor*)object;

    g_object_unref(self->source);
    g_object_unref(self->lang_manager);
    g_object_unref(self->buffer);
    g_free(self->path);
}

