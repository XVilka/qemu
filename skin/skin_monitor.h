/*
 * Skin monitor command header
 *
 * Copyright (C) 2010 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "skin.h"

#ifndef SKIN_MONITOR_H__
#define SKIN_MONITOR_H__

#define ID_LENGTH 128

void do_info_zoom_print (Monitor *mon, const QObject *data);
void do_info_zoom (Monitor *mon, QObject **ret_data);
void do_info_rotate_print (Monitor *mon, const QObject *data);
void do_info_rotate (Monitor *mon, QObject **ret_data);
void do_info_keyboard_print (Monitor *mon, const QObject *data);
void do_info_keyboard (Monitor *mon, QObject **ret_data);
void do_info_rct_print (Monitor *mon, const QObject *data);
void do_info_rct (Monitor *mon, QObject **ret_data);
void do_info_skin_print (Monitor *mon, const QObject *data);
void do_info_skin (Monitor *mon, QObject **ret_data);
void do_info_skin_id_print (Monitor *mon, const QObject *data);
void do_info_skin_id (Monitor *mon, QObject **ret_data);
int do_set_zoom (Monitor *mon, const QDict *qdict, QObject **ret_data);
int do_set_rotate (Monitor *mon, const QDict *qdict, QObject **ret_data);
int do_set_keyboard (Monitor *mon, const QDict *qdict, QObject **ret_data);
int do_set_rct (Monitor *mon, const QDict *qdict, QObject **ret_data);
int do_set_skin (Monitor *mon, const QDict *qdict, QObject **ret_data);
int do_set_skin_id (Monitor *mon, const QDict *qdict, QObject **ret_data);

#endif /* SKIN_MONITOR_H__ */
