
#include "olc.h"
#include "main.h"

void cmd_room_edit(struct connection *conn, char *args) {
    if (!args) {
        send(conn->sockfd, "Usage: roomedit <field> <value>\n", 32, 0);
        return;
    }
    char *field = strtok(args, " ");
    char *value = strtok(NULL, "");

    if(strcmp(field, "create") == 0) {
        room *new_room = malloc(sizeof(room));
        memset(new_room, 0, sizeof(room));
        add_room(new_room);
        send(conn->sockfd, "New room created with ID: ", 26, 0);
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d\n", new_room->id);
        send(conn->sockfd, id_str, strlen(id_str), 0);
        return;
    }

    if (!field || !value) {
        send(conn->sockfd, "Usage: roomedit <field> <value>\n", 32, 0);
        return;
    }

    room *current_room = conn->current_room;
    if (strcmp(field, "name") == 0) {
        strncpy(current_room->name, value, sizeof(current_room->name)-1);
        current_room->name[sizeof(current_room->name)-1] = '\0';
        send(conn->sockfd, "Room name updated.\n", 19, 0);
    } else if (strcmp(field, "description") == 0) {
        strncpy(current_room->description, value, sizeof(current_room->description)-1);
        current_room->description[sizeof(current_room->description)-1] = '\0';
        send(conn->sockfd, "Room description updated.\n", 27, 0);
    } else if (strcmp(field, "exit") == 0) {
        char *dir_str = strtok(value, " ");
        char *target_id_str = strtok(NULL, " ");
        if (!dir_str || !target_id_str) {
            send(conn->sockfd, "Usage: roomedit exit <direction> <target_room_id>\n", 50, 0);
            return;
        }
        int dir = str_to_direction(dir_str);
        if (dir == -1) {
            send_direction_list(conn->sockfd, "Invalid direction. Valid directions are: ");
            return;
        }
        int target_id = atoi(target_id_str);
        room *target_room = find_room_by_id(target_id);
        if (!target_room) {
            send(conn->sockfd, "Target room not found.\n", 23, 0);
            return;
        }
        current_room->exits[dir] = target_room;
        send(conn->sockfd, "Room exit updated.\n", 19, 0);
    } else if (strcmp(field, "rmexit") == 0) {
        char *dir_str = strtok(value, " ");
        if (!dir_str) {
            send(conn->sockfd, "Usage: roomedit rmexit <direction>\n", 50, 0);
            return;
        }
        int dir = str_to_direction(dir_str);
        if(dir == -1) {
            send_direction_list(conn->sockfd, "Invalid direction. Valid directions are: ");
            return;
        }
        current_room->exits[dir] = NULL;  
    } else {
        send(conn->sockfd, "Unknown field. Valid fields are: name, description, exit\n", 52, 0);
    }
}

void cmd_save(struct connection *conn, char *args) {
    (void)args; // Unused parameter
    save_world("world.dat");
    send(conn->sockfd, "World saved.\n", 13, 0);
}