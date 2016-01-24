/* gdict-pref-dialog.c - preferences dialog
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
#include <time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gi18n.h>

#include <gio/gio.h>

#include "gdict-source-dialog.h"
#include "gdict-pref-dialog.h"
#include "gdict-common.h"

#define GDICT_PREFERENCES_UI 	"/org/gnome/Dictionary/gdict-pref-dialog.ui"

/*******************
 * GdictPrefDialog *
 *******************/

static GtkWidget *global_dialog = NULL;

enum
{
  SOURCES_ACTIVE_COLUMN = 0,
  SOURCES_NAME_COLUMN,
  SOURCES_DESCRIPTION_COLUMN,
  
  SOURCES_N_COLUMNS
};

struct _GdictPrefDialog
{
  GtkDialog parent_instance;

  GtkBuilder *builder;

  GSettings *settings;

  gchar *print_font;
  gchar *active_source;
  GdictSourceLoader *loader;
  GtkListStore *sources_list;

  /* direct pointers to widgets */
  GtkWidget *preferences_root;
  GtkWidget *preferences_notebook;
  GtkWidget *sources_treeview;
  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *edit_button;
  GtkWidget *font_button;
};

struct _GdictPrefDialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,

  PROP_SOURCE_LOADER
};

G_DEFINE_TYPE (GdictPrefDialog, gdict_pref_dialog, GTK_TYPE_DIALOG)

static gboolean
select_active_source_name (GtkTreeModel *model,
			   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   gpointer      data)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (data);
  gboolean is_active;
  
  gtk_tree_model_get (model, iter,
      		      SOURCES_ACTIVE_COLUMN, &is_active,
      		      -1);
  if (is_active)
    {
      GtkTreeSelection *selection;
      
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_treeview));
      
      gtk_tree_selection_select_iter (selection, iter);
      
      return TRUE;
    }
  
  return FALSE;
}

static void
sources_view_cursor_changed_cb (GtkTreeView       *tree_view,
				GdictPrefDialog   *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GdictSource *source;
  gboolean is_selected;
  gchar *name;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_treeview));
  if (!selection)
    return;

  is_selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!is_selected)
    return;

  gtk_tree_model_get (model, &iter, SOURCES_NAME_COLUMN, &name, -1);
  if (!name)
    return;
  else
    {
      source = gdict_source_loader_get_source (dialog->loader, name);
      gtk_widget_set_sensitive (dialog->edit_button, gdict_source_is_editable (source));
      gtk_widget_set_sensitive (dialog->remove_button, gdict_source_is_editable (source));
      g_object_unref (source);
    }
}

static void
update_sources_view (GdictPrefDialog *dialog)
{
  const GSList *sources, *l;
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_treeview), NULL);
  
  gtk_list_store_clear (dialog->sources_list);
  
  /* force update of the sources list */
  gdict_source_loader_update (dialog->loader);
  
  sources = gdict_source_loader_get_sources (dialog->loader);
  for (l = sources; l != NULL; l = l->next)
    {
      GdictSource *source = GDICT_SOURCE (l->data);
      GtkTreeIter iter;
      const gchar *name, *description;
      gboolean is_active = FALSE;
      
      name = gdict_source_get_name (source);
      description = gdict_source_get_description (source);
      if (!description)
	description = name;

      if (strcmp (name, dialog->active_source) == 0)
        is_active = TRUE;

      gtk_list_store_append (dialog->sources_list, &iter);
      gtk_list_store_set (dialog->sources_list, &iter,
      			  SOURCES_ACTIVE_COLUMN, is_active,
      			  SOURCES_NAME_COLUMN, name,
      			  SOURCES_DESCRIPTION_COLUMN, description,
      			  -1);
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_treeview),
  			   GTK_TREE_MODEL (dialog->sources_list));
  
  /* select the currently active source name */
  gtk_tree_model_foreach (GTK_TREE_MODEL (dialog->sources_list),
  			  select_active_source_name,
  			  dialog);
  sources_view_cursor_changed_cb (GTK_TREE_VIEW(dialog->sources_treeview), dialog);
}

static void
source_renderer_toggled_cb (GtkCellRendererToggle *renderer,
			    const gchar           *path,
			    GdictPrefDialog       *dialog)
{
  GtkTreePath *treepath;
  GtkTreeIter iter;
  gboolean res;
  gboolean is_active;
  gchar *name;
  
  treepath = gtk_tree_path_new_from_string (path);
  res = gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->sources_list),
                                 &iter,
                                 treepath);
  if (!res)
    {
      gtk_tree_path_free (treepath);
      
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->sources_list), &iter,
      		      SOURCES_NAME_COLUMN, &name,
      		      SOURCES_ACTIVE_COLUMN, &is_active,
      		      -1);
  if (!is_active && name != NULL)
    {
      g_free (dialog->active_source);
      dialog->active_source = g_strdup (name);

      g_settings_set_string (dialog->settings, GDICT_SETTINGS_SOURCE_KEY, dialog->active_source);
      update_sources_view (dialog);

      g_free (name);
    }
  
  gtk_tree_path_free (treepath);
}

