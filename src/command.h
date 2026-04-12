#ifndef command_H
#define command_H


struct connection;
typedef void (*cmd_func)(struct connection *conn, char *args);

struct command {
    char *name;
    cmd_func handler;
    int min_length;
};

extern struct command player_commands[];
extern struct command builder_commands[];
extern struct command admin_commands[];

void cmd_look(struct connection *conn, char *args);
void cmd_quit(struct connection *conn, char *args);
void cmd_move(struct connection *conn, char *args);
void cmd_save(struct connection *conn, char *args);

void parse_command(struct connection *conn, char *input);

#endif