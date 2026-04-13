#ifndef status_H
#define status_H

enum status {
    STATUS_STANDING,
    STATUS_SITTING,
    STATUS_LEANING,
    STATUS_SLEEPING
};

static const char *status_names[] = {
    "standing",
    "sitting",
    "leaning",
    "sleeping"
};

#endif