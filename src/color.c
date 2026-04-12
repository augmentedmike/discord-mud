#include "color.h"

void colorize(char *out, size_t size, const char *color, const char *text) {
    snprintf(out, size, "%s%s%s", color, text, COLOR_RESET);
}