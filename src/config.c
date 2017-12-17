
#include "config.h"
#include "nvm.h"

unsigned int led_count = 0;
unsigned int brightness = 4;
unsigned int program = 0;

typedef struct {
    const char* name;
    int value;
    int min;
    int max;
} config_t;

config_t configs[] = {
    {"LEDS", 0, 0, 300},
};

char* config_read_line(char* ptr)
{
    char* name[16];
}


void config_load()
{
    char* cfg = get_file_ptr(7);


}