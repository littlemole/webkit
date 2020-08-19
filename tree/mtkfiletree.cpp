#include "mtkfiletree.h"
#include <glib-object.h>
#include <gio/gio.h>
#include "gprop.h"
#include <sstream>
#include <dlfcn.h>
#include <libgen.h>
#include <map>
#include <regex>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define PROG "[MktFileTree] "


std::string escape_shell( const std::string& cmd )
{
    std::ostringstream oss;
    const gchar* p= cmd.c_str();
    while(*p)
    {
        if( *p == '\'')
        {
            oss << "\\'";
        }
        else
        {
            oss << *p;
        }
        p++;
    }        
    return oss.str();
}


/**
 * SECTION: GFileTreeFile
 * @short_description: A Gtk File Tree File GObject
 *
 * 
 */

G_DEFINE_TYPE (MtkFile, mtk_file, G_TYPE_OBJECT   )


static void mtk_file_init(MtkFile *file)
{
    if ( !file)
    {
        g_print( PROG "mtk_file_init: MtkFile is null \n" );
        return;
    }
    file->root = 0;
    file->file_name = 0;
    file->is_place_holder = FALSE;
    file->is_directory = TRUE;
    file->is_empty = FALSE;
    file->is_hidden = FALSE;

}
 
static void mtk_file_finalize(GObject *object)
{
    MtkFile* file = (MtkFile*)object;

    g_free(file->file_name);

    if(file->root)
    {
        gtk_tree_iter_free(file->root);
    }
}

gchar* virtual_mtk_file_get_tooltip(MtkFile* self)
{
    return self->file_name;
}

std::vector<std::string> listdir(const std::string& dirname)
{
    std::vector<std::string> result;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (dirname.c_str())) != NULL) 
    {
        while ((ent = readdir (dir)) != NULL) 
        {
            int l = strlen(ent->d_name);
            if ( l == 1 && ent->d_name[0] == '.')
            {
                continue;
            }
            if ( l == 2 && ent->d_name[0] == '.' && ent->d_name[1] == '.')
            {
                continue;
            }

            result.push_back( ent->d_name );
        }
        closedir (dir);
    } 
    return result;
}

GList* virtual_mtk_file_get_children(MtkFile* self, GtkTreeIter* iter)
{
    GList* glist = 0;

    if(!self->is_directory)
    {
        return glist;
    }

    std::vector<std::string> children =  listdir(self->file_name);

    for( auto& child: children)
    {
        std::ostringstream oss;
        oss << self->file_name << "/" << child;

        MtkFile* f = mtk_file_new( oss.str().c_str() );
        f->root = gtk_tree_iter_copy(iter);
        glist = g_list_append(glist,f);
    }

    return glist;
}

void virtual_mtk_file_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    )
{
    gchar* bn = mtk_file_get_basename(self);
    gchar* c = g_markup_escape_text(bn,-1);

    std::ostringstream oss;
    if ( self->is_hidden)
    {
        oss << "<i>" << c << "</i>";
    }
    else
    {
        oss << c;
    }
    g_free(c);
    
    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );
    g_value_set_string( &gv, oss.str().c_str() );

    g_object_set_property( (GObject*)cell, "markup", &gv);

    g_free(bn);
}


void virtual_mtk_file_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    )
{
    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );

    if ( self->is_empty)
    {
        g_value_set_static_string( &gv, NULL );

        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
    else if( self->is_directory )
    {
        g_value_set_static_string( &gv, "gtk-open" );

        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
    else
    {
        g_value_set_static_string( &gv, "gtk-file" );

        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
}


static void mtk_file_class_init(MtkFileClass *klass)
{
    if ( !klass)
    {
        g_print( PROG "mtk_file_class_init: MtkFileClass is null \n" );
        return;
    }

    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = mtk_file_finalize;

    klass->get_tooltip = &virtual_mtk_file_get_tooltip;
    klass->get_children = &virtual_mtk_file_get_children;
    klass->tree_cell_render_file = &virtual_mtk_file_tree_cell_render_file;
    klass->tree_cell_render_pix = &virtual_mtk_file_tree_cell_render_pix;
}

MtkFile* mtk_file_new( const gchar* fn)
{
    MtkFile* file;

    file = (MtkFile*)g_object_new(MTK_FILE_TYPE, NULL);

    file->file_name = g_strdup(fn);

    struct stat st;
    stat(fn,&st);

    if( S_ISDIR(st.st_mode) )
    {
        file->is_directory = TRUE;
    }
    else
    {
        file->is_directory = FALSE;
    }

    char* tmp = g_strdup(fn);
    char* bn = basename(tmp);

    if( bn[0] == '.') {
        file->is_hidden = TRUE;
    }

    g_free(tmp);
    return file;
}

void mtk_file_set_path(MtkFile* file, gchar* fn)
{
    g_free(file->file_name);
    file->file_name = g_strdup(fn);

    struct stat st;
    stat(fn,&st);

    if( S_ISDIR(st.st_mode) )
    {
        file->is_directory = TRUE;
    }
    else
    {
        file->is_directory = FALSE;
    }

    char* tmp = g_strdup(fn);
    char* bn = basename(tmp);

    if( bn[0] == '.') {
        file->is_hidden = TRUE;
    }
    g_free(tmp);
}

gchar* mtk_file_get_path(MtkFile* file)
{
    return g_strdup(file->file_name);
}

gchar* mtk_file_get_parent(MtkFile* file)
{
    char* tmp = g_strdup(file->file_name);
    char* p = dirname(tmp);
    char* r = g_strdup(p);
    g_free(tmp);
    return r;

}
 
gchar* mtk_file_get_basename(MtkFile* file)
{
    char* tmp = g_strdup(file->file_name);
    char* bn = basename(tmp);
    char* r = g_strdup(bn);
    g_free(tmp);
    return r;
}

// virtuals

gchar* mtk_file_get_tooltip(MtkFile* self)
{
    MtkFileClass *klass;

    klass = MTK_FILE_GET_CLASS (self);

    return klass->get_tooltip (self);
}



GList* mtk_file_get_children(MtkFile* self, GtkTreeIter* iter)
{
    MtkFileClass *klass;

    klass = MTK_FILE_GET_CLASS (self);

    return klass->get_children (self,iter);
}


void mtk_file_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
)
{
    MtkFileClass *klass;

    klass = MTK_FILE_GET_CLASS (self);

    return klass->tree_cell_render_file( self, tree_column, cell, tree_model, iter, data);
}


void mtk_file_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
)
{
    MtkFileClass *klass;

    klass = MTK_FILE_GET_CLASS (self);

    return klass->tree_cell_render_pix( self, tree_column, cell, tree_model, iter, data);
}

