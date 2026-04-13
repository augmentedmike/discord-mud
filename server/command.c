#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "command.h"
#include "olc.h"
#include "main.h"
#include "color.h"


int in_room(struct connection *conn, room *r) {
    return conn->active && conn->state == STATE_PLAYING && conn->current_room == r;
}

void send_to_player(struct connection *conn, const void *buf, size_t len, int flags) {
    if (conn->player && conn->player->current_status == STATUS_SLEEPING) return;
    send(conn->sockfd, buf, len, flags);
}

void cmd_look(struct connection *conn, char *args) {
    (void)args; // unused
    room *current_room = conn->current_room;
    char response[2048];


    char room_name[ROOM_NAME_MAX + 16];
    colorize(room_name, sizeof(room_name), COLOR_CYAN, current_room->name);
    char room_desc[ROOM_DESC_MAX + 16];
    colorize(room_desc, sizeof(room_desc), COLOR_GREEN, current_room->description);
    snprintf(response, sizeof(response), "\n\n%s.\n\n%s\n\n", room_name, room_desc);


    int has_exits = 0;
    for(int i = 0; i < NUM_DIRECTIONS; i++) {
        if (current_room->exits[i]) {
            has_exits = 1;
            break;
        }
    }

    char exit_list[256] = "";

    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        if (current_room->exits[i]) {
            strcat(exit_list, direction_names[i]);
            strcat(exit_list, " ");
        }
    }

    if (!has_exits) {
        strcpy(exit_list, "None");
    }
    char colored_exit_list[strlen(exit_list) + 16];
    colorize(colored_exit_list, sizeof(colored_exit_list), COLOR_YELLOW, exit_list);
    strcat(response, "Exits: ");
    strcat(response, colored_exit_list);
    strcat(response, "\n\n");

    // list players in the room
    char player_list[512] = "";
    int player_count = 0;
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (in_room(&clients[i], current_room)) {
            if(clients[i].player == conn->player) {
                continue;
            }
            if (player_count > 0) {
                strcat(player_list, ", ");
            }
            char desc_copy[256];
            char player_desc[288];
            strncpy(desc_copy, clients[i].player->short_description, sizeof(desc_copy) - 1);
            desc_copy[sizeof(desc_copy) - 1] = '\0';
            int dlen = strlen(desc_copy);
            if (dlen > 0 && desc_copy[dlen - 1] == '.')
                desc_copy[dlen - 1] = '\0';
            snprintf(player_desc, sizeof(player_desc), "%s is %s here.", desc_copy, status_names[clients[i].player->current_status]);
            strcat(player_list, player_desc);

            player_count++;
        }
    }
    if (player_count == 0) {
        strcat(player_list, "None");
    }
    char colored_player_list[strlen(player_list) + 16];
    colorize(colored_player_list, sizeof(colored_player_list), COLOR_MAGENTA, player_list);
    strcat(colored_player_list, "\n\n");
    strcat(response, colored_player_list);

    char message[1024];
    snprintf(message, sizeof(message), "%s looks around the room.\n", conn->player->name);

    // loop and alert the room of her looking
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (in_room(&clients[i], conn->current_room)) {
            send_to_player(&clients[i], message, strlen(message), 0);
        }
    }

    send(conn->sockfd, response, strlen(response), 0);
}

void cmd_quit(struct connection *conn, char *args) {
    (void)args; // unused
    send(conn->sockfd, "Goodbye!\n", 9, 0);
    cleanup_connection(conn);
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
        if (in_room(&clients[i], conn->current_room)) {
            if (&clients[i] == conn) {
                char self_message[1024];
                snprintf(self_message, sizeof(self_message), "You say: %s\n", args);
                send(conn->sockfd, self_message, strlen(self_message), 0);
            } else {
                send_to_player(&clients[i], message, strlen(message), 0);
            }
        }
    }
}

// Convert third person verb to second person: "waves" -> "wave", "scratches" -> "scratch"
void deconjugate(const char *verb, char *out, size_t out_size) {
    int len = strlen(verb);
    if (len >= 4 && (strcmp(verb + len - 4, "ches") == 0 ||
                     strcmp(verb + len - 4, "shes") == 0)) {
        // scratches -> scratch, pushes -> push
        snprintf(out, out_size, "%.*s", len - 2, verb);
    } else if (len >= 3 && strcmp(verb + len - 3, "ses") == 0) {
        // tosses -> toss
        snprintf(out, out_size, "%.*s", len - 2, verb);
    } else if (len >= 3 && strcmp(verb + len - 3, "xes") == 0) {
        // relaxes -> relax
        snprintf(out, out_size, "%.*s", len - 2, verb);
    } else if (len >= 3 && strcmp(verb + len - 3, "zes") == 0) {
        // buzzes -> buzz
        snprintf(out, out_size, "%.*s", len - 2, verb);
    } else if (len >= 3 && strcmp(verb + len - 3, "ies") == 0) {
        // cries -> cry
        snprintf(out, out_size, "%.*sy", len - 3, verb);
    } else if (len >= 1 && verb[len - 1] == 's') {
        // grins -> grin, waves -> wave
        snprintf(out, out_size, "%.*s", len - 1, verb);
    } else {
        snprintf(out, out_size, "%s", verb);
    }
}

