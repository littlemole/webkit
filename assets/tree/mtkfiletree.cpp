#include "common.h"
#include "mtkfiletree.h"

#define PROG "[MktFileTree] "

/////////////////////////////////////////////////////////////////

G_DEFINE_TYPE (MtkFiletree, mtk_filetree, GTK_TYPE_TREE_VIEW   )

/////////////////////////////////////////////////////////////////

// forwards
static void mtk_filetree_expand_row(MtkFiletree *self, GtkTreeIter* iter, GtkTreePath *path, gpointer* user_data );
static void mtk_filetree_on_select(MtkFiletree *self,  GtkTreePath* path, GtkTreeViewColumn *column, gpointer user_data);
static void mtk_filetree_add_entry(MtkFiletree *self, MtkFile* file);

void mtk_filetree_tree_cell_render_file(
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);

void mtk_filetree_tree_cell_render_pix(
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);

static void mtk_filetree_finalize(GObject *object);

/////////////////////////////////////////////////////////////////


static void mtk_filetree_class_init(MtkFiletreeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = mtk_filetree_finalize;

    if ( !klass)
    {
        g_print( PROG "mtk_filetree_class_init: MtkFiletree is null \n" );
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


static void mtk_filetree_on_root_changed( GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
    MtkFiletree* self = (MtkFiletree*)gobject;
    mtk_filetree_add_root(self, self->root, self->show_hidden, self->filter);
}


static void mtk_filetree_init(MtkFiletree *self)
{
    if ( !self)
    {
        g_print( PROG "mtk_filetree_init: GFiletreeFiletree is null \n" );
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

MtkFiletree* mtk_filetree_new()
{
    MtkFiletree *tree;

    tree = (MtkFiletree*)g_object_new (MTK_TREE_TYPE, NULL);
    return tree;
}


 
static void mtk_filetree_finalize(GObject *object)
{
    MtkFiletree* self = (MtkFiletree*)object;

    g_object_unref(self->root);
    g_object_unref(self->treeModel);
    g_free(self->filter);
    g_free(self->cursel);

    g_object_unref(self->place_holder);
    g_object_unref(self->empty_dir);

}

/////////////////////////////////////////////////////////////////

void mtk_filetree_tree_cell_render_file(
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
)
{
    MtkFile* file = 0;

    gtk_tree_model_get( tree_model, iter, 0, &file, -1);

    return mtk_file_tree_cell_render_file(file,tree_column, cell, tree_model, iter, data);
}

void mtk_filetree_tree_cell_render_pix(
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
)
{
    MtkFile* file = 0;

    gtk_tree_model_get( tree_model, iter, 0, &file, -1);

    return mtk_file_tree_cell_render_pix(file,tree_column, cell, tree_model, iter, data);
}



void mtk_filetree_add_root(MtkFiletree *self, MtkFile* file, bool show_hidden, const gchar* filter)
{
    if(!file)
        return;

    bool is_refresh = file == self->root;

    g_object_ref(file);
    if(self->root)
    {
        g_object_unref(self->root);
    }
    self->root = file;

    self->show_hidden = show_hidden;

    const gchar* f = filter != 0 ? filter : self->filter;

    if( f != self->filter )
    {
        g_free(self->filter);
        self->filter = g_strdup(filter);
    }
    
    mtk_filetree_add_entry(self,file);

    if(!is_refresh)
    {
        GtkTreeIter  iter;
        gtk_tree_model_get_iter_first((GtkTreeModel*)(self->treeModel),&iter);

        GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)self);
        gtk_tree_selection_select_iter( selection, &iter);

        GtkTreePath* path = gtk_tree_model_get_path( (GtkTreeModel*)(self->treeModel), &iter);
        gtk_tree_view_expand_row( (GtkTreeView*)self, path, FALSE);
        gtk_tree_path_free(path);
    }
}

static void mtk_filetree_add_entry(MtkFiletree *self, MtkFile* file)
{
    if(!self->show_hidden)
    {
        if(file->is_hidden)
        {
            return;
        }
    }

    if(file->is_directory)
    {
        GtkTreeIter  iter;
        gtk_tree_store_append (self->treeModel, &iter, file->root );
        gtk_tree_store_set( self->treeModel, &iter, 0, file, 1, mtk_file_get_tooltip(file), -1);

        GtkTreeIter  subIter;
        gtk_tree_store_append (self->treeModel, &subIter, &iter);
        gtk_tree_store_set( self->treeModel, &subIter, 0, self->place_holder, 1, "", -1);

        if ( self->cursel && strncmp(file->file_name,self->cursel,strlen(self->cursel)) == 0 )
        {
            GtkTreeSelection* selection = gtk_tree_view_get_selection( (GtkTreeView*)self );
            gtk_tree_selection_select_iter( selection, &iter);

            GtkTreePath* path = gtk_tree_model_get_path( (GtkTreeModel*)(self->treeModel), &iter);
            gtk_tree_view_scroll_to_cell( (GtkTreeView*)self, path, NULL, FALSE, 0,0);
            gtk_tree_path_free(path);
        }
        else if( self->cursel && strncmp(self->cursel,file->file_name,strlen(file->file_name)) == 0 )
        {
            GtkTreePath* path = gtk_tree_model_get_path( (GtkTreeModel*)(self->treeModel), &iter);
            gtk_tree_view_expand_row( (GtkTreeView*)self, path, FALSE);
            gtk_tree_path_free(path);
        }
    }
    else
    {
        const std::regex rgx(self->filter);
        if(  std::regex_match(file->file_name, rgx) )
        {
            GtkTreeIter  iter;
            gtk_tree_store_append (self->treeModel, &iter, file->root );
            gtk_tree_store_set( self->treeModel, &iter, 0, file, 1, mtk_file_get_tooltip(file), -1);

             if( self->cursel && strncmp(self->cursel,file->file_name,strlen(file->file_name)) == 0 )
             {
                GtkTreeSelection* selection = gtk_tree_view_get_selection( (GtkTreeView*)self );
                gtk_tree_selection_select_iter( selection, &iter);

                GtkTreePath* path = gtk_tree_model_get_path( (GtkTreeModel*)(self->treeModel), &iter);
                gtk_tree_view_scroll_to_cell( (GtkTreeView*)self, path, NULL, FALSE, 0,0);
                gtk_tree_path_free(path);
             }
        }
    }
}


static void mtk_filetree_expand_row(MtkFiletree *self, GtkTreeIter* iter, GtkTreePath *, gpointer* user_data )
{
        MtkFile* current_dir = 0;
        gtk_tree_model_get( (GtkTreeModel*)(self->treeModel), iter, 0, &current_dir, -1);

        GtkTreeIter place_holder_iter;
        gtk_tree_model_iter_children( (GtkTreeModel*)(self->treeModel), &place_holder_iter, iter);

        MtkFile* child = 0;
        gtk_tree_model_get( (GtkTreeModel*)(self->treeModel), &place_holder_iter, 0, &child, -1);

        if ( !child || !child->is_place_holder )
        {
            return;
        }

        GList* glist = mtk_file_get_children(current_dir,iter);

        if ( glist )
        {
            for (GList* l = glist; l != NULL; l = l->next)
            {
                // do something with l->data
    
                MtkFile* f = (MtkFile*)(l->data);
                mtk_filetree_add_entry(self,f);
                g_object_unref(f);
            }        
        }
        else
        {
            GtkTreeIter child_iter;
            gtk_tree_store_append (self->treeModel, &child_iter, iter );
            gtk_tree_store_set( self->treeModel, &child_iter, 0, self->empty_dir, 1, "", -1);
        }
        g_list_free(glist);

        gtk_tree_store_remove(self->treeModel,&place_holder_iter);
}

static void mtk_filetree_on_select(MtkFiletree *self,  GtkTreePath* path, GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)self);

    GtkTreeModel* model = 0;
    GtkTreeIter iter;
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        MtkFile* file = 0;
        gtk_tree_model_get( model, &iter, 0, &file, -1);

        g_free(self->cursel);
        self->cursel = g_strdup( file->file_name );

        g_print("SELECT: %s\n", file->file_name);
    }
}
 
void mtk_filetree_clear(MtkFiletree *self)
{
    gtk_tree_store_clear( self->treeModel );
}


MtkFile* mtk_filetree_file_at_pos(MtkFiletree *self, gint x, gint y)
{
    GtkTreePath* path = 0;
    gboolean hit = gtk_tree_view_get_path_at_pos( (GtkTreeView*)self, x, y, &path, NULL, NULL, NULL );
    if(!hit)
    {
        return NULL;
    }

    GtkTreeIter iter;
    gtk_tree_model_get_iter( (GtkTreeModel*)(self->treeModel), &iter, path);
    gtk_tree_path_free(path);        

    MtkFile* file = 0;
    gtk_tree_model_get( (GtkTreeModel*)(self->treeModel), &iter, 0, &file, -1);

    return file;
}



MtkFile* mtk_filetree_get_selected_file(MtkFiletree *self)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)self);

    GtkTreeModel* model = 0;
    GtkTreeIter iter;
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        MtkFile* file = 0;
        gtk_tree_model_get( model, &iter, 0, &file, -1);

        return file;
    }
    return NULL;
}



/*
static std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}
*/

