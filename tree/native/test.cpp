#include <gtk/gtk.h>
#include <metacpp/meta.h>
#include "pywebkit.h"
#include "connector.h"
#include "mtkwebkit.h"
#include "mtkgit.h"

static std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}

class Controller
{
public:
    MtkFiletree* tree;
    PywebkitWebview* web;

    Gui ui;

    Controller()
    {
        ui
        .register_widgets( mtk_filetree_get_type, pywebkit_webview_get_type )
        .load("test.ui.xml")
        .bind(this)
        .show();

        //window = (GtkWidget*) ui.get_object("mainWindow");

        web = ui.get<PywebkitWebview>("web");
        webkit_bind(web,this);
        //pywebkit_webview_load_local_uri( (PywebkitWebview*)web,"index.html");

        tree = ui.get<MtkFiletree>("tree");

        MtkGitFile* file = mtk_gitfile_new( cwd().c_str() );
        mtk_filetree_add_root( tree, (MtkFile*)file, FALSE, ".*");

        //g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
//        ui.bind(this);
    }

    void onExit(GtkWidget* )
    {
        g_print("Exit\n");
        gtk_main_quit();
    }

    gboolean onFileOpen(GtkWidget* button)
    {
        g_print("CONTROLLER CLICK\n");

        std::function<void(std::string)> cb = [](std::string s)
        {
            g_print("CONTROLLER CLICKED %s\n", s.c_str());
        };

        Json::Value json(Json::arrayValue);
        json.append(Json::Value("CONTROLLER CLICK"));
        json.append(Json::Value("CONTROLLER CLICK2"));

        send_request( 
            web, 
            //cb, 
            "recvResponse", 
            json
        );
        return FALSE;
    }    

    int on_web(std::string msg)
    {
        g_print("web: %s\n", msg.c_str());
        return 42;
    }

    constexpr static auto meta() 
	{
		return meta::data(
			meta::mem_fun("onFileOpen", &Controller::onFileOpen),
			meta::mem_fun("on_web", &Controller::on_web),
			meta::mem_fun("onExit", &Controller::onExit)
		);
	}
};


int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    Controller controller;

    gtk_main();

    return 0;
}
