#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "st_lib.h"
#include "st_freq.h"
#include "st_idle.h"
#include "st_output.h"

// Main function: entry point for execution

void help() {
    printf(
        "Usage: program [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -h            Show this help message and exit\n"
        "  -v            Show program version and exit\n"
        "  -i            Show CPU idle stats or modify idle settings\n"
        "  -f            Show CPU frequency stats or modify frequency settings\n"
        "  -c <core>     Select CPU core (default: all cores)\n"
        "  -m            Enable modify mode\n"
        "  -o <mode>     Output format\n"
        "                  h  human-readable (default)\n"
        "                  j  JSON\n"
        "Examples:\n"
        "  idlefreq_stat -i\n"
        "      Show idle information\n"
        "  idlefreq_stat -f -c 2\n"
        "      Show frequency information for core 2\n"
        "  program -i -o json\n"
        "      Show frequency information in JSON format\n"
        "\n"
    );
}

void version() {
    printf("Version: %s\n", RELEASE_NAME);
}

int main(int argc, char *argv[]) {

    // Writing print statement to print hello world
    
    int opt;

    // Defaults
    int idle_mode = false;
    int freq_mode = false;
    int core = -1;
    int modify = false;
    char output_mode = 'h'; // j = json, h = human readable
    
    while ( (opt = getopt(argc, argv, "hvifo:mc:")) != -1 )
    {
        switch (opt) {

            case 'h':
                help();
                return 0;
                break;

            case 'v':
                version();
                return 0;
                break;

            case 'i':
                idle_mode = true;
                break;

            case 'f':
                freq_mode = true;
                break;

            case 'o':
                output_mode = optarg[0];
                break;
            
            case 'c':
                core = atoi(optarg);
                break;

            case 'm':
                modify = true;
                break;

            default:
                fprintf(stderr, "Invalid argument...\n");
                help();
                return -1;


        }
    }

    // restrictions

    // ALLOWED OUTPUT FORMATS
    const char * supported_outputs = ST_SUPPORTED_OUTPUT;
    char * chr_found = strchr(supported_outputs, output_mode); // if suggested output is not found in supported modes MACRO
    if ( ! chr_found )
    {
        fprintf(stderr, "%s %c\n", "Non supported output mode:", output_mode);
        return -1;
    }

    // CURRENT MODE: idle, frequency
    if ( idle_mode )
    {
        return st_idle(core, output_mode, modify);
    }
    else if ( freq_mode )
    {
        return st_freq(core, output_mode, modify);
        
    }
    else
    {
        fprintf(stderr, "%s\n", "Invalid mode...");
    }
    

    return 0;
}