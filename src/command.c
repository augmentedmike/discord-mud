#include "command.h"
#include "olc.h"
#include "main.h"


void cmd_look(struct connection *conn, char *args) {
    room *current_room = conn->current_room;
    char response[2048];

    int has_exits = 0;
    for(int i = 0; i < NUM_DIRECTIONS; i++) {
        if (current_room->exits[i]) {
            has_exits = 1;
            break;
        }
    }

    snprintf(response, sizeof(response), "You are in %s.\n%s\nExits: ", current_room->name, current_room->description);
    if (!has_exits) {
        strcat(response, "None");
    }

    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        if (current_room->exits[i]) {
            strcat(response, direction_names[i]);
            strcat(response, " ");
        }
    }
    strcat(response, "\n");
    send(conn->sockfd, response, strlen(response), 0);
}

void cmd_quit(struct connection *conn, char *args) {
    send(conn->sockfd, "Goodbye!\n", 9, 0);
    close(conn->sockfd);
    conn->active = 0;
}

void cmd_move(struct connection *conn, char *args) {
    if (!args) {
        send(conn->sockfd, "Usage: move <direction>\n", 24, 0);
        return;
    }
    int dir = str_to_direction(args);
    if (dir == -1) {
        send_direction_list(conn->sockfd, "Invalid direction. Valid directions are: ");
        return;
    }
    room *next_room = conn->current_room->exits[dir];
    if (!next_room) {
        send(conn->sockfd, "You can't go that way.\n", 23, 0);
        return;
    }
    conn->current_room = next_room;
    cmd_look(conn, NULL);
}

void cmd_say(struct connection *conn, char *args) {
    if (!args) {
        send(conn->sockfd, "Usage: say <message>\n", 21, 0);
        return;
    }
    char message[1024];
    snprintf(message, sizeof(message), "%s says: %s\n", conn->player->name, args);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].current_room == conn->current_room) {
            if (&clients[i] == conn) {
                char self_message[1024];
                snprintf(self_message, sizeof(self_message), "You say: %s\n", args);
                send(conn->sockfd, self_message, strlen(self_message), 0);
            } else {
                send(clients[i].sockfd, message, strlen(message), 0);
            }
        }
    }

}

struct command commands[] = {
    {"look", cmd_look, 1},
    {"roomedit", cmd_room_edit, 2},
    {"quit", cmd_quit, 2},
    {"move", cmd_move, 2},
    {"save", cmd_save, 4},
    {"say", cmd_say, 2},
    {NULL, NULL, 0}
};

void parse_command(struct connection *conn, char *input) {
    char *cmd_name = strtok(input, " ");
    char *args = strtok(NULL, "");

    if(!cmd_name) {
        return;
    }

    int len = strlen(cmd_name);
    if (len == 0) {
        send(conn->sockfd, "Please enter a command.\n", 24, 0);
        return;
    }
    for (int i = 0; commands[i].name != NULL; i++) {
        if (len >= commands[i].min_length && strncmp(cmd_name, commands[i].name, len) == 0) {
            commands[i].handler(conn, args);
            return;
        }
    }
    int dir = str_to_direction(cmd_name);
    if (dir != -1) {
        cmd_move(conn, cmd_name);
        return;
    }
    send(conn->sockfd, "Unknown command.\n", 17, 0);
}