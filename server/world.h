#ifndef WORLD_H
#define WORLD_H

#define ROOM_NAME_MAX 80
#define ROOM_DESC_MAX 1024

enum direction
{
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST,
    UP,
    DOWN,
    NUM_DIRECTIONS
};


struct room
{
    int id;
    char name[ROOM_NAME_MAX];
    char description[ROOM_DESC_MAX];
    struct room *exits[NUM_DIRECTIONS];
    struct room *next;
    struct room *prev;
};


typedef struct room room;

void add_room(room *new_room);
void send_direction_list(int sockfd, const char *prefix);
void save_world(const char *filename);
void load_world(const char *filename);
int str_to_direction(const char *str);

room * find_room_by_id(int id);
extern room *room_list;
extern int next_room_id;

extern const char *direction_names[];

#endif