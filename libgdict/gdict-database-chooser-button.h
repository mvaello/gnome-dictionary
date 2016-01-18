/* gdict-database-chooser-button.h - display widget for database names
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

#ifndef __GDICT_DATABASE_CHOOSER_BUTTON_H__
#define __GDICT_DATABASE_CHOOSER_BUTTON_H__

#include <gtk/gtk.h>
#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_DATABASE_CHOOSER_BUTTON		(gdict_database_chooser_button_get_type ())
#define GDICT_DATABASE_CHOOSER_BUTTON(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_DATABASE_CHOOSER_BUTTON, GdictDatabaseChooserButton))
#define GDICT_IS_DATABASE_CHOOSER_BUTTON(obj) \
(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_DATABASE_CHOOSER_BUTTON))
#define GDICT_DATABASE_CHOOSER_BUTTON_CLASS(klass) \
(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_DATABASE_CHOOSER_BUTTON, GdictDatabaseChooserButtonClass))
#define GDICT_IS_DATABASE_CHOOSER_BUTTON_CLASS(klass) \
(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_DATABASE_CHOOSER_BUTTON))
#define GDICT_DATABASE_CHOOSER_BUTTON_GET_CLASS(obj) \
(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_DATABASE_CHOOSER_BUTTON, GdictDatabaseChooserButtonClass))

typedef struct _GdictDatabaseChooserButton		GdictDatabaseChooserButton;
typedef struct _GdictDatabaseChooserButtonPrivate	GdictDatabaseChooserButtonPrivate;
typedef struct _GdictDatabaseChooserButtonClass		GdictDatabaseChooserButtonClass;

struct _GdictDatabaseChooserButton
{
  /*< private >*/
  GtkMenuButton parent_instance;
  
  GdictDatabaseChooserButtonPrivate *priv;
};

struct _GdictDatabaseChooserButtonClass
{
  /*< private >*/
  GtkMenuButtonClass parent_class;

  /*< public >*/
  void (*database_activated) (GdictDatabaseChooserButton *chooser_button,
		  	      const gchar		 *name,
			      const gchar		 *description);
  void (*selection_changed)  (GdictDatabaseChooserButton *chooser);

  /*< private >*/
  /* padding for future expansion */
  void (*_gdict_padding2) (void);
  void (*_gdict_padding3) (void);
  void (*_gdict_padding4) (void);
  void (*_gdict_padding5) (void);
  void (*_gdict_padding6) (void);
};

GType         gdict_database_chooser_button_get_type             (void) G_GNUC_CONST;

GtkWidget *   gdict_database_chooser_button_new                  (void);
GtkWidget *   gdict_database_chooser_button_new_with_context     (GdictContext         *context);

GdictContext *gdict_database_chooser_button_get_context          (GdictDatabaseChooserButton *chooser);
void          gdict_database_chooser_button_set_context          (GdictDatabaseChooserButton *chooser,
								  GdictContext		     *context);
gboolean      gdict_database_chooser_button_select_database      (GdictDatabaseChooserButton *chooser,
								  const gchar		     *db_name);
gboolean      gdict_database_chooser_button_unselect_database    (GdictDatabaseChooserButton *chooser,
								  const gchar		     *db_name);
gboolean      gdict_database_chooser_button_set_current_database (GdictDatabaseChooserButton *chooser,
								  const gchar		     *db_name);
gchar *       gdict_database_chooser_button_get_current_database (GdictDatabaseChooserButton *chooser) G_GNUC_MALLOC;
gchar **      gdict_database_chooser_button_get_databases        (GdictDatabaseChooserButton *chooser,
								  gsize			     *length) G_GNUC_MALLOC;
gint          gdict_database_chooser_button_count_databases      (GdictDatabaseChooserButton *chooser);
gboolean      gdict_database_chooser_button_has_database         (GdictDatabaseChooserButton *chooser,
								  const gchar		     *database);
void          gdict_database_chooser_button_clear                (GdictDatabaseChooserButton *chooser);

G_END_DECLS

#endif /* __GDICT_DATABASE_CHOOSER_BUTTON_H__ */
