#ifndef __MO_MTK_H__
#define __MO_MTK_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _MtkFile		MtkFile;
typedef struct _MtkFileClass	MtkFileClass;

/**
 * MtkFile: 
 */
struct _MtkFile {
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
 * MtkFileClass: 
 */

struct _MtkFileClass {
/*< public >*/
    GObjectClass parent;

    gchar* (*get_tooltip)(MtkFile* self);
    GList* (*get_children)(MtkFile* self, GtkTreeIter* iter);

    void (*tree_cell_render_file)(
            MtkFile* self,
            GtkTreeViewColumn *tree_column,
            GtkCellRenderer *cell,
            GtkTreeModel *tree_model,
            GtkTreeIter *iter,
            gpointer data    
    );

    void (*tree_cell_render_pix)(
            MtkFile* self,
            GtkTreeViewColumn *tree_column,
            GtkCellRenderer *cell,
            GtkTreeModel *tree_model,
            GtkTreeIter *iter,
            gpointer data    
    );

    gpointer padding[12];
};

GType		mtk_file_get_type	() G_GNUC_CONST;


/**
 * mtk_file_new: 
 * @fn : A #gchar*
 * Returns: (transfer full): a #MtkFile
 */
MtkFile*	mtk_file_new( const gchar* fn);

/**
 * mtk_file_set_path: 
 * @self: A #MtkFile
 * @fn : A #gchar*
 */
void mtk_file_set_path(MtkFile* self,  gchar* fn);

/**
 * mtk_file_get_path: 
 * @self: A #MtkFile
 *
 * Returns: (transfer none): a #gchar*
 */
gchar* mtk_file_get_path(MtkFile* self);

/**
 * mtk_file_get_parent: 
 * @self: A #MtkFile
 *
 * Returns: (transfer full): a #gchar*
 */
gchar* mtk_file_get_parent(MtkFile* self);

/**
 * mtk_file_get_basename: 
 * @self: A #MtkFile
 *
 * Returns: (transfer full): a #gchar*
 */
gchar* mtk_file_get_basename(MtkFile* self);

/**
 * mtk_file_get_tooltip: 
 * @self: A #MtkFile
 *
 * Returns: (transfer none): a #gchar*
 */
gchar* mtk_file_get_tooltip(MtkFile* self);

/**
 * mtk_file_get_children: 
 * @self: A #MtkFile
 * @iter: A #GtkTreeIter
 *
 * Returns: (element-type MtkFile) (transfer full): List of #MtkFile
 */
GList* mtk_file_get_children(MtkFile* self, GtkTreeIter* iter);

/**
 * mtk_file_tree_cell_render_file: 
 * @self: A #GtkTreeViewColumn
 * @tree_column: A #GtkTreeIter
 * @cell: A #GtkCellRenderer
 * @tree_model: A #GtkTreeModel
 * @iter: A #GtkTreeIter
 * @data: A #gpointer
 */
void mtk_file_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);


/**
 * mtk_file_tree_cell_render_pix: 
 * @self: A #GtkTreeViewColumn
 * @tree_column: A #GtkTreeIter
 * @cell: A #GtkCellRenderer
 * @tree_model: A #GtkTreeModel
 * @iter: A #GtkTreeIter
 * @data: A #gpointer
 */
void mtk_file_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);


/////////////////////////////////////////////////////////////////////////////

typedef struct _MtkFiletree		MtkFiletree;
typedef struct _MtkFiletreeClass	MtkFiletreeClass;

/**
 * MtkFiletree: 
 */

struct _MtkFiletree {
/*< public >*/
    GtkTreeView parent;
    MtkFile* root;
    GtkTreeStore* treeModel;
    MtkFile* place_holder;
    MtkFile* empty_dir;
    gboolean show_hidden;
    gchar* filter;
    gchar* cursel;
};

/**
 * MtkFiletreeClass: 
 */

struct _MtkFiletreeClass {
/*< public >*/
    GtkTreeViewClass parent;
    gchar* dir;
};

GType		mtk_filetree_get_type	() G_GNUC_CONST;

/**
 * mtk_filetree_new: 
 * Returns: (transfer full): a #MtkFiletree
 */
MtkFiletree*	mtk_filetree_new		();

/**
 * mtk_filetree_add_root: 
 * @self: A #MtkFiletree
 * @file: A #MtkFile
 * @show_hidden: A #gboolean
 * @filter: A #gchar*
 */
void mtk_filetree_add_root(MtkFiletree *self, MtkFile* file, bool show_hidden, const gchar* filter);

/**
 * mtk_filetree_clear: 
 * @self: A #MtkFiletree
 */
void mtk_filetree_clear(MtkFiletree *self);

/**
 * mtk_filetree_get_selected_file: 
 * @self: A #MtkFiletree
 * Returns: (transfer full): a #MtkFile
 */
MtkFile* mtk_filetree_get_selected_file(MtkFiletree *self);

/**
 * mtk_filetree_file_at_pos: 
 * @self: A #MtkFiletree
 * @x: A #gint
 * @y: A #gint
 *
 * Returns: (transfer full): a #MtkFile
 */
MtkFile* mtk_filetree_file_at_pos(MtkFiletree *self, gint x, gint y);


/**
 * mtk_bash: 
 * @cmd: A #gchar*
 * @status: (out caller-allocates) (optional) : A #gint
 * Returns: (transfer full): a #gchar*
 */
gchar* mtk_bash(const gchar* cmd, gint* status);

/**
 * MtkAsyncBashCallbackFunc: 
 * @status: A #gint
 * @out: A #gchar*
 * @user_data: A #gpointer
 *
 */

typedef void (*MtkAsyncBashCallbackFunc)(int status, const gchar* out, gpointer user_data);

