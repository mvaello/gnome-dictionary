/* gdict-database-chooser-button.c - display widget for database names
 *
 * Copyright (C) 2015  Juan R. García Blanco <juanrgar@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:gdict-database-chooser-button
 * @short_description: Display the list of available databases in a popover
 *
 * Each #GdictContext has a list of databases, that is dictionaries that
 * can be queried. #GdictDatabaseChooserButton is a button widget that,
 * when clicked, loads all available databases in the associated
 * #GdictContext, and displays them in a #GdictDatabaseChooser contained
 * in a #GtkPopover.
 *
 * #GdictDatabaseChooserButton is available since Gdict 0.10
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "gdict-database-chooser-button.h"
#include "gdict-database-chooser.h"
#include "gdict-utils.h"
#include "gdict-debug.h"
#include "gdict-private.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"

#define GDICT_DATABASE_CHOOSER_BUTTON_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_DATABASE_CHOOSER_BUTTON, GdictDatabaseChooserButtonPrivate))

struct _GdictDatabaseChooserButtonPrivate
{
  GtkWidget *db_chooser;
  GtkWidget *stack;
  GtkWidget *spinner;
  GtkWidget *popover;

  GdkCursor *busy_cursor;

  guint start_id;
  guint end_id;
  guint error_id;

  guint is_loaded : 1;
};

enum
{
  PROP_0,

  PROP_CONTEXT,
  PROP_COUNT
};

enum
{
  DATABASE_ACTIVATED,
  SELECTION_CHANGED,

  LAST_SIGNAL
};

static guint db_chooser_button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GdictDatabaseChooserButton,
               gdict_database_chooser_button,
               GTK_TYPE_MENU_BUTTON)

static void
set_gdict_context (GdictDatabaseChooserButton *chooser_button,
		   GdictContext		      *context)
{
  GdictDatabaseChooserButtonPrivate *priv;
  GdictContext *old_context;

  g_assert (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser_button));
  priv = chooser_button->priv;

  old_context = gdict_database_chooser_get_context (GDICT_DATABASE_CHOOSER (priv->db_chooser));
  if (context == old_context)
    return;

  if (old_context)
    {
      if (priv->start_id)
        {
          g_signal_handler_disconnect (old_context, priv->start_id);
          g_signal_handler_disconnect (old_context, priv->end_id);

          priv->start_id = 0;
          priv->end_id = 0;
        }

      if (priv->error_id)
        {
          g_signal_handler_disconnect (old_context, priv->error_id);

          priv->error_id = 0;
        }

      priv->is_loaded = FALSE;
    }

  gdict_database_chooser_set_context (GDICT_DATABASE_CHOOSER (priv->db_chooser), context);
}

static void
get_gdict_context (GdictDatabaseChooserButton *chooser_button,
		   GValue		      *value)
{
  GdictDatabaseChooserButtonPrivate *priv;

  g_assert (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser_button));
  priv = chooser_button->priv;

  g_object_get (G_OBJECT (priv->db_chooser),
		"context", value,
		NULL);
}

static void
get_results_count (GdictDatabaseChooserButton *chooser_button,
		   GValue		      *value)
{
  GdictDatabaseChooserButtonPrivate *priv;

  g_assert (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser_button));
  priv = chooser_button->priv;

  g_object_get (G_OBJECT (priv->db_chooser),
		"count", value,
		NULL);
}

static void
gdict_database_chooser_button_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gdict_database_chooser_button_parent_class)->finalize (gobject);
}

static void
gdict_database_chooser_button_dispose (GObject *gobject)
{
  GdictDatabaseChooserButton *chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (gobject);
  GdictDatabaseChooserButtonPrivate *priv = chooser_button->priv;

  g_clear_object (&priv->busy_cursor);

  G_OBJECT_CLASS (gdict_database_chooser_button_parent_class)->dispose (gobject);
}

static void
gdict_database_chooser_button_set_property (GObject      *gobject,
					    guint         prop_id,
					    const GValue *value,
					    GParamSpec   *pspec)
{
  GdictDatabaseChooserButton *chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (gobject);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      set_gdict_context (chooser_button, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
gdict_database_chooser_button_get_property (GObject    *gobject,
					    guint       prop_id,
					    GValue     *value,
					    GParamSpec *pspec)
{
  GdictDatabaseChooserButton *chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (gobject);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      get_gdict_context (chooser_button, value);
      break;
    case PROP_COUNT:
      get_results_count (chooser_button, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
selection_changed_cb (GdictDatabaseChooser *chooser,
                      gpointer		    user_data)
{
  g_signal_emit (user_data, db_chooser_button_signals[SELECTION_CHANGED], 0);
}

static void
database_activated_cb (GdictDatabaseChooser *chooser,
		       const gchar	    *name,
		       const gchar	    *description,
		       gpointer		     user_data)
{
  GdictDatabaseChooserButton *chooser_button = user_data;

  gtk_button_set_label (GTK_BUTTON (chooser_button), name);
  gtk_widget_set_tooltip_text (GTK_WIDGET (chooser_button), description);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chooser_button), FALSE);

  g_signal_emit (user_data, db_chooser_button_signals[DATABASE_ACTIVATED], 0,
		 name, description);
}

static void
lookup_start_cb (GdictContext *context,
		 gpointer      user_data)
{
  GdictDatabaseChooserButton *chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (user_data);
  GdictDatabaseChooserButtonPrivate *priv = chooser_button->priv;

  if (!priv->busy_cursor)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (chooser_button));

      priv->busy_cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
    }

  if (gtk_widget_get_window (GTK_WIDGET (chooser_button)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser_button)),
			   priv->busy_cursor);

  gtk_spinner_start (GTK_SPINNER (priv->spinner));
}

static void
lookup_end_cb (GdictContext *context,
	       gpointer      user_data)
{
  GdictDatabaseChooserButton *chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (user_data);
  GdictDatabaseChooserButtonPrivate *priv = chooser_button->priv;

  if (gtk_widget_get_window (GTK_WIDGET (chooser_button)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser_button)),
			   NULL);

  gtk_spinner_stop (GTK_SPINNER (priv->spinner));
  gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "chooser");

  gtk_widget_set_size_request (GTK_WIDGET (priv->popover), 300, -1);
  gtk_widget_set_vexpand (GTK_WIDGET (priv->popover), TRUE);

  priv->is_loaded = TRUE;
}

static void
error_cb (GdictContext *context,
          const GError *error,
	  gpointer      user_data)
{
  GdictDatabaseChooserButton *chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (user_data);
  GdictDatabaseChooserButtonPrivate *priv = chooser_button->priv;

  gtk_spinner_stop (GTK_SPINNER (priv->spinner));

  priv->is_loaded = FALSE;
}

static void
gdict_database_chooser_button_clicked (GtkButton *button)
{
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (gdict_database_chooser_button_parent_class);
  GdictDatabaseChooserButton *chooser_button;
  GdictDatabaseChooserButtonPrivate *priv;
  GtkToggleButton *toggle;
  GdictContext *context;
  gboolean active;

  button_class->clicked (button);

  toggle = GTK_TOGGLE_BUTTON (button);
  active = gtk_toggle_button_get_active (toggle);

  GDICT_NOTE (CHOOSER, "Button clicked: %s", active ? "active" : "inactive");

  chooser_button = GDICT_DATABASE_CHOOSER_BUTTON (button);
  priv = GDICT_DATABASE_CHOOSER_BUTTON_GET_PRIVATE (chooser_button);

  if (active && !priv->is_loaded)
    {
      gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "spinner");

      if (!priv->start_id)
	{
	  context = gdict_database_chooser_get_context (GDICT_DATABASE_CHOOSER (priv->db_chooser));
	  priv->start_id = g_signal_connect (context, "database-lookup-start",
					     G_CALLBACK (lookup_start_cb),
					     chooser_button);
	  priv->end_id = g_signal_connect (context, "database-lookup-end",
					   G_CALLBACK (lookup_end_cb),
					   chooser_button);
	}

      if (!priv->error_id)
	priv->error_id = g_signal_connect (context, "error",
					   G_CALLBACK (error_cb),
					   chooser_button);

      gdict_database_chooser_refresh (GDICT_DATABASE_CHOOSER (priv->db_chooser));
    }
}

static void
gdict_database_chooser_button_class_init (GdictDatabaseChooserButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  gobject_class->finalize = gdict_database_chooser_button_finalize;
  gobject_class->dispose = gdict_database_chooser_button_dispose;
  gobject_class->set_property = gdict_database_chooser_button_set_property;
  gobject_class->get_property = gdict_database_chooser_button_get_property;
  button_class->clicked = gdict_database_chooser_button_clicked;

  /**
   * GdictDatabaseChooserButton:context:
   *
   * The #GdictContext used to retrieve the list of available databases.
   *
   * Since: 0.10
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			"Context",
  				   			"The GdictContext object used to get the list of databases",
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
  /**
   * GdictDatabaseChooserButton:count:
   *
   * The number of displayed databases or, if no #GdictContext is set, -1.
   *
   * Since: 0.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COUNT,
                                   g_param_spec_int ("count",
                                                     "Count",
                                                     "The number of available databases",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READABLE));

  /**
   * GdictDatabaseChooserButton::database-activated:
   * @chooser: the database chooser button that received the signal
   * @name: the name of the activated database
   * @description: the description of the activated database
   *
   * The ::database-activated signal is emitted each time the user
   * activated a row in the database chooser widget, either by double
   * clicking on it or by a keyboard event.
   *
   * Since: 0.10
   */
  db_chooser_button_signals[DATABASE_ACTIVATED] =
    g_signal_new ("database-activated",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GdictDatabaseChooserButtonClass, database_activated),
		  NULL, NULL,
		  gdict_marshal_VOID__STRING_STRING,
		  G_TYPE_NONE, 2,
		  G_TYPE_STRING,
		  G_TYPE_STRING);
  /**
   * GdictDatabaseChooserButton::selection-changed:
   * @chooser: the database chooser button that received the signal
   *
   * The ::selection-changed signal is emitted each time the selection
   * inside the database chooser has been changed.
   *
   * Since: 0.12
   */
  db_chooser_button_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictDatabaseChooserButtonClass, selection_changed),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (gobject_class, sizeof (GdictDatabaseChooserButtonPrivate));
}

