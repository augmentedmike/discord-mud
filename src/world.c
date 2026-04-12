#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

room *room_list;
int next_room_id = 1;

void add_room(room *new_room) {
    if(new_room->id == 0) {
        new_room->id = next_room_id++;
    }
    if (new_room->id >= next_room_id) {
        next_room_id = new_room->id + 1;
    }
    new_room->next = room_list;
    new_room->prev = NULL;

    if (room_list) {
        room_list->prev = new_room;
    }
    room_list = new_room;
}

room * find_room_by_id(int id) {
    room *current = room_list;
    while (current) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

const char *direction_names[] = {
    "north",
    "northeast",
    "east",
    "southeast",
    "south",
    "southwest",
    "west",
    "northwest",
    "up",
    "down"
};

void send_direction_list(int sockfd, const char *prefix) {
    char buffer[256] = "";
    strcat(buffer, prefix);
    for(int i = 0; i < NUM_DIRECTIONS; i++) {
        strcat(buffer, direction_names[i]);
        if (i < NUM_DIRECTIONS - 1) {
            strcat(buffer, ", ");
        }
    }
    strcat(buffer, "\n");
    send(sockfd, buffer, strlen(buffer), 0);
}   

int str_to_direction(const char *str) {
    int len = strlen(str);
    for(int i = 0; i < NUM_DIRECTIONS; i++) {
        if (strncmp(str, direction_names[i], len) == 0) {
            return i;
        }
    }
    return -1;
}

void save_world(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to save world");
        return;
    }

    room *current = room_list;
    while (current) {
        fprintf(file, "ROOM %d\n", current->id);
        fprintf(file, "NAME %s\n", current->name);
        fprintf(file, "DESCRIPTION %s\n", current->description);
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            if (current->exits[i]) {
                fprintf(file, "EXIT %s %d\n", direction_names[i], current->exits[i]->id);
            }
        }
        fprintf(file, "ENDROOM\n");
        current = current->next;
    }

    fclose(file);
}

void load_world(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to load world");
        return;
    }

    char line[2048];
    room *current_room = NULL;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "ROOM ", 5) == 0) {
            current_room = malloc(sizeof(room));
            memset(current_room, 0, sizeof(room));
            current_room->id = atoi(line + 5);
        } else if (strncmp(line, "NAME ", 5) == 0 && current_room) {
            strncpy(current_room->name, line + 5, sizeof(current_room->name)-1);
            current_room->name[strcspn(current_room->name, "\n")] = '\0';
        } else if (strncmp(line, "DESCRIPTION ", 12) == 0 && current_room) {
            strncpy(current_room->description, line + 12, sizeof(current_room->description)-1);
            current_room->description[strcspn(current_room->description, "\n")] = '\0';
        } else if (strncmp(line, "ENDROOM", 7) == 0 && current_room) {
            add_room(current_room);
            current_room = NULL;
        }
    }


    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "ROOM ", 5) == 0) {
            current_room = find_room_by_id(atoi(line + 5));
        } else if (strncmp(line, "EXIT ", 5) == 0 && current_room) {
            char *dir_str = strtok(line + 5, " ");
            char *target_id_str = strtok(NULL, " ");
            if (dir_str && target_id_str) {
                int dir = str_to_direction(dir_str);
                int target_id = atoi(target_id_str);
                room *target_room = find_room_by_id(target_id);
                if (dir != -1 && target_room) {
                    current_room->exits[dir] = target_room;
                }
            }
        }
    }

    fclose(file);
}