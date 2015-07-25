/* main.c
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>

#include "gdict-app.h"
#include "gdict-common.h"

int
main (int argc, char *argv[])
{
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_set_prgname ("gnome-dictionary");

  if (!gdict_create_config_dir ())
    exit (1);

  if (!gdict_create_data_dir ())
    exit (1);

  gdict_init (&argc, &argv);

  return g_application_run (gdict_app_new (), argc, argv);
}