#define MTK_ASYNC_BASH_CALLBACK(f) ((MtkAsyncBashCallbackFunc) (void (*)(int,gchar*,gpointer)) (f))

/**
 * mtk_bash_async: 
 * @cmd: A #gchar*
 * @callback: (closure user_data) (scope async): A #MtkAsyncBashCallbackFunc
 * @user_data: (closure): a #gpointer
 *
 */
void mtk_bash_async(const gchar* cmd, MtkAsyncBashCallbackFunc callback, gpointer user_data);

// @user_data: (closure callback): a #gpointer
// * Returns: (transfer full): a #gchar*
 


/**
 * MtkGitCmd: 
 * @MTK_GIT_STATUS: git status
 * @MTK_GIT_DIFF: git diff
 */

typedef enum MtkGitCmd_ {

    MTK_GIT_STATUS,

    MTK_GIT_PULL,
    MTK_GIT_ADD,
    MTK_GIT_COMMIT,
    MTK_GIT_PUSH,

    MTK_GIT_RESTORE,
    MTK_GIT_RESTORE_STAGED,
    MTK_GIT_RESTORE_ORIGIN,

    MTK_GIT_DIFF,
    MTK_GIT_DIFF_CACHED,
    MTK_GIT_DIFF_ORIGIN,

    MTK_GIT_DEFAULT_BRANCH,
    MTK_GIT_BRANCHES,
    MTK_GIT_SWITCH_BRANCH,
    MTK_GIT_DELETE_BRANCH,

    MTK_GIT_HAS_LOCAL_COMMITS,
    MTK_GIT_ORIGIN_STATUS,
    MTK_GIT_PORCELAIN,
    MTK_GIT_VIEW_FILE
} MtkGitCmd;


//typedef enum MtkGitCmd_ MtkGitCmd;


/**
 * mtk_git_cmd: 
 * @file: A #MtkFile*
 * @cmd: A #gchar*
 * @status: (out callee-allocates) : A #char**
 * @contents: (out callee-allocates) : A #char**
 * Returns: a #gint , the exit code
 */
gint mtk_git_cmd(MtkFile* file, MtkGitCmd cmd, gchar** status, gchar** contents );


/**
 * MtkAsyncGitCallbackFunc: 
 * @exit_code: A #gint
 * @status: A #gchar*
 * @out: A #gchar*
 * @user_data: A #gpointer
 *
 */

typedef void (*MtkAsyncGitCallbackFunc)(int exit_code, const gchar* status, const gchar* out, gpointer user_data);

#define MTK_ASYNC_GIT_CALLBACK(f) ((MtkAsyncGitCallbackFunc) (void (*)(int,gchar*,gchar*,gpointer)) (f))

/**
 * mtk_git_cmd_async: 
 * @file: A #MtkFile*
 * @cmd: A #gchar*
 * @callback: (closure user_data) (scope async): A #MtkAsyncGitCallbackFunc
 * @user_data: (closure): a #gpointer
 */
void mtk_git_cmd_async(MtkFile* file, MtkGitCmd cmd, MtkAsyncGitCallbackFunc callback, gpointer user_data );


gboolean mtk_git_has_local_commits(MtkFile* file);

gboolean mtk_git_switch_branch(MtkFile* file, const gchar* branch);
gboolean mtk_git_delete_branch(MtkFile* file, const gchar* branch);


gint mtk_git_commit(MtkFile* file, const gchar* msg, char** status,  char** contents );

//////////////////////////////////////////////////////////////////////////



typedef struct _MtkGitFile		MtkGitFile;
typedef struct _MtkGitFileClass	MtkGitFileClass;

typedef struct _GitStatus GitStatus;

/**
 * MtkGitFile: 
 */
struct _MtkGitFile {
/*< public >*/
    MtkFile parent;
    gchar* status;
    GitStatus* gitdata;    
};

/**
 * MtkGitFileClass: 
 */

struct _MtkGitFileClass {
/*< public >*/
    MtkFileClass parent;

};

GType		mtk_gitfile_get_type	() G_GNUC_CONST;


/**
 * mtk_gitfile_new: 
 * @fn : A #gchar*
 * Returns: (transfer full): a #MtkGitFile
 */
MtkGitFile*	mtk_gitfile_new( const gchar* fn);

#ifdef __cplusplus
}


#endif

#define MTK_FILE_TYPE		\
    (mtk_file_get_type())
#define MTK_FILE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_FILE_TYPE, MtkFile))
#define MTK_FILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_FILE_TYPE, MtkFileClass))
#define MTK_IS_FILETREE_FILE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_FILE_TYPE))
#define MTK_IS_FILETREE_FILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_FILE_TYPE))
#define MTK_FILE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_FILE_TYPE, MtkFileClass))


#define MTK_TREE_TYPE		\
    (mtk_filetree_get_type())
#define MTK_TREE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_TREE_TYPE, MtkFiletree))
#define MTK_TREE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_TREE_TYPE, MtkFiletreeClass))
#define MTK_TREE_IS_FILETREE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_TREE_TYPE))
#define MTK_TREE_IS_FILETREE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_TREE_TYPE))
#define MTK_TREE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_TREE_TYPE, MtkFiletreeClass))


#define MTK_GITFILE_TYPE		\
    (mtk_gitfile_get_type())
#define MTK_GITFILE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_GITFILE_TYPE, MtkGitFile))
#define MTK_GITFILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_GITFILE_TYPE, MtkGitFileClass))
#define MTK_IS_FILETREE_GITFILE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_GITFILE_TYPE))
#define MTK_IS_FILETREE_GITFILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_GITFILE_TYPE))
#define MTK_GITFILE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_GITFILE_TYPE, MtkGitFileClass))


#endif /* __MO_WEBKIT_H__ */
