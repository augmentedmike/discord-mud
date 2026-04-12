#ifndef player_H
#define player_H

#define ROLE_PLAYER  0
#define ROLE_BUILDER 1
#define ROLE_ADMIN   2

struct player
{
    char name[32];
    char password_hash[128];
    int role;
    int saved_room_id;
};

void hash_password(const char *password, char *hash_out);
void save_player(const char *filename, struct player *player);
void load_player(const char *filename, struct player *player);

#endif