/* gdict-common.h - shared code between application and applet
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

#include <sys/types.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "gdict-common.h"

gchar *
gdict_get_data_dir (void)
{
  gchar *retval;

  retval = g_build_filename (g_get_user_data_dir (),
                             g_get_prgname (),
                             NULL);
  return retval;
}

/* legacy data dir. pre 3.3.4 */
gchar *
gdict_get_old_data_dir (void)
{
  gchar *retval;

  retval = g_build_filename (g_get_home_dir (),
		  	     ".gnome2",
			     "gnome-dictionary",
			     NULL);
  
  return retval;
}

gchar *
gdict_get_config_dir (void)
{
  gchar *retval;

  retval = g_build_filename (g_get_user_config_dir (),
                             g_get_prgname (),
                             NULL);
  return retval;
}

gboolean
gdict_migrate_configs (void)
{
  gchar *old_data_dir_name; // this one was used for configs only
  gchar *config_dir_name;
  gboolean res = TRUE;

  old_data_dir_name = gdict_get_old_data_dir ();
  config_dir_name = gdict_get_config_dir ();

  /* move configs from pre-XDG directory to right place */
  if (g_file_test (old_data_dir_name, G_FILE_TEST_IS_DIR))
    {
      g_message ("Migrating old configs to XDG directory layout...");

      if (g_rename (old_data_dir_name, config_dir_name) == -1)
        {
          g_critical ("Unable to rename file '%s' to '%s': %s",
                      old_data_dir_name,
                      config_dir_name,
                      g_strerror (errno));

          res = FALSE;
        }
    }

  g_free (config_dir_name);
  g_free (old_data_dir_name);

  return res;
}

gboolean
gdict_create_config_dir (void)
{
  gchar *config_dir_name;
  gboolean res = TRUE;

  config_dir_name = gdict_get_config_dir ();

  gdict_migrate_configs ();

  if (!g_file_test (config_dir_name, G_FILE_TEST_IS_DIR))
    {
      if (!g_file_test (config_dir_name, G_FILE_TEST_IS_DIR)) {
        g_message ("Creating XDG config direcotry: %s", config_dir_name);

        if (g_mkdir (config_dir_name, 0700) == -1)
          {
            g_critical ("Unable to create directory '%s': %s",
                        config_dir_name,
                        g_strerror (errno));

            res = FALSE;
          }
      }
    }

  g_free (config_dir_name);
  return res;
}

/* create the data directory inside $HOME, if it doesn't exist yet */
gboolean
gdict_create_data_dir (void)
{
  gchar *data_dir_name;
  
  data_dir_name = gdict_get_data_dir ();

  if (g_mkdir (data_dir_name, 0700) == -1)
    {
      /* this is weird, but sometimes there's a "gnome-dictionary" file
       * inside $HOME/.gnome2; see bug #329126.
       */
      if ((errno == EEXIST) &&
          (g_file_test (data_dir_name, G_FILE_TEST_IS_REGULAR)))
        {
          gchar *backup = g_strdup_printf ("%s.pre-2-14", data_dir_name);
	      
	  if (g_rename (data_dir_name, backup) == -1)
	    {
              g_critical ("Unable to rename file '%s' to '%s': %s",
                          data_dir_name,
                          backup,
                          g_strerror (errno));

	      g_free (backup);

              goto error;
            }

	  g_free (backup);
	  
          if (g_mkdir (data_dir_name, 0700) == -1)
            {
              g_critical ("Unable to create the data directory '%s': %s",
                          data_dir_name,
                          g_strerror (errno));

              goto error;
            }

	  goto success;
	}
      
      if (errno != EEXIST)
        {
          g_critical ("Unable to create the data directory '%s': %s",
                      data_dir_name,
                      g_strerror (errno));
          goto error;
	}
    }

success:
  g_free (data_dir_name);

  return TRUE;

error:
  g_free (data_dir_name);

  return FALSE;
}

/* shows an error dialog making it transient for @parent */
void
gdict_show_error_dialog (GtkWindow   *parent,
			 const gchar *message,
			 const gchar *detail)
{
  GtkWidget *dialog;
  
  g_return_if_fail ((parent == NULL) || (GTK_IS_WINDOW (parent)));
  g_return_if_fail (message != NULL);
  
  dialog = gtk_message_dialog_new (parent,
  				   GTK_DIALOG_DESTROY_WITH_PARENT,
  				   GTK_MESSAGE_ERROR,
  				   GTK_BUTTONS_OK,
  				   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), "");
  
  if (detail)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
  					      "%s", detail);
  
  if (parent && gtk_window_get_group (parent))
    gtk_window_group_add_window (gtk_window_get_group (parent), GTK_WINDOW (dialog));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

void
gdict_show_gerror_dialog (GtkWindow   *parent,
			  const gchar *message,
			  GError      *error)
{
  g_return_if_fail ((parent == NULL) || (GTK_IS_WINDOW (parent)));
  g_return_if_fail (message != NULL);
  g_return_if_fail (error != NULL);
  
  gdict_show_error_dialog (parent, message, error->message);
            
  g_error_free (error);
  error = NULL;
}
