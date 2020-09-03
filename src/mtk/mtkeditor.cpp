#include "glue/common.h"
#include "mtk/mtkeditor.h"

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

    gprops<MtkEditorClass> props{
        gprop( &MtkEditor::lang, g_param_spec_string ( "language", "Language", "syntax highlight language", "", G_PARAM_READWRITE) )
    };
    props.install(klass);

    // GObject signals
    Signals signals(klass);
    signals.install("changed" ); 
}

static void on_changed(GtkTextBuffer* buff, gpointer user_data)
{
    MtkEditor* self = (MtkEditor*)user_data;

    g_signal_emit_by_name(self,"changed");
}


static void mtk_editor_init(MtkEditor* self)
{
    if ( !self)
    {
        g_print( PROG "mtk_editor_init: MtkEditor is null \n" );
        return;
    }

    self->path = 0;
    self->lang = g_strdup(".*");

    self->source = gtk_source_file_new();
    self->lang_manager = gtk_source_language_manager_new();

    GtkSourceBuffer* buffer = (GtkSourceBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );

    g_signal_connect( G_OBJECT (buffer), "changed", G_CALLBACK(on_changed), self);
}

MtkEditor* mtk_editor_new()
{
    MtkEditor* edit;

    edit = (MtkEditor*)g_object_new (MTK_EDITOR_TYPE, NULL);
    return edit;
}

static void mtk_editor_finalize(GObject *object)
{
    MtkEditor* self = (MtkEditor*)object;

    g_object_unref(self->source);
    g_object_unref(self->lang_manager);
    g_free(self->path);
    g_free(self->lang);
}

gboolean mtk_editor_is_modified(MtkEditor* self)
{
    GtkSourceBuffer* buffer = (GtkSourceBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );
    return gtk_text_buffer_get_modified( (GtkTextBuffer*)buffer);
}

void on_loaded(GObject* source_object, GAsyncResult *res,gpointer user_data)
{
    MtkEditor* self = (MtkEditor*) user_data;

    GError* error = 0;
    gboolean success = gtk_source_file_loader_load_finish( (GtkSourceFileLoader*)source_object,res,&error);
    if(success)
    {
        
    }
    g_object_unref(source_object);
}

void mtk_edit_load(MtkEditor* self, const gchar* path)
{
    self->path = g_strdup(path);
    GtkSourceBuffer* buffer = (GtkSourceBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );

    GFile* file = g_file_new_for_path(path);
    gtk_source_file_set_location(self->source,file);

    if(self->lang)
    {
        GtkSourceLanguage* lang = gtk_source_language_manager_get_language(self->lang_manager,self->lang);
        if(lang)
        {
            gtk_source_buffer_set_language(buffer,lang);
        }
    }

    GtkSourceFileLoader* loader = gtk_source_file_loader_new(buffer,self->source);
    gtk_source_file_loader_load_async( loader, 0, NULL, NULL, NULL, NULL, on_loaded, self);

}

void on_saved(GObject* source_object, GAsyncResult *res,gpointer user_data)
{
    MtkEditor* self = (MtkEditor*) user_data;

    GError* error = 0;
    gboolean success = gtk_source_file_saver_save_finish( (GtkSourceFileSaver*)source_object,res,&error);
    if(success)
    {
        
    }
    g_object_unref(source_object);
}

void mtk_editor_save(MtkEditor* self)
{
    GtkSourceBuffer* buffer = (GtkSourceBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );

    GtkSourceFileSaver* saver = gtk_source_file_saver_new(buffer,self->source);
    gtk_source_file_saver_save_async( saver, 0, NULL, NULL, NULL, NULL, on_saved, self );

    gtk_text_buffer_set_modified( (GtkTextBuffer*)buffer, FALSE );
}

void mtk_editor_save_as(MtkEditor* self, const gchar* path)
{
    GtkSourceBuffer* buffer = (GtkSourceBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );

    GFile* target = g_file_new_for_path(path);

    GtkSourceFileSaver* saver = gtk_source_file_saver_new_with_target(buffer,self->source, target);
    gtk_source_file_saver_save_async( saver, 0, NULL, NULL, NULL, NULL, on_saved, self );

    g_object_unref(target);
}

gchar* mtk_editor_get_text(MtkEditor* self)
{
    GtkTextBuffer* buffer = (GtkTextBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );

    GtkTextIter start;
    gtk_text_buffer_get_start_iter(buffer,&start);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer,&end);

    return gtk_text_buffer_get_text( buffer, &start, &end, FALSE);
}

void mtk_editor_set_text(MtkEditor* self, const gchar* txt)
{
    GtkTextBuffer* buffer = (GtkTextBuffer*) gtk_text_view_get_buffer( (GtkTextView*)self );

    gtk_text_buffer_set_text( buffer, txt, -1 );
}
