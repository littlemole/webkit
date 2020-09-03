#include "common.h"
#include "mtkfiletree.h"

#define PROG "[MktFile] "


/**
 * SECTION: MtkFile
 * @short_description: A Gtk File Tree File GObject
 *
 * 
 */

G_DEFINE_TYPE (MtkFile, mtk_file, G_TYPE_OBJECT   )

//////////////////////////////////////////////////////////////////////////
// forwards

static void mtk_file_finalize(GObject *object);
static gchar* virtual_mtk_file_get_tooltip(MtkFile* self);
static GList* virtual_mtk_file_get_children(MtkFile* self, GtkTreeIter* iter);
static void virtual_mtk_file_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    );
static void virtual_mtk_file_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    );

static void mtk_file_on_path_changed( GObject *gobject, GParamSpec *pspec, gpointer user_data);

//////////////////////////////////////////////////////////////////////////


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

    gprops<MtkFileClass> props{
        gprop( &MtkFile::file_name, g_param_spec_string( "path", "Path", "filesystem path", "/", G_PARAM_READWRITE))
    };
    props.install(klass);

}


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

    g_signal_connect( G_OBJECT ((GObject*)file), "notify::path", G_CALLBACK(mtk_file_on_path_changed), file);

}
 

static void mtk_file_finalize(GObject *object)
{
    MtkFile* file = (MtkFile*)object;

    g_free(file->file_name);

    if(file->root)
    {
        gtk_tree_iter_free(file->root);
        file->root = 0;
    }
}

//////////////////////////////////////////////////////////////////////////

MtkFile* mtk_file_new( const gchar* fn)
{
    MtkFile* file;

    file = (MtkFile*)g_object_new(MTK_FILE_TYPE, NULL);

//    GValue gv = G_VALUE_INIT;
//    g_value_init( &gv, G_TYPE_STRING );
//    g_value_set_string( &gv, fn );

    gval gv(fn);
    g_object_set_property( G_OBJECT(file),"path",&gv);
  
    return file;
}

//////////////////////////////////////////////////////////////////////////

static void mtk_file_on_path_changed( GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
    MtkFile* file = (MtkFile*)gobject;

    struct stat st;
    stat(file->file_name,&st);

    if( S_ISDIR(st.st_mode) )
    {
        file->is_directory = TRUE;
    }
    else
    {
        file->is_directory = FALSE;
    }

    char* tmp = g_strdup(file->file_name);
    char* bn = basename(tmp);

    if( bn[0] == '.') {
        file->is_hidden = TRUE;
    }

    g_free(tmp);    
}


//////////////////////////////////////////////////////////////////////////

static gchar* virtual_mtk_file_get_tooltip(MtkFile* self)
{
    return self->file_name;
}


static GList* virtual_mtk_file_get_children(MtkFile* self, GtkTreeIter* iter)
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


static void virtual_mtk_file_tree_cell_render_file(
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
  /*  
    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );
    g_value_set_string( &gv, oss.str().c_str() );
*/
    gval gv(oss.str() );
    g_object_set_property( (GObject*)cell, "markup", &gv);

    g_free(bn);
}



static void virtual_mtk_file_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    )
{
//    GValue gv = G_VALUE_INIT;
//    g_value_init( &gv, G_TYPE_STRING );

    if ( self->is_empty)
    {
//        g_value_set_static_string( &gv, NULL );

        gval gv( (const char*)NULL, TRUE);
        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
    else if( self->is_directory )
    {
//        g_value_set_static_string( &gv, "gtk-open" );

        gval gv( "gtk-open", TRUE);
        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
    else
    {
        gval gv( "gtk-file", TRUE);
//        g_value_set_static_string( &gv, "gtk-file" );

        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
}


//////////////////////////////////////////////////////////////////////////


// virtual launchers

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

//////////////////////////////////////////////////////////////////////////


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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


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


static void mtk_bash_on_finish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    g_print("mtk_filetree_bash_on_finish \n");

    GSubprocess* process = (GSubprocess*)source_object;

    GError* error=0;
    gboolean success = g_subprocess_wait_check_finish ( process, res, &error);

    AsyncBash* ab = (AsyncBash*) user_data;

    if(!success)
    {
        ab->status = -1;
    }
    else
    {
        ab->status = g_subprocess_get_exit_status(process);
    }
        
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

static void mtk_bash_on_data(GObject *source_object, GAsyncResult *res, gpointer user_data);

void mtk_bash_queue_read(AsyncBash *ab)
{
    g_data_input_stream_read_line_async( ab->stream, 0, ab->cancellable, mtk_bash_on_data, ab );
}

static void mtk_bash_on_data(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    GError* error = 0;
    gsize len = 0;
    char* line = g_data_input_stream_read_line_finish_utf8( (GDataInputStream*)source_object, res, &len, &error);

    AsyncBash* ab = (AsyncBash*) user_data;

    if(line && len>0)
    {
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

//////////////////////////////////////////////////////////////////////////


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
}

