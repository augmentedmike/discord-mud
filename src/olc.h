#ifndef olc_H
#define olc_H

struct connection;
void cmd_room_edit(struct connection *conn, char *args);
void cmd_save(struct connection *conn, char *args);

#endif