static void
gdict_database_chooser_button_init (GdictDatabaseChooserButton *chooser_button)
{
  GdictDatabaseChooserButtonPrivate *priv;

  chooser_button->priv = priv = GDICT_DATABASE_CHOOSER_BUTTON_GET_PRIVATE (chooser_button);

  priv->start_id = 0;
  priv->end_id = 0;
  priv->error_id = 0;

  priv->popover = gtk_popover_new (GTK_WIDGET (chooser_button));
  gtk_menu_button_set_direction (GTK_MENU_BUTTON (chooser_button), GTK_ARROW_NONE);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (chooser_button), priv->popover);

  priv->stack = gtk_stack_new ();
  gtk_container_add (GTK_CONTAINER (priv->popover), priv->stack);
  gtk_widget_show (priv->stack);

  priv->spinner = gtk_spinner_new ();
  gtk_stack_add_named (GTK_STACK (priv->stack), priv->spinner, "spinner");
  gtk_widget_show (priv->spinner);

  priv->db_chooser = gdict_database_chooser_new ();
  gtk_stack_add_named (GTK_STACK (priv->stack), priv->db_chooser, "chooser");
  gtk_widget_show (priv->db_chooser);

  g_signal_connect (priv->db_chooser,
                    "selection-changed", G_CALLBACK (selection_changed_cb),
                    chooser_button);

  g_signal_connect (priv->db_chooser,
                    "database-activated", G_CALLBACK (database_activated_cb),
                    chooser_button);

  priv->is_loaded = FALSE;
}

