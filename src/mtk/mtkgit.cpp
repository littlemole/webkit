#include "glue/common.h"
#include "mtk/mtkfiletree.h"
#include "mtk/mtkgit.h"
#include <iostream>

#define PROG "[MktGitFile] "

//////////////////////////////////////////////////////////////////////

// forwards

std::map<std::string,std::string> git_porcelain(MtkFile* file);
std::map<std::string,std::string> git_origin_status(MtkFile* file);


static GitStatus* mtk_gitfile_get_status_data(MtkGitFile* file);

gchar* virtual_mtk_gitfile_get_tooltip(MtkFile* file);
GList* virtual_mtk_gitfile_get_children(MtkFile* file, GtkTreeIter* iter);
void virtual_mtk_gitfile_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    );
void virtual_mtk_gitfile_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data
    );

static void mtk_gitfile_finalize(GObject *object);

//////////////////////////////////////////////////////////////////////

/**
 * SECTION: MtkGitFile
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


static void mtk_gitfile_init(MtkGitFile *file)
{
    if ( !file)
    {
        g_print( PROG "mtk_gitfile_init: MtkGitFile is null \n" );
        return;
    }

    file->status = g_strdup("  ");
    file->gitdata = 0;
}

MtkGitFile*	mtk_gitfile_new( const gchar* fn)
{
    MtkGitFile* file;

    file = (MtkGitFile*)g_object_new (MTK_GITFILE_TYPE, NULL);

    MtkFile* f = (MtkFile*)file;

    gval gv(fn);
/*    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );
    g_value_set_string( &gv, fn );
*/
    g_object_set_property( G_OBJECT(f),"path",&gv);    

    return file;    
}
 
static void mtk_gitfile_finalize(GObject *object)
{
    MtkGitFile* file = (MtkGitFile*)object;

    G_OBJECT_CLASS (mtk_gitfile_parent_class)->finalize (object);

    g_free( file->status );

    delete file->gitdata;
}


//////////////////////////////////////////////////////////////////////



gchar* virtual_mtk_gitfile_get_tooltip(MtkFile* file)
{
    MtkGitFile* f = (MtkGitFile*)file;
    GitStatus* gs = mtk_gitfile_get_status_data( f);

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

    std::map<std::string,std::string> git_paths = git_porcelain(file);
    std::map<std::string,std::string> origin = git_origin_status(file);
    
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

    GitStatus* gs = mtk_gitfile_get_status_data( (MtkGitFile*) self);

    std::ostringstream oss;
    if ( self->is_hidden)
    {
        oss << "<i>" << c << "</i>";
    }
    else
    {
        oss  << c;
    }
    g_free(c);

    gval gv(oss.str());
    
/*    GValue gv = G_VALUE_INIT;
    g_value_init( &gv, G_TYPE_STRING );
    g_value_set_string( &gv, oss.str().c_str() );
    */
    g_object_set_property( (GObject*)cell, "markup", &gv);

    gval gvc(gs->color);
/*    GValue gvc = G_VALUE_INIT;
    g_value_init( &gvc, G_TYPE_STRING );
    g_value_set_string( &gvc,gs->color.c_str() );
    */
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
//    GValue gv = G_VALUE_INIT;
//    g_value_init( &gv, G_TYPE_STRING );

    if ( self->is_empty )
    {
        //g_value_set_static_string( &gv, NULL );
        gval gv( (const char*)NULL, TRUE);
        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
    else
    {
        GitStatus* gs = mtk_gitfile_get_status_data( (MtkGitFile*) self);

        //g_value_set_static_string( &gv, gs->icon.c_str() );
        gval gv(gs->icon);
        g_object_set_property( (GObject*)cell, "stock_id", &gv);
    }
}

//////////////////////////////////////////////////////////////////////


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

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


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
    for( size_t i = 1; i < lines.size(); i++)
    {
        std::string line = lines[i];
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

    if( cmd == MTK_GIT_VIEW_FILE && file->is_directory)
    {
        c = make_cmd_target(cd, "ls -lah", target);
    }

    gchar* tmp = mtk_bash(c.c_str(),&exit_code);
    std::string content = tmp;
    g_free(tmp);

    if( cmd == MTK_GIT_BRANCHES)
    {
        return git_get_branches(exit_code,content,status,contents);
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

//////////////////////////////////////////////////////////////////////


gboolean mtk_git_has_local_commits(MtkFile* file)
{
    gchar* status = 0;
    gchar* content = 0;

    gint exit_code = mtk_git_cmd(file, MTK_GIT_HAS_LOCAL_COMMITS, &status, &content);
    if(exit_code != 0 || !content || strlen(content) == 0 )
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

    std::map<std::string,std::string> result;

    gint exit_code = mtk_git_cmd(file, MTK_GIT_ORIGIN_STATUS, &status, &content);
    if(exit_code != 0)
    {
        return result;
    }

    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    std::string gitroot = status;
    std::vector<std::string> lines = split(content,'\n');
    for( size_t i = 0; i < lines.size(); i++)
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

std::map<std::string,std::string> git_porcelain(MtkFile* file)
{
    gchar* status = 0;
    gchar* content = 0;

    std::map<std::string,std::string> result;

    gint exit_code = mtk_git_cmd(file, MTK_GIT_PORCELAIN, &status, &content);
    if(exit_code != 0)
    {
        return result;
    }

    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    std::vector<std::string> lines = split(content,'\n');

    std::string gitroot = status;
    for( size_t i = 0; i < lines.size(); i++)
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

/*
    for( auto it = result.begin(); it != result.end(); it++)
    {
        g_print("%s -> %s\n", (*it).first.c_str(), (*it).second.c_str() );
    }
*/    
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


gboolean mtk_git_create_branch(MtkFile* file, const gchar* branch)
{
    std::string cd = file->file_name;
    if ( !file->is_directory)
    {
        gchar* parent = mtk_file_get_parent(file);
        cd = parent;
        g_free(parent);
    }

    gint exit_code = 0;
    std::string cmd = std::string("git checkout -b ");
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

    std::cout << "GIT: " << c << std::endl;

    gchar* tmp = mtk_bash(c.c_str(),&exit_code);
    std::string content = tmp;
    g_free(tmp);

    std::cout << exit_code << " " << content << std::endl;

    size_t pos = content.find("\n");
    if ( pos == std::string::npos )
    {
        if(status)
        {
            *status = g_strdup(content.c_str());
        }
        if(contents)
        {
            *contents = g_strdup("");
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

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

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

    if( agc->cmd == MTK_GIT_BRANCHES)
    {
        return git_get_branches_async(agc,exit_code,content);
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

    if( cmd == MTK_GIT_VIEW_FILE && file->is_directory)
    {
        c = make_cmd_target(cd, "ls -lah", target);
    }
 

    AsyncGitClosure* agc = new AsyncGitClosure{ callback, user_data, cmd };

    mtk_bash_async(c.c_str(),git_on_async_cmd,agc);
}



//////////////////////////////////////////////////////////////////////
