#include "main.h"
#include "command.h"
#include "player.h"
#include <signal.h>
#include <errno.h>


int server_fd;
struct connection clients[MAX_CLIENTS];

int setup_listener(void) {
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);

    printf("Server listening on port %d\n", PORT);
    fflush(stdout);

    return server_fd;
}

void handle_new_connection(int server_fd, struct connection *clients, room *starting_room) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int new_socket;

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("accept failed");
        return;
    }

    int found = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].sockfd = new_socket;
            clients[i].active = 1;
            clients[i].addr = address;
            clients[i].current_room = starting_room;
            clients[i].available_commands = NULL;
            clients[i].player = NULL;
            found = 1;
            send(clients[i].sockfd, "What is your name?\n", 20, 0);
            break;
        }
    }
    if (!found) {
        close(new_socket);
    }
}

void handle_login(struct connection *conn) {
    if (conn->state == STATE_GET_NAME) {
        char name[32];
        strncpy(name, conn->bufferin, sizeof(name)-1);
        name[sizeof(name)-1] = '\0';
        conn->player = malloc(sizeof(struct player));
        memset(conn->player, 0, sizeof(struct player));
        strncpy(conn->player->name, name, sizeof(conn->player->name)-1);
        conn->player->name[sizeof(conn->player->name)-1] = '\0';

        char suppress[] = {255, 251, 1};
        send(conn->sockfd, suppress, 3, 0);
        send(conn->sockfd, "Enter password: ", 16, 0);
        conn->state = STATE_GET_PASSWORD;
    } else if (conn->state == STATE_GET_PASSWORD) {
        char restore[] = {255, 252, 1};
        send(conn->sockfd, restore, 3, 0);
        send(conn->sockfd, "\n", 1, 0);

        char password[128];
        strncpy(password, conn->bufferin, sizeof(password)-1);
        password[sizeof(password)-1] = '\0';
        
        char filepath[64];
        snprintf(filepath, sizeof(filepath), "players/%s.dat", conn->player->name);
        FILE *file = fopen(filepath, "r");
        if (!file) {
            hash_password(password, conn->player->password_hash);
            conn->player->role = ROLE_PLAYER;
            conn->player->saved_room_id = 1;
            save_player(filepath, conn->player);
            send(conn->sockfd, "New player created!\n", 21, 0);
        } else {
            load_player(filepath, conn->player);
            fclose(file);
            char check_hash[128];
            hash_password(password, check_hash);
            if (strcmp(check_hash, conn->player->password_hash) != 0) {
                send(conn->sockfd, "Invalid password. Connection closing.\n", 38, 0);
                cleanup_connection(conn);
                close(conn->sockfd);
                conn->active = 0;
                return;
            }



            char welcome_msg[64];
            snprintf(welcome_msg, sizeof(welcome_msg), "Welcome back, %s!\n\n\n", conn->player->name);
            send(conn->sockfd, welcome_msg, strlen(welcome_msg), 0);
        }


        // assemble the available commands based on role - admin has all 3, builder has 2, and player has 1
        // need to allocate a new array and copy the appropriate commands over
        int num_player_cmds, num_builder_cmds, num_admin_cmds, total_cmds = 0;
        for(num_player_cmds = 0; player_commands[num_player_cmds].name != NULL; num_player_cmds++);
        for(num_builder_cmds = 0; builder_commands[num_builder_cmds].name != NULL; num_builder_cmds++);
        for(num_admin_cmds = 0; admin_commands[num_admin_cmds].name != NULL; num_admin_cmds++);
        if(conn->player->role >= ROLE_PLAYER) total_cmds += num_player_cmds;
        if(conn->player->role >= ROLE_BUILDER) total_cmds += num_builder_cmds;
        if(conn->player->role >= ROLE_ADMIN) total_cmds += num_admin_cmds;
        conn->available_commands = malloc(sizeof(struct command) * (total_cmds + 1));
        int cmd_index = 0;
        if(conn->player->role >= ROLE_PLAYER) {
            for(int i = 0; i < num_player_cmds; i++) {
                conn->available_commands[cmd_index++] = player_commands[i];
            }
        }
        if(conn->player->role >= ROLE_BUILDER) {
            for(int i = 0; i < num_builder_cmds; i++) {
                conn->available_commands[cmd_index++] = builder_commands[i];
            }
        }
        if(conn->player->role >= ROLE_ADMIN) {
            for(int i = 0; i < num_admin_cmds; i++) {
                conn->available_commands[cmd_index++] = admin_commands[i];
            }
        }
        conn->available_commands[cmd_index].name = NULL; // null terminate the command list

        conn->state = STATE_PLAYING;
        cmd_look(conn, NULL);
    }
}

void cleanup_connection(struct connection *conn) {
    free(conn->player);
    conn->state = STATE_GET_NAME;
    if(conn->available_commands) {
        free(conn->available_commands);
    }
}

void handle_client_input(struct connection *clients, fd_set *readfds) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && FD_ISSET(clients[i].sockfd, readfds)) {
            int valread = read(clients[i].sockfd, clients[i].bufferin, BUFFER_SIZE);
            if (valread <= 0) {
                close(clients[i].sockfd);
                cleanup_connection(&clients[i]);
                clients[i].active = 0;
            } else {
                clients[i].bufferin[valread] = '\0';
                char *newline = strchr(clients[i].bufferin, '\r');
                if (newline) *newline = '\0';
                newline = strchr(clients[i].bufferin, '\n');
                if (newline) *newline = '\0';

                // deal with telnet negotiation
                int is_telnet_command = 0;
                for (int j = 0; j < valread; j++) {
                    if ((unsigned char)clients[i].bufferin[j] == 255) {
                        is_telnet_command = 1;
                        break;
                    }
                }
                if (is_telnet_command) continue;
                if (strlen(clients[i].bufferin) == 0) continue;

                if (clients[i].state == STATE_PLAYING) {
                    parse_command(&clients[i], clients[i].bufferin);
                } else {
                    handle_login(&clients[i]);
                }
            }
        }
    }
}

int build_fd_set(fd_set *readfds, int server_fd, struct connection *clients) {
    int max_sd = server_fd;

    FD_ZERO(readfds);
    FD_SET(server_fd, readfds);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            FD_SET(clients[i].sockfd, readfds);
            if (clients[i].sockfd > max_sd) {
                max_sd = clients[i].sockfd;
            }
        }
    }

    return max_sd;
}

void signal_handler(int signum) {
    printf("Caught signal %d, shutting down...\n", signum);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].sockfd);
        }
    }
    save_world("world.dat");
    close(server_fd);
    exit(EXIT_SUCCESS);
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    memset(clients, 0, sizeof(clients));

    server_fd = setup_listener();

    load_world("world.dat");

    room *starting_room = find_room_by_id(1);
    if(!starting_room) {
        starting_room = malloc(sizeof(room)); 
        memset(starting_room, 0, sizeof(room));
        starting_room->id = 1;
        strcpy(starting_room->name, "The Void");
        strcpy(starting_room->description, "You see nothing but a swirling mist. There is no determinate floor, ceiling, or walls.");
        add_room(starting_room);
    }

    while (1) {
        fd_set readfds;
        int max_sd = build_fd_set(&readfds, server_fd, clients);

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds))
            handle_new_connection(server_fd, clients, starting_room);

        handle_client_input(clients, &readfds);
    }
}