static void
sources_view_row_activated_cb (GtkTreeView       *tree_view,
			       GtkTreePath       *tree_path,
			       GtkTreeViewColumn *tree_iter,
			       GdictPrefDialog   *dialog)
{
  GtkWidget *edit_dialog;
  gchar *source_name;
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (tree_view);
  if (!model)
    return;
  
  if (!gtk_tree_model_get_iter (model, &iter, tree_path))
    return;
  
  gtk_tree_model_get (model, &iter, SOURCES_NAME_COLUMN, &source_name, -1);
  if (!source_name)
    return;
  
  edit_dialog = gdict_source_dialog_new (GTK_WINDOW (dialog),
					 _("View Dictionary Source"),
					 GDICT_SOURCE_DIALOG_VIEW,
					 dialog->loader,
					 source_name);
  gtk_dialog_run (GTK_DIALOG (edit_dialog));

  gtk_widget_destroy (edit_dialog);
  g_free (source_name);

  update_sources_view (dialog);
}

static void
build_sources_view (GdictPrefDialog *dialog)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  
  if (dialog->sources_list)
    return;
    
  dialog->sources_list = gtk_list_store_new (SOURCES_N_COLUMNS,
  					     G_TYPE_BOOLEAN,  /* active */
  					     G_TYPE_STRING,   /* name */
  					     G_TYPE_STRING    /* description */);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dialog->sources_list),
  					SOURCES_DESCRIPTION_COLUMN,
  					GTK_SORT_ASCENDING);
  
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
  g_signal_connect (renderer, "toggled",
  		    G_CALLBACK (source_renderer_toggled_cb),
  		    dialog);
  
  column = gtk_tree_view_column_new_with_attributes ("active",
  						     renderer,
  						     "active", SOURCES_ACTIVE_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("description",
  						     renderer,
  						     "text", SOURCES_DESCRIPTION_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_treeview), column);
  
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->sources_treeview), FALSE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_treeview),
  			   GTK_TREE_MODEL (dialog->sources_list));

  g_signal_connect (dialog->sources_treeview, "row-activated",
		    G_CALLBACK (sources_view_row_activated_cb),
		    dialog);
  g_signal_connect (dialog->sources_treeview, "cursor-changed",
		    G_CALLBACK (sources_view_cursor_changed_cb),
		    dialog);
}

static void
source_add_clicked_cb (GdictPrefDialog *dialog)
{
  GtkWidget *add_dialog;
  
  add_dialog = gdict_source_dialog_new (GTK_WINDOW (dialog),
  					_("Add Dictionary Source"),
  					GDICT_SOURCE_DIALOG_CREATE,
  					dialog->loader,
  					NULL);

  gtk_dialog_run (GTK_DIALOG (add_dialog));

  gtk_widget_destroy (add_dialog);

  update_sources_view (dialog);
}

static void
source_remove_clicked_cb (GdictPrefDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean is_selected;
  gchar *name, *description;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_treeview));
  if (!selection)
    return;
  
  is_selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!is_selected)
    return;
    
  gtk_tree_model_get (model, &iter,
  		      SOURCES_NAME_COLUMN, &name,
  		      SOURCES_DESCRIPTION_COLUMN, &description,
  		      -1);
  if (!name) 
    return;
  else
    {
      GtkWidget *confirm_dialog;
      gint response;
      
      confirm_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					       GTK_DIALOG_DESTROY_WITH_PARENT,
      					       GTK_MESSAGE_WARNING,
      					       GTK_BUTTONS_NONE,
      					       _("Remove \"%s\"?"), description);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (confirm_dialog),
      						_("This will permanently remove the "
      						  "dictionary source from the list."));
      
      gtk_dialog_add_button (GTK_DIALOG (confirm_dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
      gtk_dialog_add_button (GTK_DIALOG (confirm_dialog), _("_Remove"), GTK_RESPONSE_OK);
      
      gtk_window_set_title (GTK_WINDOW (confirm_dialog), "");
      
      response = gtk_dialog_run (GTK_DIALOG (confirm_dialog));
      gtk_widget_destroy (confirm_dialog);

      if (response == GTK_RESPONSE_CANCEL)
        goto out;
    }
  
  if (gdict_source_loader_remove_source (dialog->loader, name))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  else
    {
      GtkWidget *error_dialog;
      gchar *message;
      
      message = g_strdup_printf (_("Unable to remove source '%s'"),
      				 description);
      
      error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					     GTK_DIALOG_DESTROY_WITH_PARENT,
      					     GTK_MESSAGE_ERROR,
      					     GTK_BUTTONS_OK,
      					     "%s", message);
      gtk_window_set_title (GTK_WINDOW (error_dialog), "");
      
      gtk_dialog_run (GTK_DIALOG (error_dialog));
      
      gtk_widget_destroy (error_dialog);
    }

out:
  g_free (name);
  g_free (description);
  
  update_sources_view (dialog);
}