/**
 * SECTION: GFileTree
 * @short_description: A Gtk File Tree widget
 *
 * 
 */

G_DEFINE_TYPE (MtkFiletree, mtk_filetree, GTK_TYPE_TREE_VIEW   )


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

static void mtk_filetree_expand_row(MtkFiletree *self, GtkTreeIter* iter, GtkTreePath *path, gpointer* user_data );
static void mtk_filetree_on_select(MtkFiletree *self,  GtkTreePath* path, GtkTreeViewColumn *column, gpointer user_data);
static void mtk_filetree_add_entry(MtkFiletree *self, MtkFile* file);


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
}


void mtk_filetree_add_root(MtkFiletree *self, MtkFile* file, bool show_hidden, const gchar* filter)
{
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


static void mtk_filetree_class_init(MtkFiletreeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = mtk_filetree_finalize;

    if ( !klass)
    {
        g_print( PROG "mtk_filetree_class_init: MtkFiletree is null \n" );
        return;
    }


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


MtkFiletree* mtk_filetree_new()
{
    MtkFiletree *tree;

    tree = (MtkFiletree*)g_object_new (MTK_TREE_TYPE, NULL);
    return tree;
}


static std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}

class AsyncBash 
{
public:
    int status = 0;
    bool done = false;
    GCancellable* cancellable = 0;      
    GSubprocess* process = 0;
    GDataInputStream* stream = 0;  
    gchar* buffer = 0;
    MtkAsyncBashCallbackFunc cb = 0;
    gpointer user_data = 0;
};


void mtk_bash_on_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    g_print("mtk_filetree_bash_on_finish \n");

    GSubprocess* process = (GSubprocess*)source_object;

    GError* error=0;
    gboolean success = g_subprocess_wait_check_finish ( process, res, &error);

    AsyncBash* ab = (AsyncBash*) user_data;

    ab->status = g_subprocess_get_exit_status(process);

    g_print("mtk_filetree_bash_on_finish : %i\n", ab->status);

    ab->done = true;
    if(ab->stream == 0)
    {
        g_object_unref(ab->process);
        g_print("DATA1: %s\n", ab->buffer );

        ab->cb(ab->status,ab->buffer,ab->user_data);

        delete ab;
    }
}

void mtk_bash_on_data(GObject *source_object, GAsyncResult *res, gpointer user_data);

void mtk_bash_queue_read(AsyncBash *ab)
{

    g_data_input_stream_read_line_async( ab->stream, 0, ab->cancellable, mtk_bash_on_data, ab );
}

void mtk_bash_on_data(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    GError* error = 0;
    gsize len = 0;
    char* line = g_data_input_stream_read_line_finish_utf8( (GDataInputStream*)source_object, res, &len, &error);

    AsyncBash* ab = (AsyncBash*) user_data;

    if(line && len>0)
    {
        gchar* c = 0;
        if(ab->buffer)
        {
            std::ostringstream oss;
            oss << ab->buffer << line << "\n";

            g_free(ab->buffer);
            ab->buffer = g_strdup(oss.str().c_str());
        }
        else
        {
            std::ostringstream oss;
            oss << line << "\n";
            ab->buffer = g_strdup(oss.str().c_str());
        }
    }
    if(line && len>=0)
    {
        mtk_bash_queue_read(ab);
    }
    else if ( !line ||  len<0)
    {
        g_input_stream_close((GInputStream*)ab->stream,NULL,NULL);
        ab->stream = 0;

        if(ab->done == true)
        {
            g_object_unref(ab->process);

            ab->cb(ab->status,ab->buffer,ab->user_data);

            delete ab;
        }
    }
}