void cmd_emote(struct connection *conn, char *args) {
    if (!args) {
        send(conn->sockfd, "Usage: emote <action>\n", 22, 0);
        return;
    }

    // Build room message: "Mike waves at Monse.\n"
    char room_msg[1024];
    snprintf(room_msg, sizeof(room_msg), "%s %s\n", conn->player->name, args);

    // Build self message: "You wave at Monse.\n"
    char verb[64];
    char rest[960] = "";
    char *space = strchr(args, ' ');
    if (space) {
        int vlen = space - args;
        snprintf(verb, sizeof(verb), "%.*s", vlen, args);
        snprintf(rest, sizeof(rest), " %s", space + 1);
    } else {
        snprintf(verb, sizeof(verb), "%s", args);
    }
    char verb2[64];
    deconjugate(verb, verb2, sizeof(verb2));
    char self_msg[1024];
    snprintf(self_msg, sizeof(self_msg), "You %s%s\n", verb2, rest);

    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (in_room(&clients[i], conn->current_room)) {
            if (&clients[i] == conn) {
                send(conn->sockfd, self_msg, strlen(self_msg), 0);
            } else {
                send_to_player(&clients[i], room_msg, strlen(room_msg), 0);
            }
        }
    }
}

void cmd_describe(struct connection *conn, char *args) {
    if (!args) {
        send(conn->sockfd, "Usage: describe <short|long> <description>\n", 45, 0);
        return;
    }
    char *type = strtok(args, " ");
    char *desc = strtok(NULL, "");
    if (!type || !desc) {
        send(conn->sockfd, "Usage: describe <short|long> <description>\n", 45, 0);
        return;
    }
    if (strcmp(type, "short") == 0) {
        strncpy(conn->player->short_description, desc, sizeof(conn->player->short_description) - 1);
        conn->player->short_description[sizeof(conn->player->short_description) - 1] = '\0';
        send(conn->sockfd, "Short description updated.\n", 28, 0);
    } else if (strcmp(type, "long") == 0) {
        strncpy(conn->player->long_description, desc, sizeof(conn->player->long_description) - 1);
        conn->player->long_description[sizeof(conn->player->long_description) - 1] = '\0';
        send(conn->sockfd, "Long description updated.\n", 27, 0);
    } else {
        send(conn->sockfd, "Invalid description type. Use 'short' or 'long'.\n", 53, 0);
    }
}

void broadcast_action(struct connection *conn, const char *self_msg, const char *room_msg) {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (in_room(&clients[i], conn->current_room)) {
            if (&clients[i] == conn) {
                send(conn->sockfd, self_msg, strlen(self_msg), 0);
            } else {
                send_to_player(&clients[i], room_msg, strlen(room_msg), 0);
            }
        }
    }
}

struct status_action {
    const char *command;
    enum status new_status;
    const char *self_msg;
    const char *room_fmt;
};

struct status_action status_actions[] = {
    {"sit",   STATUS_SITTING,  "You sit down.\n",                    "%s sits down.\n"},
    {"stand", STATUS_STANDING, "You stand up.\n",                    "%s stands up.\n"},
    {"sleep", STATUS_SLEEPING, "You lie down and go to sleep.\n",    "%s lies down and goes to sleep.\n"},
    {"lean",  STATUS_LEANING,  "You lean against something.\n",      "%s leans against something.\n"},
    {"wake",  STATUS_STANDING, "You wake up and stand.\n",           "%s wakes up and stands.\n"},
    {NULL, 0, NULL, NULL}
};

void cmd_do_status(struct connection *conn, const char *cmd_name) {
    for (int i = 0; status_actions[i].command; i++) {
        if (strcmp(status_actions[i].command, cmd_name) == 0) {
            conn->player->current_status = status_actions[i].new_status;
            char room_msg[256];
            snprintf(room_msg, sizeof(room_msg), status_actions[i].room_fmt, conn->player->name);
            broadcast_action(conn, status_actions[i].self_msg, room_msg);
            return;
        }
    }
}

void cmd_sit(struct connection *conn, char *args)   { (void)args; cmd_do_status(conn, "sit"); }
void cmd_stand(struct connection *conn, char *args) { (void)args; cmd_do_status(conn, "stand"); }
void cmd_sleep(struct connection *conn, char *args) { (void)args; cmd_do_status(conn, "sleep"); }
void cmd_lean(struct connection *conn, char *args)  { (void)args; cmd_do_status(conn, "lean"); }
void cmd_wake(struct connection *conn, char *args)  { (void)args; cmd_do_status(conn, "wake"); }

struct command player_commands[] = {
    {"describe", cmd_describe, 3},
    {"emote", cmd_emote, 1},
    {"look", cmd_look, 1},
    {"lean", cmd_lean, 1},
    {"move", cmd_move, 2},
    {"quit", cmd_quit, 2},
    {"say", cmd_say, 2},
    {"sit", cmd_sit, 2},
    {"sleep", cmd_sleep, 2},
    {"stand", cmd_stand, 2},
    {"wake", cmd_wake, 2},
    {NULL, NULL, 0}
};

struct command builder_commands[] = {
    {"roomedit", cmd_room_edit, 2},
    {"save", cmd_save, 4},
    {NULL, NULL, 0}
};

struct command admin_commands[] = {
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

    for (int i = 0; conn->available_commands[i].name != NULL; i++) {
        if (len >= conn->available_commands[i].min_length && strncmp(cmd_name, conn->available_commands[i].name, len) == 0) {
            conn->available_commands[i].handler(conn, args);
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