/**
 * gdict_database_chooser_button_new:
 *
 * Creates a new #GdictDatabaseChooserButton widget. A Database chooser button
 * widget can be used to display the list of available databases on a dictionary
 * source using the #GdictContext representing it. After creation, the
 * #GdictContext can be set using gdict_database_chooser_button_set_context().
 *
 * Return value: the newly created #GdictDatabaseChooserButton widget.
 *
 * Since: 0.10
 */
GtkWidget *
gdict_database_chooser_button_new (void)
{
  return g_object_new (GDICT_TYPE_DATABASE_CHOOSER_BUTTON, NULL);
}

/**
 * gdict_database_chooser_button_new_with_context:
 * @context: a #GdictContext
 *
 * Creates a new #GdictDatabaseChooserButton, using @context as the representation
 * of the dictionary source to query for the list of available databases.
 *
 * Return value: the newly created #GdictDatabaseChooserButton widget.
 *
 * Since: 0.10
 */
GtkWidget *
gdict_database_chooser_button_new_with_context (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);

  return g_object_new (GDICT_TYPE_DATABASE_CHOOSER_BUTTON,
                       "context", context,
                       NULL);
}

/**
 * gdict_database_chooser_button_get_context:
 * @chooser: a #GdictDatabaseChooserButton
 *
 * Retrieves the #GdictContext used by @chooser.
 *
 * Return value: (transfer none): a #GdictContext or %NULL
 *
 * Since: 0.10
 */
GdictContext *
gdict_database_chooser_button_get_context (GdictDatabaseChooserButton *chooser)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), NULL);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_get_context (GDICT_DATABASE_CHOOSER (priv->db_chooser));
}

/**
 * gdict_database_chooser_button_set_context:
 * @chooser: a #GdictDatabaseChooserButton
 * @context: a #GdictContext
 *
 * Sets the #GdictContext to be used to query a dictionary source
 * for the list of available databases.
 *
 * Since: 0.10
 */
