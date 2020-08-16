#ifndef __MO_GFILETREE_H__
#define __MO_GFILETREE_H__

#include <gtk/gtk.h>


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _GfiletreeFile		GfiletreeFile;
typedef struct _GfiletreeFileClass	GfiletreeFileClass;

/**
 * GfiletreeFile: 
 */
struct _GfiletreeFile {
/*< public >*/
    GObject parent;
    GtkTreeIter* root;
    gchar* file_name;
    gboolean is_place_holder;
    gboolean is_directory;
    gboolean is_empty;
    gboolean is_hidden;
};

/**
 * GfiletreeFileClass: 
 */

struct _GfiletreeFileClass {
/*< public >*/
    GObjectClass parent;

    gchar* (*get_tooltip)(GfiletreeFile* file);
    GList* (*get_children)(GfiletreeFile* file, GtkTreeIter* iter);

    void (*tree_cell_render_file)(
            GfiletreeFile* self,
            GtkTreeViewColumn *tree_column,
            GtkCellRenderer *cell,
            GtkTreeModel *tree_model,
            GtkTreeIter *iter,
            gpointer data    
    );

    void (*tree_cell_render_pix)(
            GfiletreeFile* self,
            GtkTreeViewColumn *tree_column,
            GtkCellRenderer *cell,
            GtkTreeModel *tree_model,
            GtkTreeIter *iter,
            gpointer data    
    );

    gpointer padding[12];
};

GType		gfiletree_file_get_type	() G_GNUC_CONST;


/**
 * gfiletree_file_new: 
 * @self: A #GfiletreeFile
 * @fn : A #gchar*
 * Returns: (transfer full): a #GfiletreeFile
 */
GfiletreeFile*	gfiletree_file_new( const gchar* fn);

/**
 * gfiletree_file_set_path: 
 * @self: A #GfiletreeFile
 * @fn : A #gchar*
 */
void gfiletree_file_set_path(GfiletreeFile*,  gchar* fn);

/**
 * gfiletree_file_get_path: 
 * @self: A #GfiletreeFile
 *
 * Returns: (transfer none): a #gchar*
 */
gchar* gfiletree_file_get_path(GfiletreeFile*);

/**
 * gfiletree_file_get_parent: 
 * @self: A #GfiletreeFile
 *
 * Returns: (transfer full): a #gchar*
 */
gchar* gfiletree_file_get_parent(GfiletreeFile*);

/**
 * gfiletree_file_get_basename: 
 * @self: A #GfiletreeFile
 *
 * Returns: (transfer full): a #gchar*
 */
gchar* gfiletree_file_get_basename(GfiletreeFile*);

/**
 * gfiletree_file_get_tooltip: 
 * @self: A #GfiletreeFile
 *
 * Returns: (transfer none): a #gchar*
 */
gchar* gfiletree_file_get_tooltip(GfiletreeFile*);

/**
 * gfiletree_file_get_children: 
 * @self: A #GfiletreeFile
 * @iter: A #GtkTreeIter
 *
 * Returns: (element-type GfiletreeFile) (transfer full): List of #GfiletreeFile
 */
GList* gfiletree_file_get_children(GfiletreeFile* self, GtkTreeIter* iter);

/**
 * gfiletree_file_tree_cell_render_file: 
 * @self: A #GtkTreeViewColumn
 * @tree_column: A #GtkTreeIter
 * @cell: A #GtkCellRenderer
 * @tree_model: A #GtkTreeModel
 * @iter: A #GtkTreeIter
 * @data: A #gpointer
 */
void gfiletree_file_tree_cell_render_file(
        GfiletreeFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);


/**
 * gfiletree_file_tree_cell_render_pix: 
 * @self: A #GtkTreeViewColumn
 * @tree_column: A #GtkTreeIter
 * @cell: A #GtkCellRenderer
 * @tree_model: A #GtkTreeModel
 * @iter: A #GtkTreeIter
 * @data: A #gpointer
 */
void gfiletree_file_tree_cell_render_pix(
        GfiletreeFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);


/////////////////////////////////////////////////////////////////////////////

typedef struct _GfiletreeFiletree		GfiletreeFiletree;
typedef struct _GfiletreeFiletreeClass	GfiletreeFiletreeClass;

/**
 * GfiletreeFiletree: 
 */

struct _GfiletreeFiletree {
/*< public >*/
    GtkTreeView parent;
    GfiletreeFile* root;
    GtkTreeStore* treeModel;
    GfiletreeFile* place_holder;
    GfiletreeFile* empty_dir;
    gboolean show_hidden;
    gchar* filter;
    gchar* cursel;

};

/**
 * GfiletreeFiletreeClass: 
 */