static void
source_edit_clicked_cb (GdictPrefDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean is_selected;
  gchar *name;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_treeview));
  if (!selection)
    return;

  is_selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!is_selected)
    return;

  gtk_tree_model_get (model, &iter, SOURCES_NAME_COLUMN, &name, -1);
  if (!name)
    return;
  else
    {
      GtkWidget *edit_dialog;

      edit_dialog = gdict_source_dialog_new (GTK_WINDOW (dialog),
                                             _("Edit Dictionary Source"),
                                             GDICT_SOURCE_DIALOG_EDIT,
                                             dialog->loader,
                                             name);
      gtk_dialog_run (GTK_DIALOG (edit_dialog));

      gtk_widget_destroy (edit_dialog);
    }

  g_free (name);

  update_sources_view (dialog);
}

static void
set_source_loader (GdictPrefDialog   *dialog,
		   GdictSourceLoader *loader)
{
  if (!dialog->sources_list)
    return;
  
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  dialog->loader = g_object_ref (loader);
  
  update_sources_view (dialog);
}

static void
font_button_font_set_cb (GdictPrefDialog *dialog,
                         GtkFontButton   *font_button)
{
  const gchar *font;
  
  font = gtk_font_button_get_font_name (font_button);
  if (!font || font[0] == '\0')
    return;

  if (dialog->print_font && (strcmp (dialog->print_font, font) == 0))
    return;
  
  g_free (dialog->print_font);
  dialog->print_font = g_strdup (font);

  g_settings_set_string (dialog->settings, GDICT_SETTINGS_PRINT_FONT_KEY, dialog->print_font);
}

static void
gdict_pref_dialog_finalize (GObject *object)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);

  g_clear_object (&dialog->settings);
  g_clear_object (&dialog->loader);

  g_free (dialog->active_source);
  
  G_OBJECT_CLASS (gdict_pref_dialog_parent_class)->finalize (object);
}

static void
gdict_pref_dialog_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      set_source_loader (dialog, g_value_get_object (value));
      break;

    default:
      break;
    }
}

static void
gdict_pref_dialog_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      g_value_set_object (value, dialog->loader);
      break;

    default:
      break;
    }
}

static void
gdict_pref_dialog_class_init (GdictPrefDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->set_property = gdict_pref_dialog_set_property;
  gobject_class->get_property = gdict_pref_dialog_get_property;
  gobject_class->finalize = gdict_pref_dialog_finalize;

  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  				   			"Source Loader",
  				   			"The GdictSourceLoader used by the application",
  				   			GDICT_TYPE_SOURCE_LOADER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  gtk_widget_class_set_template_from_resource (widget_class, GDICT_PREFERENCES_UI);

  gtk_widget_class_bind_template_child (widget_class, GdictPrefDialog, preferences_notebook);
  gtk_widget_class_bind_template_child (widget_class, GdictPrefDialog, sources_treeview);
  gtk_widget_class_bind_template_child (widget_class, GdictPrefDialog, add_button);
  gtk_widget_class_bind_template_child (widget_class, GdictPrefDialog, remove_button);
  gtk_widget_class_bind_template_child (widget_class, GdictPrefDialog, edit_button);
  gtk_widget_class_bind_template_child (widget_class, GdictPrefDialog, font_button);

  gtk_widget_class_bind_template_callback (widget_class, source_add_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, source_remove_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, source_edit_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, font_button_font_set_cb);
}

static void
gdict_pref_dialog_init (GdictPrefDialog *dialog)
{
  gchar *font;

  gtk_widget_init_template (GTK_WIDGET (dialog));

  dialog->settings = g_settings_new (GDICT_SETTINGS_SCHEMA);
  dialog->active_source = g_settings_get_string (dialog->settings, GDICT_SETTINGS_SOURCE_KEY);

  build_sources_view (dialog);

  font = g_settings_get_string (dialog->settings, GDICT_SETTINGS_PRINT_FONT_KEY);
  gtk_font_button_set_font_name (GTK_FONT_BUTTON (dialog->font_button), font);
  g_free (font);
  
  gtk_widget_show_all (dialog->preferences_notebook);
}

void
gdict_show_pref_dialog (GtkWidget         *parent,
			const gchar       *title,
			GdictSourceLoader *loader)
{
  GtkWidget *dialog;
  
  g_return_if_fail (GTK_IS_WIDGET (parent));
  g_return_if_fail (GDICT_IS_SOURCE_LOADER (loader));
  
  if (parent != NULL)
    dialog = g_object_get_data (G_OBJECT (parent), "gdict-pref-dialog");
  else
    dialog = global_dialog;
  
  if (dialog == NULL)
    {
      dialog = g_object_new (GDICT_TYPE_PREF_DIALOG,
                             "source-loader", loader,
                             "title", title,
                             "use-header-bar", 1,
                             NULL);
      
      g_object_ref_sink (dialog);
      
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_widget_hide_on_delete),
                        NULL);
      
      if (parent != NULL && GTK_IS_WINDOW (parent))
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
          gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
          g_object_set_data_full (G_OBJECT (parent), "gdict-pref-dialog",
                                  dialog,
                                  g_object_unref);
        }
      else
        global_dialog = dialog;
    }

  gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (parent));
  gtk_window_present (GTK_WINDOW (dialog));
}