void mtk_bash_async(const gchar* cmd, MtkAsyncBashCallbackFunc cb, gpointer user_data)
{
    g_print("mtk_bash_async: %s\n", cmd );

    AsyncBash* ab = new AsyncBash;    
    ab->user_data = user_data;
    ab->cb = (MtkAsyncBashCallbackFunc) cb;

    GError* error=0;
    ab->process = g_subprocess_new(
        (GSubprocessFlags)(G_SUBPROCESS_FLAGS_STDOUT_PIPE|G_SUBPROCESS_FLAGS_STDERR_MERGE),
        &error,
        "/bin/bash",
        "-c",
        cmd,
        NULL
    );

    if(!ab->process)
    {
        g_print("process error\n");
        return ;
    }

    ab->cancellable = g_cancellable_new ();

    g_subprocess_wait_check_async(ab->process,ab->cancellable,mtk_bash_on_finish, ab);

    ab->stream = g_data_input_stream_new( g_subprocess_get_stdout_pipe(ab->process) );

    if(!ab->stream)
    {
        g_print("process stream error\n");
        return ;
    }

    mtk_bash_queue_read(ab);
}


///////////////////////////////////////////////////


class Git 
{
public:

    int exit_status = 0;

    typedef std::pair<std::string,std::string> GitResult;

    Git(MtkFile* f)
        : file(f)
    {
        g_object_ref(f);

        cd = file->file_name;
        target = file->is_directory ? "." : file->file_name;
        if ( !file->is_directory)
        {
            gchar* parent = mtk_file_get_parent(file);
            cd = parent;
            g_free(parent);
        }
    }

    std::string default_branch()
    {
        shell( make_cmd( " git symbolic-ref refs/remotes/origin/HEAD | sed 's@^refs/remotes/origin/@@'" ) );
        std::string txt = read();
        return txt;
    }

    GitResult create_branch(const std::string& name)
    {
        std::ostringstream oss;
        oss <<  "git checkout -b " << name << " && git push -u origin HEAD " ;
        shell( make_cmd(oss.str().c_str()) );
        std::string txt = read();
        return GitResult{ "create new branch ", txt };
    }


    GitResult delete_branch(const std::string& name)
    {
        std::ostringstream oss;
        oss <<  "git branch -d"  << name ;
        shell( make_cmd(oss.str().c_str()) );
        std::string txt = read();
        return GitResult{ std::string("delete branch ") + name, txt };
    }

    GitResult select_branch(const std::string& name)
    {
        std::ostringstream oss;
        oss <<  "git checkout "  << name ;
        shell( make_cmd(oss.str().c_str()) );
        std::string txt = read();
        return GitResult{ std::string("selected branch ") + name, txt };
    }

    bool has_local_commits()
    {
        shell( make_cmd("git log \"origin/$(git branch | grep \"*\" | cut -d' ' -f2)..HEAD\" ") );
        std::string txt = read();
        return !txt.empty();
    }

    std::vector<std::string> split(const std::string& txt, const char seperator)
    {
        std::vector<std::string> result;
        size_t startpos = 0;
        size_t pos = txt.find(seperator,startpos);
        while(pos != std::string::npos)
        {
            result.push_back( txt.substr(startpos,pos-startpos) );
            startpos = pos + 1;
            pos = txt.find(seperator,startpos);
        }
        return result;
    }

    std::map<std::string,std::string> origin_status()
    {
        shell( make_cmd_target("git diff --name-status origin/$(git branch | grep '*' | cut -d' ' -f2) -- ") );
        std::string txt = read();

        std::map<std::string,std::string> result;

        std::vector<std::string> lines = split(txt,'\n');

        std::string gitroot = lines[0];
        for( int i = 1; i < lines.size(); i++)
        {
            std::string line = lines[i];
            size_t pos = line.find_first_of(" \t");
            std::string status = line.substr(0,pos);
            size_t pos2 = line.find_first_not_of(" \t",pos);
            std::string path = line.substr(pos2);

            path = gitroot + "/" + path;

            if ( path.substr(0,cd.size()) == cd )
            {
                path = path.substr(cd.size()+1);
            }
            result[path] = status;
        }
        for( auto it = result.begin(); it != result.end(); it++)
        {
            g_print("%s -> %s\n", (*it).first.c_str(), (*it).second.c_str() );
        }
        return result;
    }

    std::map<std::string,std::string> porcelain()
    {
        shell( make_cmd_target("git status --porcelain -uall --ignored ") );
        std::string txt = read();

        std::map<std::string,std::string> result;

        std::vector<std::string> lines = split(txt,'\n');

        std::string gitroot = lines[0];
        for( int i = 1; i < lines.size(); i++)
        {
            std::string line = lines[i];
            std::string status = line.substr(0,2);
            std::string path = line.substr(3);

            std::vector<std::string> v = split(path, ' ');
            if (!v.empty())
            {
                path = v[0];
            }

            path = gitroot + "/" + path;

            if( path.substr(0,cd.size()) == cd )
            {
                path = path.substr(cd.size()+1);
            }

            bool is_dir=false;
            size_t pos = path.find("/");
            if( pos != std::string::npos )
            {
                std::vector<std::string> items = split(path,'/');
                path = items[0];
                is_dir= true;
            }

            if( result.count(path) != 0)
            {
                if ( status != "!!" && status != "??")
                {
                    result[path] = status;
                }
            }
            else
            {
                if(!is_dir)
                {
                    result[path] = status;
                }
                else
                {
                    if ( status != "!!" )
                    {
                        result[path] = status;
                    }                    
                }
            }
        }
        for( auto it = result.begin(); it != result.end(); it++)
        {
            g_print("%s -> %s\n", (*it).first.c_str(), (*it).second.c_str() );
        }

        return result;
    }