struct _GfiletreeFiletreeClass {
/*< public >*/
    GtkTreeViewClass parent;
    gchar* dir;
};

GType		gfiletree_filetree_get_type	() G_GNUC_CONST;

/**
 * gfiletree_filetree_new: 
 * Returns: (transfer full): a #GfiletreeFiletree
 */
GfiletreeFiletree*	gfiletree_filetree_new		();

/**
 * gfiletree_filetree_add_root: 
 * @self: A #GfiletreeFiletree
 * @file: A #GfiletreeFile
 * @self: A #GfiletreeFile
 * @self: A #GfiletreeFile
 */
void gfiletree_filetree_add_root(GfiletreeFiletree *self, GfiletreeFile* file, bool show_hidden, const gchar* filter);

/**
 * gfiletree_filetree_get_selected_file: 
 * @self: A #GfiletreeFiletree
 */
void gfiletree_filetree_clear(GfiletreeFiletree *self);

/**
 * gfiletree_filetree_get_selected_file: 
 * @self: A #GfiletreeFiletree
 * Returns: (transfer full): a #GfiletreeFile
 */
GfiletreeFile* gfiletree_filetree_get_selected_file(GfiletreeFiletree *self);

/**
 * gfiletree_filetree_file_at_pos: 
 * @self: A #GfiletreeFiletree
 * @x: A #gint
 * @y: A #gint
 *
 * Returns: (transfer full): a #GfiletreeFile
 */
GfiletreeFile* gfiletree_filetree_file_at_pos(GfiletreeFiletree *self, gint x, gint y);


/**
 * gfiletree_filetree_bash: 
 * @self: A #GfiletreeFiletree
 * @cmd: A #gchar*
 *
 * Returns: (transfer full): a #gchar*
 */
const gchar* gfiletree_filetree_bash(GfiletreeFiletree *self, gchar* cmd);


//////////////////////////////////////////////////////////////////////////



typedef struct _GfiletreeGitFile		GfiletreeGitFile;
typedef struct _GfiletreeGitFileClass	GfiletreeGitFileClass;

struct GitStatus;

/**
 * GfiletreeGitFile: 
 */
struct _GfiletreeGitFile {
/*< public >*/
    GfiletreeFile parent;
    gchar* status;
    GitStatus* gitdata;    
};

/**
 * GfiletreeGitFileClass: 
 */

struct _GfiletreeGitFileClass {
/*< public >*/
    GfiletreeFileClass parent;

};

GType		gfiletree_gitfile_get_type	() G_GNUC_CONST;


/**
 * gfiletree_gitfile_new: 
 * @self: A #GfiletreeGitFile
 * @fn : A #gchar*
 * Returns: (transfer full): a #GfiletreeGitFile
 */
GfiletreeGitFile*	gfiletree_gitfile_new( const gchar* fn);

#ifdef __cplusplus
}


#endif

#define GFILETREE_FILE_TYPE		\
    (gfiletree_file_get_type())
#define GFILETREE_FILE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), GFILETREE_FILE_TYPE, GfiletreeFile))
#define GFILETREE_FILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), GFILETREE_FILE_TYPE, GfiletreeFileClass))
#define GFILETREE_IS_FILETREE_FILE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GFILETREE_FILE_TYPE))
#define GFILETREE_IS_FILETREE_FILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  GFILETREE_FILE_TYPE))
#define GFILETREE_FILE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), GFILETREE_FILE_TYPE, GfiletreeFileClass))


#define GFILETREE_TYPE		\
    (gfiletree_filetree_get_type())
#define GFILETREE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), GFILETREE_TYPE, GfiletreeFiletree))
#define GFILETREE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), GFILETREE_TYPE, GfiletreeFiletreeClass))
#define GFILETREE_IS_FILETREE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GFILETREE_TYPE))
#define GFILETREE_IS_FILETREE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  GFILETREE_TYPE))
#define GFILETREE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), GFILETREE_TYPE, GfiletreeFiletreeClass))


#define GFILETREE_GITFILE_TYPE		\
    (gfiletree_gitfile_get_type())
#define GFILETREE_GITFILE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), GFILETREE_GITFILE_TYPE, GfiletreeGitFile))
#define GFILETREE_GITFILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), GFILETREE_GITFILE_TYPE, GfiletreeGitFileClass))
#define GFILETREE_IS_FILETREE_GITFILE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GFILETREE_GITFILE_TYPE))
#define GFILETREE_IS_FILETREE_GITFILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  GFILETREE_GITFILE_TYPE))
#define GFILETREE_GITFILE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), GFILETREE_GITFILE_TYPE, GfiletreeGitFileClass))


#endif /* __MO_WEBKIT_H__ */
