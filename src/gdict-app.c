/* gdict-app.c - main application class
 *
 * This file is part of GNOME Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "gdict-common.h"
#include "gdict-pref-dialog.h"
#include "gdict-app.h"

static GdictApp *singleton = NULL;

struct _GdictAppClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (GdictApp, gdict_app, G_TYPE_OBJECT);

static GOptionEntry gdict_app_goptions[] = {
  {
    "look-up", 0,
    0,
    G_OPTION_ARG_STRING_ARRAY, NULL,
    N_("Words to look up"), N_("WORD")
  },
  {
    "match", 0,
    0,
    G_OPTION_ARG_STRING_ARRAY, NULL,
    N_("Words to match"), N_("WORD")
  },
  {
    "source", 's',
    0,
    G_OPTION_ARG_STRING, NULL,
    N_("Dictionary source to use"), N_("NAME")
  },
  {
    "database", 'D',
    0,
    G_OPTION_ARG_STRING, NULL,
    N_("Database to use"), N_("NAME")
  },
  {
    "strategy", 'S',
    0,
    G_OPTION_ARG_STRING, NULL,
    N_("Strategy to use"), N_("NAME")
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_STRING_ARRAY, NULL,
    N_("Words to look up"), N_("WORDS")
  },
  { NULL },
};

static void
gdict_app_cmd_preferences (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  GtkApplication *app = user_data;
  GdictWindow *window;

  g_assert (GTK_IS_APPLICATION (app));

  window = GDICT_WINDOW (gtk_application_get_windows (app)->data);
  gdict_show_pref_dialog (GTK_WIDGET (window),
                          _("Dictionary Preferences"),
                          window->loader);
}

static void
gdict_app_cmd_help (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GtkApplication *app = user_data;
  GdictWindow *window;
  GError *err = NULL;

  g_return_if_fail (GTK_IS_APPLICATION (app));

  window = GDICT_WINDOW (gtk_application_get_windows (app)->data);
  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (window)),
                "help:gnome-dictionary",
                gtk_get_current_event_time (), &err);
  if (err)
    {
      gdict_show_gerror_dialog (GTK_WINDOW (window),
                                _("There was an error while displaying help"),
                                err);
      g_error_free (err);
    }
}

static void
gdict_app_cmd_about (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  GtkApplication *app = user_data;
  GdictWindow *window;

  g_assert (GTK_IS_APPLICATION (app));

  window = GDICT_WINDOW (gtk_application_get_windows (app)->data);
  gdict_show_about_dialog (GTK_WIDGET (window));
}

static void
gdict_app_cmd_quit (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GtkApplication *app = user_data;
  GList *windows;

  g_assert (GTK_IS_APPLICATION (app));

  windows = gtk_application_get_windows (app);
  g_list_foreach (windows, (GFunc)gtk_widget_destroy, NULL);
}

static const GActionEntry app_entries[] =
{
  { "preferences", gdict_app_cmd_preferences, NULL, NULL, NULL },
  { "help", gdict_app_cmd_help, NULL, NULL, NULL },
  { "about", gdict_app_cmd_about, NULL, NULL, NULL },
  { "quit", gdict_app_cmd_quit, NULL, NULL, NULL }
};

static void
gdict_app_finalize (GObject *object)
{
  GdictApp *app = GDICT_APP (object);

  if (app->loader)
    g_object_unref (app->loader);

  G_OBJECT_CLASS (gdict_app_parent_class)->finalize (object);
}

static void
gdict_app_class_init (GdictAppClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = gdict_app_finalize;
}

static void
gdict_app_init (GdictApp *app)
{
}

static void
gdict_activate (GApplication *application,
                GdictApp     *gdict_app)
{
  GtkWidget *window;

}

gchar **
strv_concat (gchar **strv1, gchar **strv2)
{
  gchar **tmp;
  guint len1, len2;
  gint i;

  len1 = g_strv_length (strv1);
  len2 = g_strv_length (strv2);
  tmp = g_realloc (strv1, len1 + len2 + 1);
  for (i = 0; i < len2; i++)
    tmp[len1 + i] = (gchar *)strv2[i];
  tmp[len1 + len2] = NULL;

  return tmp;
}

static int
gdict_command_line (GApplication *application,
                    GApplicationCommandLine *cmd_line,
                    GdictApp                *gdict_app)
{
  GSList *l;
  gsize words_len, i;
  GVariantDict *options;
  gchar **gdict_lookup_words = NULL;
  gchar **gdict_match_words = NULL;
  gchar **remaining = NULL;
  const gchar *gdict_source_name = NULL;
  const gchar *gdict_database_name = NULL;
  const gchar *gdict_strategy_name = NULL;

  options = g_application_command_line_get_options_dict (cmd_line);

  g_variant_dict_lookup (options, "look-up", "^as", &gdict_lookup_words);
  g_variant_dict_lookup (options, "match", "^as", &gdict_match_words);
  g_variant_dict_lookup (options, "source", "&s", &gdict_source_name);
  g_variant_dict_lookup (options, "database", "&s", &gdict_database_name);
  g_variant_dict_lookup (options, "strategy", "&s", &gdict_strategy_name);
  g_variant_dict_lookup (options, G_OPTION_REMAINING, "^as", &remaining);

  if (remaining != NULL)
    {
      if (gdict_match_words != NULL)
        {
          gchar **tmp;
          tmp = strv_concat (gdict_match_words, remaining);
          g_strfreev (gdict_match_words);
          g_strfreev (remaining);
          gdict_match_words = tmp;
        }
      else
        {
          gdict_match_words = remaining;
        }
      remaining = NULL;
    }

  if (gdict_lookup_words == NULL &&
      gdict_match_words == NULL)
    {
      GtkWidget *window = gdict_window_new (GDICT_WINDOW_ACTION_CLEAR,
                                            singleton->loader,
                                            gdict_source_name,
                                            gdict_database_name,
                                            gdict_strategy_name,
                                            NULL);
      gtk_window_set_application (GTK_WINDOW (window), singleton->app);
      gtk_widget_show (window);

      goto out;
    }

  if (gdict_lookup_words != NULL)
    words_len = g_strv_length (gdict_lookup_words);
  else
    words_len = 0;

  for (i = 0; i < words_len; i++)
    {
      const gchar *word = gdict_lookup_words[i];
      GtkWidget *window;

      window = gdict_window_new (GDICT_WINDOW_ACTION_LOOKUP,
                                 singleton->loader,
                                 gdict_source_name,
                                 gdict_database_name,
                                 gdict_strategy_name,
                                 word);
      
      gtk_window_set_application (GTK_WINDOW (window), singleton->app);
      gtk_widget_show (window);
    }

  if (gdict_match_words != NULL)
    words_len = g_strv_length (gdict_match_words);
  else
    words_len = 0;

  for (i = 0; i < words_len; i++)
    {
      const gchar *word = gdict_match_words[i];
      GtkWidget *window;

      window = gdict_window_new (GDICT_WINDOW_ACTION_MATCH,
      				 singleton->loader,
				 gdict_source_name,
                                 gdict_database_name,
                                 gdict_strategy_name,
				 word);
      
      gtk_window_set_application (GTK_WINDOW (window), singleton->app);
      gtk_widget_show (window);
    }

out:
  g_strfreev (gdict_lookup_words);
  g_strfreev (gdict_match_words);

  return 0;
}

static void
gdict_startup (GApplication *application,
               gpointer      user_data)
{
  GtkBuilder *builder = gtk_builder_new ();
  GError * error = NULL;
  static const char *lookup_accels[2] = { "<Primary>l", NULL };
  static const char *escape_accels[2] = { "Escape", NULL };

  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   application);

  if (!gtk_builder_add_from_file (builder,
                                  PKGDATADIR "/gnome-dictionary-menus.ui",
                                  &error))
    {
      g_warning ("Building menus failed: %s", error->message);
      g_error_free (error);

      return;
    }

  gtk_application_set_menubar (GTK_APPLICATION (application),
                               G_MENU_MODEL (gtk_builder_get_object (builder, "menubar")));
  gtk_application_set_app_menu (GTK_APPLICATION (application),
                                G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.lookup", lookup_accels);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.escape", escape_accels);

  g_object_unref (builder);
}

void
gdict_main (int    *argc,
            char ***argv)
{
  gchar *loader_path;

  g_set_prgname ("gnome-dictionary");

  if (!gdict_create_config_dir ())
    exit (1);

  if (!gdict_create_data_dir ())
    exit (1);

  gtk_window_set_default_icon_name ("accessories-dictionary");

  /* the main application instance */
  singleton = g_object_new (gdict_app_get_type (), NULL);

  /* add user's path for fetching dictionary sources */  
  singleton->loader = gdict_source_loader_new ();
  loader_path = gdict_get_config_dir ();
  gdict_source_loader_add_search_path (singleton->loader, loader_path);
  g_free (loader_path);

  singleton->app = gtk_application_new ("org.gnome.Dictionary", G_APPLICATION_HANDLES_COMMAND_LINE);
  g_application_add_main_option_entries (G_APPLICATION (singleton->app), gdict_app_goptions);
  g_signal_connect (singleton->app, "command-line", G_CALLBACK (gdict_command_line), singleton);
  g_signal_connect (singleton->app, "startup", G_CALLBACK (gdict_startup), singleton);

  g_application_run (G_APPLICATION (singleton->app), *argc, *argv);
}

void
gdict_cleanup (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_init()\n");
      return;
    }

  g_object_unref (singleton->app);
  g_object_unref (singleton);
}