    GitResult view_file()
    {
        if ( file->is_directory)
        {
            return bash( make_cmd("ls -lah") );
        }
        else
        {
            return bash( make_cmd_target("cat") );
        }
    }

    GitResult commit(const std::string& msg)
    {
        std::ostringstream oss;
        oss << "git commit -m'" << escape(msg.c_str()) << "' ";

        int r = shell( make_cmd( oss.str().c_str() ) );
        if(r == 0)
        {
            std::string txt = read();
            return make_result(txt);
        }
        else
        {
            return status();
        }
    }


    GitResult status()
    {
        return bash( make_cmd_target("git status") );
    }

    GitResult diff()
    {
        return bash( make_cmd_target("git diff") );
    }

    GitResult diff_cached()
    {
        return bash( make_cmd_target("git diff --cached") );
    }

    GitResult diff_origin()
    {
        return bash( make_cmd_target("git diff origin/$(git branch | grep '*' | cut -d' ' -f2) -- ") );
    }

    GitResult add()
    {
        return bash( make_cmd_target("git add") );
    }

    GitResult restore()
    {
        return bash( make_cmd_target("git restore") );
    }

    GitResult restore_staged()
    {
        return bash( make_cmd_target("git restore --staged ") );
    }

    GitResult restore_origin()
    {
        if (has_local_commits() == false)
        {
            return GitResult{ "no local commits","reset aborted"};
        }

        return bash( make_cmd("git reset HEAD~ ") );
    }

    GitResult pull()
    {
        return bash( make_cmd("GIT_ASKPASS=true git pull") );
    }

    GitResult push()
    {
        return bash( make_cmd("GIT_ASKPASS=true git push"));
    }

    

    ~Git()
    {
        dispose();
        g_object_ref(file);
    }

private:


    std::string make_cmd(const char* cmd)
    {
        //std::string cmd = escape(git_cmd);

        std::ostringstream oss;
        oss << "cd " << cd 
            << " && git rev-parse --show-toplevel 2>&1 ; " 
            << cmd;

        return oss.str();
    }

    std::string make_cmd_target(const char* cmd)
    {
//        std::string cmd = escape(git_cmd);

        std::ostringstream oss;
        oss << "cd " << cd 
            << " && git rev-parse --show-toplevel 2>&1 ; " 
            << cmd
            << " " << target;

        return oss.str();
    }

    GitResult bash(const std::string& cmd)
    {
        shell(cmd);

        std::string txt = read();
        return make_result(txt);
    }

    int shell(const std::string& cmd)
    {
        dispose();

        g_print("SHELLOUT: %s\n", cmd.c_str());

        process = g_subprocess_new(
            flags,
            &error,
            "/bin/bash",
            "-c",
            cmd.c_str(),
            NULL
        );

        if(!process)
        {
            g_print("process error\n");
            return -1;
        }

        g_subprocess_wait(process,NULL,&error);

        stream = g_subprocess_get_stdout_pipe(process);

        if(!stream)
        {
            g_print("process stream error\n");
            return -1;
        }

        exit_status = g_subprocess_get_exit_status(process);


        return exit_status;
    }

    std::string read()
    {
        std::ostringstream oss;
        gchar buffer[1024];

        gssize s = g_input_stream_read( stream, buffer, 1024, NULL, &error);

        while(s>0)
        {
            oss.write(buffer,s);
            s = g_input_stream_read( stream, buffer, 1024, NULL, &error);
        }
        return oss.str();
    }

    GitResult make_result(const std::string& txt)
    {
        size_t pos = txt.find("\n");
        if ( pos == std::string::npos )
        {
            return GitResult{txt,""};
        }

        std::string headline = txt.substr(0,pos);
        std::string body = txt.substr(pos+1);

        return GitResult{headline,body};
    }

    std::string escape( const gchar* cmd )
    {
        std::ostringstream oss;
        const gchar* p= cmd;
        while(*p)
        {
            if( *p == '\'')
            {
                oss << "\\'";
            }
            else
            {
                oss << *p;
            }
            p++;
        }        
        return oss.str();
    }

    void dispose()
    {
        exit_status = 0;
        if(stream)
        {
            g_input_stream_close(stream,NULL,NULL);
            stream = 0;
        }
        if(process)
        {
            g_object_unref(process);
            process = 0;
        }
    }

    std::string cd;
    std::string target;

    MtkFile* file = 0;
    GError* error = 0;
    GSubprocess* process = 0;
    GInputStream* stream = 0;
    GSubprocessFlags flags = (GSubprocessFlags)(G_SUBPROCESS_FLAGS_STDOUT_PIPE|G_SUBPROCESS_FLAGS_STDERR_MERGE);
};