void
gdict_database_chooser_button_set_context (GdictDatabaseChooserButton *chooser,
					   GdictContext		      *context)
{
  g_return_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser));
  g_return_if_fail (context == NULL || GDICT_IS_CONTEXT (context));

  set_gdict_context (chooser, context);

  g_object_notify (G_OBJECT (chooser), "context");
}

/**
 * gdict_database_chooser_button_get_databases:
 * @chooser: a #GdictDatabaseChooserButton
 * @length: return location for the length of the returned vector
 *
 * Gets the list of available database names.
 *
 * Return value: (transfer full): a newly allocated, %NULL terminated string vector
 *   containing database names. Use g_strfreev() to deallocate it.
 *
 * Since: 0.10
 */
gchar **
gdict_database_chooser_button_get_databases (GdictDatabaseChooserButton *chooser,
					     gsize			*length)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), NULL);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_get_databases (GDICT_DATABASE_CHOOSER (priv->db_chooser),
					       length);
}

/**
 * gdict_database_chooser_button_has_database:
 * @chooser: a #GdictDatabaseChooserButton
 * @database: the name of a database
 *
 * Checks whether the @chooser displays @database
 *
 * Return value: %TRUE if the search database name is present
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_button_has_database (GdictDatabaseChooserButton	*chooser,
					    const gchar			*database)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), FALSE);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_has_database (GDICT_DATABASE_CHOOSER (priv->db_chooser),
					      database);
}

/**
 * gdict_database_chooser_button_count_databases:
 * @chooser: a #GdictDatabaseChooserButton
 *
 * Returns the number of databases found.
 *
 * Return value: the number of databases or -1 if no context is set
 *
 * Since: 0.10
 */
gint
gdict_database_chooser_button_count_databases (GdictDatabaseChooserButton *chooser)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), -1);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_count_databases (GDICT_DATABASE_CHOOSER (priv->db_chooser));
}

/**
 * gdict_database_chooser_button_clear:
 * @chooser: a #GdictDatabaseChooserButton
 *
 * Clears @chooser.
 *
 * Since: 0.10
 */
void
gdict_database_chooser_button_clear (GdictDatabaseChooserButton *chooser)
{
  GdictDatabaseChooserButtonPrivate *priv;

  g_return_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser));

  priv = chooser->priv;

  gdict_database_chooser_clear (GDICT_DATABASE_CHOOSER (priv->db_chooser));
  priv->is_loaded = FALSE;
}

/**
 * gdict_database_chooser_button_select_database:
 * @chooser: a #GdictDatabaseChooserButton
 * @db_name: name of the database to select
 *
 * Selects the database with @db_name inside the @chooser widget.
 *
 * Return value: %TRUE if the database was found and selected
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_button_select_database (GdictDatabaseChooserButton	*chooser,
					       const gchar			*db_name)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), FALSE);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_select_database (GDICT_DATABASE_CHOOSER (priv->db_chooser),
						 db_name);
}

/**
 * gdict_database_chooser_button_unselect_database:
 * @chooser: a #GdictDatabaseChooserButton
 * @db_name: name of the database to unselect
 *
 * Unselects the database @db_name inside the @chooser widget
 *
 * Return value: %TRUE if the database was found and unselected
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_button_unselect_database (GdictDatabaseChooserButton	*chooser,
                                          const gchar				*db_name)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), FALSE);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_unselect_database (GDICT_DATABASE_CHOOSER (priv->db_chooser),
						   db_name);
}

/**
 * gdict_database_chooser_button_set_current_database:
 * @chooser: a #GdictDatabaseChooserButton
 * @db_name: the name of the database
 *
 * Sets @db_name as the current database. This function will select
 * and activate the corresponding row, if the database is found.
 *
 * Return value: %TRUE if the database was found and set
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_button_set_current_database (GdictDatabaseChooserButton	*chooser,
                                             const gchar			*db_name)
{
  gboolean valid;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), FALSE);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  valid = gdict_database_chooser_set_current_database (GDICT_DATABASE_CHOOSER (priv->db_chooser),
						       db_name);

  gtk_button_set_label (GTK_BUTTON (chooser), db_name);

  return valid;
}

/**
 * gdict_database_chooser_button_get_current_database:
 * @chooser: a #GdictDatabaseChooserButton
 *
 * Retrieves the name of the currently selected database inside @chooser
 *
 * Return value: the name of the selected database. Use g_free() on the
 *   returned string when done using it
 *
 * Since: 0.10
 */
gchar *
gdict_database_chooser_button_get_current_database (GdictDatabaseChooserButton *chooser)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER_BUTTON (chooser), NULL);
  GdictDatabaseChooserButtonPrivate *priv = chooser->priv;

  return gdict_database_chooser_get_current_database (GDICT_DATABASE_CHOOSER (priv->db_chooser));
}
