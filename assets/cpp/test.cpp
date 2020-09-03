 #include <gtk/gtk.h>
 #include <metacpp/meta.h>

static void activate (GtkApplication* app, gpointer  user_data)
{
  GtkWidget *window;
  GtkWidget *label;

  window = gtk_application_window_new (app);
  label = gtk_label_new ("Hello GNOME!");
  gtk_container_add (GTK_CONTAINER (window), label);
  gtk_window_set_title (GTK_WINDOW (window), "Welcome to GNOME");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);
  gtk_widget_show_all (window);
}

static gboolean on_click(GtkWidget* label, GdkEvent* e, gpointer user_data)
{
    g_print("CLICK\n");
    return FALSE;
}

class Controller
{
public:

    void activate (GtkApplication* app)//, gpointer  user_data)
    {
        GtkWidget *window;
        GtkWidget *label;

        window = gtk_application_window_new (app);
        label = gtk_label_new ("Hello GNOME Controller!");
        gtk_container_add (GTK_CONTAINER (window), label);
        gtk_window_set_title (GTK_WINDOW (window), "Welcome to GNOME");
        gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);
        gtk_widget_show_all (window);
    }

    gboolean on_click(GtkWidget* label, GdkEvent* e)
    {
        g_print("CONTROLLER CLICK\n");
        return FALSE;
    }    

    constexpr static auto meta() 
	{
		return meta::data(
			meta::mem_fun("activate", &Controller::activate),
			meta::mem_fun("on_click", &Controller::on_click)
		);
	}
};

template<class T, class R, class ... Args>
struct bound_method{
    T* that;
    R(T::*fun)(Args ...);
};

template<class S, class T, class ... Args>
void connect(S* source, const char* signal, void(T::*fun)(Args ...), void* t)
{
    
    auto bm = new bound_method<T,void,Args...>{ (T*)t, fun };

    auto handler = []( Args ... args, gpointer user_data) -> void
    {
        auto bm = (bound_method<T,void,Args...>*)user_data;
        T* t = (T*) bm->that;
        void(T::*fun)(Args ...) = bm->fun;
        (t->*fun)(args...);
        //delete bm;
    };

    void(*ptr)(Args...,gpointer) = handler;

    auto h = reinterpret_cast<void(*)()>(ptr);
    g_signal_connect (source, signal, h, bm);
    
}

template<class S, class R, class T, class ... Args>
void connect(S* source, const char* signal, R(T::*fun)(Args ...), void* t)
{
    
    auto bm = new bound_method<T,R,Args...>{ (T*)t, fun };

    auto handler = []( Args ... args, gpointer user_data) -> R
    {
        auto bm = (bound_method<T,R,Args...>*)user_data;
        T* t = (T*) bm->that;
        g_print("handler : t is %i\n", (void*)t );
        R(T::*fun)(Args ...) = bm->fun;
        return (t->*fun)(args...);
        //delete bm;
    };

    R(*ptr)(Args...,gpointer) = handler;

    auto h = reinterpret_cast<void(*)()>(ptr);
    g_signal_connect (source, signal, h, bm);
    
}

void connector(GtkBuilder *builder,
                          GObject *object,
                          const gchar *signal_name,
                          const gchar *handler_name,
                          GObject *connect_object,
                          GConnectFlags flags,
                          gpointer user_data)
{
    Controller* controller = (Controller*) user_data;

    g_print("connect: %s -> %s \n", signal_name, handler_name);

    meta::find<Controller>(handler_name, [object,signal_name,user_data](auto m)
    {
        g_print("  found: %s  \n", m.name);
        connect(object,signal_name, m.member, user_data);
    });
    //connect(object,signal_name, &Controller::activate, &controller);
}

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

  GtkApplication *app;
  int status = 0;

  Controller controller;

  GtkBuilder *builder = gtk_builder_new();
  GError* error = 0;

  if ( gtk_builder_add_from_file(builder,"test.ui.xml", &error) == 0)
  {
      g_printerr ("Error loading file: %s\n", error->message);
      g_clear_error (&error);
      return 1;
  }

  auto window = gtk_builder_get_object (builder, "mainWindow");
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  auto label = gtk_builder_get_object (builder, "label");
  gtk_widget_set_has_window((GtkWidget*)label,TRUE);
  gtk_widget_set_events((GtkWidget*)label,GDK_BUTTON_PRESS_MASK);
//  int id = g_signal_connect (label, "button-press-event", G_CALLBACK (on_click), NULL);
//  g_print("on_click: %i\n",id);

    gtk_window_set_default_size((GtkWindow*)window,300,300);

    gtk_builder_connect_signals_full(builder, connector, &controller);

  gtk_widget_show_all((GtkWidget*)window);

  gtk_main();
//  app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
//  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

//  connect(app,"activate", &Controller::activate, &controller);

//  status = g_application_run (G_APPLICATION (app), argc, argv);
//  g_object_unref (app);

  return status;
}