gchar* mtk_bash( const gchar* cmd, gint* result)
{
    if(result) *result = -1;

    GError* error=0;

    GSubprocess* process = g_subprocess_new(
        (GSubprocessFlags)(G_SUBPROCESS_FLAGS_STDOUT_PIPE|G_SUBPROCESS_FLAGS_STDERR_MERGE),
        &error,
        "/bin/bash",
        "-c",
        cmd,
        NULL
    );

    if(!process)
    {
        g_print("process error\n");
        return "";
    }

    g_subprocess_wait(process,NULL,&error);

    GInputStream* stream = g_subprocess_get_stdout_pipe(process);

    if(!stream)
    {
        g_print("process stream error\n");
        return "";
    }

    int exit_status = g_subprocess_get_exit_status(process);
    if(result) *result = exit_status;

    std::ostringstream oss;
    gchar buffer[1024];


    gssize s = g_input_stream_read( stream, buffer, 1024, NULL, &error);

    while(s>0)
    {
        oss.write(buffer,s);
        s = g_input_stream_read( stream, buffer, 1024, NULL, &error);
    }

    return g_strdup( oss.str().c_str() );

/*    MtkFile* file = mtk_filetree_get_selected_file(self);

    Git git(file);

    g_object_unref(file);

    Git::GitResult gr = git.status();

    g_print("GIT: %s \n", gr.first.c_str() );
    g_print("GIT: %s \n", gr.second.c_str() );

    return g_strdup(gr.second.c_str());
*/
}

//////////////////////////////////////////////////////////////////////

/**
 * SECTION: GFileTreeGitFile
 * @short_description: A Gtk GitFile Tree File GObject
 *
 * 
 */

G_DEFINE_TYPE (MtkGitFile, mtk_gitfile, MTK_FILE_TYPE   )

struct _GitStatus
{
    std::string color;
    std::string icon;
    std::string tooltip;
};

static GitStatus* mtk_gitfile_get_status_data(MtkGitFile* file);

static void mtk_gitfile_init(MtkGitFile *file)
{
    if ( !file)
    {
        g_print( PROG "mtk_gitfile_init: MtkGitFile is null \n" );
        return;
    }

    file->status = g_strdup("  ");
    file->gitdata = 0;
    /*
    file->root = 0;
    file->file_name = 0;
    file->is_place_holder = FALSE;
    file->is_directory = TRUE;
    file->is_empty = FALSE;
    file->is_hidden = FALSE;
*/
}



static GitStatus* mtk_gitfile_get_status_data_new(MtkGitFile* file)
{
    using color_map = std::pair<std::string,std::string>;
    static std::map<std::string,std::string> colors = {
        color_map{ "black" , "#000000" },
        color_map{ "green" , "#17901B" },
        color_map{ "ref"   , "#FF0000" },
        color_map{ "orange" , "#FF7D4B" },
        color_map{ "blue" , "#5A8DF3" },
        color_map{ "gray" , "#AAAAAA" }     
    };

    char X = file->status[0];
    char Y = file->status[1];

    gchar* stock = "gtk-file";
    MtkFile* f = (MtkFile*)file;
    if( f->is_directory )
    {
        stock = "gtk-open";
    }

    if ( X == 'O' && Y == 'O' )
    {
        return new GitStatus{ colors["green"], "gtk-apply", "File is committed waiting for push."  };
    }

    if ( X == '?' && Y == '?' )
    {
        return new GitStatus{ colors["black"], "gtk-dialog-question", "File is unknown to git."  };
    }

    if ( X == '!' && Y == '!' )
    {
        return new GitStatus{ colors["gray"], stock, "File is ingored by git."  };
    }

    if ( ( X == 'D' && Y == 'D' ) || ( X == 'U' && Y == 'D' ) )
    {
        return new GitStatus{ colors["red"], "gtk-dialog-error", "File has merge conflict."  };
    }

    if ( ( X == 'U' && Y == 'A' ) || ( X == 'A' && Y == 'A' ) )
    {
        return new GitStatus{ colors["red"], "gtk-dialog-error", "File has merge conflict."  };
    }

    if ( Y == 'U' )
    {
        return new GitStatus{ colors["red"], "gtk-dialog-error", "File has merge conflict."  };
    }

    if ( Y == 'M' || Y == 'D' || Y == 'R' || Y == 'C' )
    {
        return new GitStatus{ colors["orange"], "gtk-edit", "File is locally modified but not added for commit yet."  };
    }

    if ( Y == 'A' )
    {
        return new GitStatus{ colors["blue"], "gtk-dialog-info", "File is added."  };
    }

    if ( X == 'M' || X == 'D' || X == 'R' || X == 'C' )
    {
        return new GitStatus{ colors["green"], "gtk-go-up", "File is locally modified and added to be committed."  };
    }

    if ( Y == ' ' )
    {
        return new GitStatus{ colors["black"], stock, "No changes in git."  };
    }

    return new GitStatus{ colors["black"], stock, "Uptodate in git."  };    
}


static GitStatus* mtk_gitfile_get_status_data(MtkGitFile* file)
{
    if(!file->gitdata)
    {
        file->gitdata = mtk_gitfile_get_status_data_new(file);
    }

    return file->gitdata;
}
 
static void mtk_gitfile_finalize(GObject *object)
{
    MtkGitFile* file = (MtkGitFile*)object;

    G_OBJECT_CLASS (mtk_gitfile_parent_class)->finalize (object);

    g_free( file->status );

    delete file->gitdata;
    // TODO . chain up? 
/*
    g_free(file->file_name);

    if(file->root)
    {
        gtk_tree_iter_free(file->root);
    }
*/
}

gchar* virtual_mtk_gitfile_get_tooltip(MtkFile* file)
{
    MtkGitFile* f = (MtkGitFile*)file;
    GitStatus* gs = mtk_gitfile_get_status_data( f);

    //return g_strdup(gs.tooltip.c_str());
    return (gchar*)(gs->tooltip.c_str());
}

