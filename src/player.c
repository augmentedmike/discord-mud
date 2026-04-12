#include <stdio.h>
#include <stdlib.h>
#include <CommonCrypto/CommonDigest.h>
#include <string.h>

#include "player.h"


void hash_password(const char *password, char *hash_out) {
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(password, strlen(password), hash);
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_out + (i * 2), "%02x", hash[i]);
    }
    hash_out[CC_SHA256_DIGEST_LENGTH * 2] = '\0';
}

void save_player(const char *filename, struct player *player) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to save player");
        return;
    }
    fprintf(file, "NAME %s\n", player->name);
    fprintf(file, "PASSWORD_HASH %s\n", player->password_hash);
    fprintf(file, "ROLE %d\n", player->role);
    fprintf(file, "SAVED_ROOM_ID %d\n", player->saved_room_id);
    fclose(file);
}

void load_player(const char *filename, struct player *player) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to load player");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, " ");
        char *value = strtok(NULL, "\n");
        if (strcmp(key, "NAME") == 0) {
            strncpy(player->name, value, sizeof(player->name)-1);
            player->name[sizeof(player->name)-1] = '\0';
        } else if (strcmp(key, "PASSWORD_HASH") == 0) {
            strncpy(player->password_hash, value, sizeof(player->password_hash)-1);
            player->password_hash[sizeof(player->password_hash)-1] = '\0';
        } else if (strcmp(key, "ROLE") == 0) {
            player->role = atoi(value);
        } else if (strcmp(key, "SAVED_ROOM_ID") == 0) {
            player->saved_room_id = atoi(value);
        }
    }
    fclose(file);
}