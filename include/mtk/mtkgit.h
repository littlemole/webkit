#ifndef __MO_MTK_GIT_H__
#define __MO_MTK_GIT_H__

#include <gtk/gtk.h>
#include "mtkfiletree.h"

#ifdef __cplusplus
extern "C"
{
#endif



/////////////////////////////////////////////////////////////////////////////



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

gboolean mtk_git_create_branch(MtkFile* file, const gchar* branch);
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
 * mtk_gitfile_new: (constructor)
 * @fn : A #gchar*
 * Returns: (transfer full): a #MtkGitFile
 */
MtkGitFile*	mtk_gitfile_new( const gchar* fn);

#ifdef __cplusplus
}


#endif


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