GList* virtual_mtk_gitfile_get_children(MtkFile* file, GtkTreeIter* iter)
{
    GList* glist = 0;

    if (!file->is_directory)
    {
        return glist;
    }

    std::vector<std::string> children = listdir(file->file_name);
    std::sort(children.begin(), children.end());

    std::vector<MtkGitFile*> dirs;
    std::vector<MtkGitFile*> files;
    
    Git git(file);
    std::map<std::string,std::string> git_paths = git.porcelain();
    std::map<std::string,std::string> origin = git.origin_status();


    for( auto& child : children)
    {

        std::string status = "";
        if( git_paths.count(child) > 0)
        {
            status = git_paths[child];
        }
        else if ( origin.count(child) > 0 )
        {
            status = "OO";
        }

        std::ostringstream oss;
        oss << file->file_name << "/" << child;

        std::string target = oss.str();

        MtkGitFile* gf = mtk_gitfile_new( target.c_str() );
        gf->status = g_strdup(status.c_str());

        MtkFile* f = (MtkFile*)gf;
        f->root = gtk_tree_iter_copy(iter);
        if ( f->is_directory )
        {
            dirs.push_back(gf);
        }
        else
        {
            files.push_back(gf);
        }
    }

    for( auto d : dirs )
    {
        glist = g_list_append(glist,d);
    }

    for( auto f : files )
    {
        glist = g_list_append(glist,f);
    }

    return glist;
}

void virtual_mtk_gitfile_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    )
{
    gchar* bn = mtk_file_get_basename(self);
    gchar* c = g_markup_escape_text(bn,-1);

    std::ostringstream oss;
    if ( self->is_hidden)
    {
        oss << "<i>" << c << "</i>";
    }
    else
    {
        oss << c;
    }
    g_free(c);
    
    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );
    g_value_set_string( &gv, oss.str().c_str() );
    g_object_set_property( (GObject*)cell, "markup", &gv);

    GitStatus* gs = mtk_gitfile_get_status_data( (MtkGitFile*) self);

    GValue gvc = G_VALUE_INIT;
    g_value_init( &gvc, G_TYPE_STRING );
    g_value_set_string( &gvc,gs->color.c_str() );
    g_object_set_property( (GObject*)cell, "foreground", &gvc);

    g_free(bn);
}

