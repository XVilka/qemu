/*
 * Skin monitor command handling
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

#include "skin_monitor.h"
#include "skin_button.h"
#include "monitor.h"
#include "qjson.h"

char skinid[ID_LENGTH];

void skin_handle_rotate(int force);
void skin_handle_zooming(void);
void skin_update_keyboard(void *opaque);
int skin_reload(const char *skin_path);

void do_info_zoom_print (Monitor *mon, const QObject *data)
{
    QDict *qdict = qobject_to_qdict(data);

    monitor_printf(mon, "zoom=%d\n", (int)qdict_get_int(qdict, "zoom"));
}

void do_info_zoom (Monitor *mon, QObject **ret_data)
{
    *ret_data = qobject_from_jsonf("{ 'zoom': %d }", skin->zoom_factor);
}

void do_info_rotate_print (Monitor *mon, const QObject *data)
{
    QDict *qdict = qobject_to_qdict(data);

    monitor_printf(mon, "rotate=%s\n", qdict_get_str(qdict, "rotate"));
}

void do_info_rotate (Monitor *mon, QObject **ret_data)
{
    *ret_data = qobject_from_jsonf("{ 'rotate': %s }",
                                   (skin->rotate == on) ? "on" : "off");
}

void do_info_keyboard_print (Monitor *mon, const QObject *data)
{
    QDict *qdict = qobject_to_qdict(data);

    monitor_printf(mon, "keyboard=%s\n", qdict_get_str(qdict, "keyboard"));
}

void do_info_keyboard (Monitor *mon, QObject **ret_data)
{
    *ret_data = qobject_from_jsonf("{ 'keyboard': %s }",
                                   (skin->keyboard.visible == on) ? "on" : "off");
}

void do_info_rct_print (Monitor *mon, const QObject *data)
{
    QDict *qdict = qobject_to_qdict(data);

    monitor_printf(mon, "rct=%s\n", qdict_get_str(qdict, "rct"));
}

void do_info_rct (Monitor *mon, QObject **ret_data)
{
    *ret_data = qobject_from_jsonf("{ 'rct': %s }",
                                   (skin->rct == on) ? "on" : "off");
}


void do_info_skin_print (Monitor *mon, const QObject *data)
{
    QDict *qdict = qobject_to_qdict(data);

    monitor_printf(mon, "skin=%s\n", qdict_get_str(qdict, "skin"));
}

void do_info_skin (Monitor *mon, QObject **ret_data)
{
    *ret_data = qobject_from_jsonf("{ 'skin': %s }", skin->path);
}

void do_info_skin_id_print (Monitor *mon, const QObject *data)
{
    QDict *qdict = qobject_to_qdict(data);

    monitor_printf(mon, "skin_id=%s\n", qdict_get_str(qdict, "skin_id"));
}

void do_info_skin_id (Monitor *mon, QObject **ret_data)
{
    *ret_data = qobject_from_jsonf("{ 'skin_id': %s }", skinid);
}

int do_set_zoom (Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    int newzoom = qdict_get_int(qdict, "level");

    if (newzoom < ZOOM_MIN_FACTOR || newzoom > ZOOM_MAX_FACTOR) {
        qerror_report(QERR_INVALID_PARAMETER, "level");
        return -1;
    } else if (newzoom != skin->zoom_factor) {
        skin->zoom_factor = newzoom;
        skin_handle_zooming();
    }

    do_info_zoom(mon, ret_data);
    return 0;
}

int do_set_rotate (Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    const char *state = qdict_get_str(qdict, "state");
    int newstate;

    if (strcmp(state, "on") == 0) {
        newstate = on;
    } else if (strcmp(state, "off") == 0) {
        newstate = off;
    } else if (strcmp(state, "toggle") == 0) {
        newstate = (skin->rotate == on) ? off : on;
    } else {
        qerror_report(QERR_INVALID_PARAMETER, "state");
        return -1;
    }

    if (skin->rotate != newstate) {
        skin->rotate_req = newstate;
        skin_handle_rotate(0);
    }

    do_info_rotate(mon, ret_data);
    return 0;
}

int do_set_keyboard (Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    const char *state = qdict_get_str(qdict, "state");
    int newstate;

    if (strcmp(state, "on") == 0) {
        newstate = on;
    } else if (strcmp(state, "off") == 0) {
        newstate = off;
    } else if (strcmp(state, "toggle") == 0) {
        newstate = (skin->keyboard.visible == on) ? off : on;
    } else {
        qerror_report(QERR_INVALID_PARAMETER, "state");
        return -1;
    }

    if (skin->keyboard.visible != newstate) {
        skin->keyboard.visible = newstate;
        skin_update_keyboard((void *)1);
        kbd_put_keycode(0x3b);
        kbd_put_keycode(0x3b | 0x80);

        skin_button_checkswitch(skin, skin->keyboard.button);
    }

    do_info_keyboard(mon, ret_data);
    return 0;
}

int do_set_rct (Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    const char *state = qdict_get_str(qdict, "state");
    int newstate;

    if (strcmp(state, "on") == 0) {
        newstate = on;
    } else if (strcmp(state, "off") == 0) {
        newstate = off;
    } else if (strcmp(state, "toggle") == 0) {
        newstate = (skin->rct == on) ? off : on;
    } else {
        qerror_report(QERR_INVALID_PARAMETER, "state");
        return -1;
    }

    if (skin->rct != newstate) {
        skin->rct = newstate;
        skin_handle_rotate(1);
    }

    do_info_rct(mon, ret_data);
    return 0;
}

int do_set_skin (Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    const char *newskin = qdict_get_str(qdict, "filename");

    if (newskin) {
        if (!skin_reload(newskin)) {
            qerror_report(QERR_INVALID_PARAMETER, "filename");
            return -1;
        }

        do_info_skin(mon, ret_data);
    }
    return 0;
}

int do_set_skin_id (Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    const char *newid = qdict_get_str(qdict, "id");

    if (newid) {
        strncpy(skinid, newid, ID_LENGTH - 1);
        do_info_skin_id(mon, ret_data);
    }
    return 0;
}