void virtual_mtk_gitfile_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    )
{
    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );

    if ( self->is_empty )
    {
        g_value_set_static_string( &gv, NULL );
        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
    else
    {
        GitStatus* gs = mtk_gitfile_get_status_data( (MtkGitFile*) self);

        g_value_set_static_string( &gv, gs->icon.c_str() );
        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
}


static void mtk_gitfile_class_init(MtkGitFileClass *klass)
{
    if ( !klass)
    {
        g_print( PROG "mtk_gitfile_class_init: MtkFileClass is null \n" );
        return;
    }

    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize     = mtk_gitfile_finalize;

    MtkFileClass* parentClass = (MtkFileClass*)klass;

    parentClass->get_tooltip = &virtual_mtk_gitfile_get_tooltip;
    parentClass->get_children = &virtual_mtk_gitfile_get_children;
    parentClass->tree_cell_render_file = &virtual_mtk_gitfile_tree_cell_render_file;
    parentClass->tree_cell_render_pix = &virtual_mtk_gitfile_tree_cell_render_pix;
}

MtkGitFile*	mtk_gitfile_new( const gchar* fn)
{
    MtkGitFile* file;

    file = (MtkGitFile*)g_object_new (MTK_GITFILE_TYPE, NULL);

    MtkFile* f = (MtkFile*)file;

    f->file_name = g_strdup(fn);

    struct stat st;
    stat(fn,&st);

    if( S_ISDIR(st.st_mode) )
    {
        f->is_directory = TRUE;
    }
    else
    {
        f->is_directory = FALSE;
    }

    char* tmp = g_strdup(fn);
    char* bn = basename(tmp);

    if( bn[0] == '.') {
        f->is_hidden = TRUE;
    }

    g_free(tmp);
    return file;    
}

std::vector<std::string> split(const std::string& txt, const char seperator)
{
    std::vector<std::string> result;
    size_t startpos = 0;
    size_t pos = txt.find(seperator,startpos);
    while(pos != std::string::npos)
    {
        result.push_back( txt.substr(startpos,pos-startpos) );
        startpos = pos + 1;
        pos = txt.find(seperator,startpos);
    }
    return result;
}

std::string make_cmd(const std::string& cd, const std::string& cmd)
{
    std::ostringstream oss;
    oss << "cd " << cd 
        << " && git rev-parse --show-toplevel 2>&1 ; " 
        << cmd;

    return oss.str();
}

std::string make_cmd_target(const std::string& cd, const std::string& cmd, const std::string& target)
{
    std::ostringstream oss;
    oss << "cd " << cd 
        << " && git rev-parse --show-toplevel 2>&1 ; " 
        << cmd
        << " " << target;

    return oss.str();
}

gint git_get_branches(gint exit_code, const std::string& out, gchar** status, gchar** contents)
{
    std::vector<std::string> lines = split(out,'\n');
    std::ostringstream oss;
    for( auto& line : lines)
    {
        if(!line.empty())
        {
            if(line[0] == '*')
            {
                if(status)
                {
                    *status = g_strdup(line.c_str());
                }
            }
            else
            {
                oss << line << "\n";
            }
        }
    }
    if(contents)
    {
        *contents = g_strdup(oss.str().c_str());
    }
    return exit_code;
}


gint mtk_git_cmd(MtkFile* file, MtkGitCmd cmd, gchar** status, gchar** contents )
{
    static std::map<MtkGitCmd,std::string> cmds_without_target = {
        { MTK_GIT_DEFAULT_BRANCH, "git symbolic-ref refs/remotes/origin/HEAD | sed 's@^refs/remotes/origin/@@'" },
        { MTK_GIT_BRANCHES, "git branch --no-color"},
        { MTK_GIT_HAS_LOCAL_COMMITS, "git log \"origin/$(git branch | grep \"*\" | cut -d' ' -f2)..HEAD\" " }
    };

    static std::map<MtkGitCmd,std::string> cmds_with_target = {
        { MTK_GIT_STATUS, "git status" },
        { MTK_GIT_ADD, "git add" },
        { MTK_GIT_DIFF, "git diff" },
        { MTK_GIT_DIFF_CACHED, "git diff --cached" },
        { MTK_GIT_DIFF_ORIGIN, "git diff origin/$(git branch | grep '*' | cut -d' ' -f2) -- " },
        { MTK_GIT_VIEW_FILE, "cat "},
        { MTK_GIT_ORIGIN_STATUS, "git diff --name-status origin/$(git branch | grep '*' | cut -d' ' -f2) -- " },
        { MTK_GIT_PORCELAIN, "git status --porcelain -uall --ignored " }
    };

    std::string cd = file->file_name;
    std::string target = file->is_directory ? "." : file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    gint exit_code = 0;
    std::string c;

    if (cmds_without_target.count(cmd)>0)
    {
        c = cmds_without_target[cmd];
        c = make_cmd(cd,c);
    }
    if (cmds_with_target.count(cmd)>0)
    {
        c = cmds_with_target[cmd];
        c = make_cmd_target(cd,c,target);
    }
    if(c.empty())
    {
        if(status) *status = g_strdup("synchronous git command not found");
        if(contents) *contents = 0;
        return -1;
    }

    gchar* tmp = mtk_bash(c.c_str(),&exit_code);
    std::string content = tmp;
    g_free(tmp);

    switch( cmd )
    {
        case MTK_GIT_BRANCHES: {
            return git_get_branches(exit_code,content,status,contents);
        }
    }


    size_t pos = content.find("\n");
    if ( pos == std::string::npos )
    {
        if(status)
        {
            *status = g_strdup(content.c_str());
        }
        return exit_code;
    }
    if(status)
    {
        *status = g_strdup(content.substr(0,pos).c_str());
    }
    if(contents)
    {
        *contents = g_strdup(content.substr(pos+1).c_str());
    }
    return exit_code;
}

gboolean mtk_git_has_local_commits(MtkFile* file)
{
    gchar* status = 0;
    gchar* content = 0;

    gint exit_code = mtk_git_cmd(file, MTK_GIT_HAS_LOCAL_COMMITS, &status, &content);
    if(!content || strlen(content) == 0 )
    {
        return false;
    }
    g_free(status);
    g_free(content);
    return true;
}

std::map<std::string,std::string> git_origin_status(MtkFile* file)
{
    gchar* status = 0;
    gchar* content = 0;

    gint exit_code = mtk_git_cmd(file, MTK_GIT_ORIGIN_STATUS, &status, &content);

    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    std::map<std::string,std::string> result;

    std::string gitroot = status;
    std::vector<std::string> lines = split(content,'\n');
    for( int i = 0; i < lines.size(); i++)
    {
        std::string line = lines[i];
        std::string status = line.substr(0,1);
        std::string path = line.substr(2);

        path = gitroot + "/" + path;

        if ( path.substr(0,cd.size()) == cd )
        {
            path = path.substr(cd.size()+1);
        }
        result[path] = status;
    }
    g_free(status);
    g_free(content);
    return result;
}

std::map<std::string,std::string> porcelain(MtkFile* file)
{
    gchar* status = 0;
    gchar* content = 0;

    gint exit_code = mtk_git_cmd(file, MTK_GIT_ORIGIN_STATUS, &status, &content);

    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    std::map<std::string,std::string> result;

    std::vector<std::string> lines = split(content,'\n');

    std::string gitroot = status;
    for( int i = 0; i < lines.size(); i++)
    {
        std::string line = lines[i];
        std::string status = line.substr(0,2);
        std::string path = line.substr(3);

        std::vector<std::string> v = split(path, ' ');
        if (!v.empty())
        {
            path = v[0];
        }

        path = gitroot + "/" + path;

        if( path.substr(0,cd.size()) == cd )
        {
            path = path.substr(cd.size()+1);
        }

        bool is_dir=false;
        size_t pos = path.find("/");
        if( pos != std::string::npos )
        {
            std::vector<std::string> items = split(path,'/');
            path = items[0];
            is_dir=true;
        }

        if( result.count(path) != 0)
        {
            if ( status != "!!" && status != "??")
            {
                result[path] = status;
            }
        }
        else
        {
            if(!is_dir)
            {
                result[path] = status;
            }
            else
            {
                if ( status != "!!" )
                {
                    result[path] = status;
                }
            }
        }
    }
    g_free(status);
    g_free(content);
    return result;
}

gboolean mtk_git_switch_branch(MtkFile* file, const gchar* branch)
{
    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    gint exit_code = 0;
    std::string cmd = std::string("git checkout ");
    cmd += branch;
    std::string c = make_cmd(cd,cmd.c_str());

    gchar* tmp = mtk_bash(c.c_str(),&exit_code);
    std::string content = tmp;
    g_free(tmp);

    return exit_code == 0;
}


gboolean mtk_git_delete_branch(MtkFile* file, const gchar* branch)
{
    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    gint exit_code = 0;
    std::string cmd = std::string("git branch -d  ");
    cmd += branch;
    std::string c = make_cmd(cd,cmd.c_str());

    gchar* tmp = mtk_bash(c.c_str(),&exit_code);
    std::string content = tmp;
    g_free(tmp);

    return exit_code == 0;
}

gint mtk_git_commit(MtkFile* file, const gchar* msg, char** status,  char** contents )
{
    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    gint exit_code = 0;
    std::string cmd = std::string("git commit -m'");
    cmd += escape_shell(msg);
    cmd += "'";
    std::string c = make_cmd(cd,cmd.c_str());

    gchar* tmp = mtk_bash(c.c_str(),&exit_code);
    std::string content = tmp;
    g_free(tmp);

    size_t pos = content.find("\n");
    if ( pos == std::string::npos )
    {
        if(status)
        {
            *status = g_strdup(content.c_str());
        }
        return exit_code;
    }
    if(status)
    {
        *status = g_strdup(content.substr(0,pos).c_str());
    }
    if(contents)
    {
        *contents = g_strdup(content.substr(pos+1).c_str());
    }
    return exit_code;

}


struct AsyncGitClosure
{
    MtkAsyncGitCallbackFunc callback = 0;
    gpointer user_data = 0;
    MtkGitCmd cmd;
};


void git_get_branches_async(AsyncGitClosure* agc, gint exit_code, const std::string& out )
{
    std::vector<std::string> lines = split(out,'\n');
    std::ostringstream oss;
    std::string status;
    for( auto& line : lines)
    {
        if(!line.empty())
        {
            if(line[0] == '*')
            {
                status = line.c_str();
            }
            else
            {
                oss << line << "\n";
            }
        }
    }

    (agc->callback)(exit_code,status.c_str(), oss.str().c_str(), agc->user_data);

    delete agc;
}

void git_on_async_cmd( int exit_code,const gchar* out, gpointer user_data)
{
    AsyncGitClosure* agc = (AsyncGitClosure*) user_data;

    std::string content = out;

    switch( agc->cmd )
    {
        case MTK_GIT_BRANCHES: {
            return git_get_branches_async(agc,exit_code,content);
        }
    }


    size_t pos = content.find("\n");
    if ( pos == std::string::npos )
    {
        (agc->callback)(exit_code, content.c_str(), "", agc->user_data);
    }
    else
    {
        (agc->callback)(exit_code, content.substr(0,pos).c_str(), content.substr(pos+1).c_str(), agc->user_data);
    }

    delete agc;
}


void mtk_git_cmd_async(MtkFile* file, MtkGitCmd cmd, MtkAsyncGitCallbackFunc callback, gpointer user_data )
{
    static std::map<MtkGitCmd,std::string> cmds_without_target = {
        { MTK_GIT_PULL, "GIT_ASKPASS=true git pull" },
        { MTK_GIT_PUSH, "GIT_ASKPASS=true git push" },
        { MTK_GIT_DEFAULT_BRANCH, "git symbolic-ref refs/remotes/origin/HEAD | sed 's@^refs/remotes/origin/@@'" },
        { MTK_GIT_BRANCHES, "git branch --no-color"}        
    };

    static std::map<MtkGitCmd,std::string> cmds_with_target = {
        { MTK_GIT_RESTORE, "git restore" },
        { MTK_GIT_RESTORE_STAGED, "git restore --staged" },
        { MTK_GIT_RESTORE_ORIGIN, "git reset HEAD~" },
        { MTK_GIT_STATUS, "git status" },
        { MTK_GIT_ADD, "git add" },
        { MTK_GIT_DIFF, "git diff" },
        { MTK_GIT_DIFF_CACHED, "git diff --cached" },
        { MTK_GIT_DIFF_ORIGIN, "git diff origin/$(git branch | grep '*' | cut -d' ' -f2) -- " },
        { MTK_GIT_VIEW_FILE, "cat "}
    };

    std::string cd = file->file_name;
    std::string target = file->is_directory ? "." : file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    std::string c;

    if (cmds_without_target.count(cmd)>0)
    {
        c = cmds_without_target[cmd];
        c = make_cmd(cd,c);
    }
    if (cmds_with_target.count(cmd)>0)
    {
        c = cmds_with_target[cmd];
        c = make_cmd_target(cd,c,target);
    }
    if(c.empty())
    {
        (callback)(-1,"asynchronous git command not found","",user_data);
        return;
    }

    AsyncGitClosure* agc = new AsyncGitClosure{ callback, user_data, cmd };

    mtk_bash_async(c.c_str(),git_on_async_cmd,agc);
}




// todo: specials: 
//  { GIT_BRANCHES, "git branch --no-color" },
//  { GIT_SWITCH_BRANCH, "git branch --no-color" },
// GIT_DELETE_BRANCH
//        { GIT_HAS_LOCAL_COMMITS, "git log \"origin/$(git branch | grep \"*\" | cut -d' ' -f2)..HEAD\" "},
// GIT_ORIGIN_STATUS, GIT_PORCELAIN, 