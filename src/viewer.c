#include "hdl-core/hdl.h"
#include <stdio.h>
// SDL required for testing
#include <SDL2/SDL.h>
// Standard library for compiling
#include <stdlib.h>

// Multiply the size of a pixel
#define TEST_MULTISAMPLING 3

SDL_Window *sdl_window;
SDL_Renderer *sdl_renderer;

struct HDL_Interface hdl_interface;

void clearScreen (uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

void hLine (uint16_t x, uint16_t y, int16_t len) {
    SDL_RenderDrawLine(sdl_renderer, x * TEST_MULTISAMPLING, y * TEST_MULTISAMPLING, (x + len) * TEST_MULTISAMPLING, y * TEST_MULTISAMPLING);
}

void vLine (uint16_t x, uint16_t y, int16_t len) {
    SDL_RenderDrawLine(sdl_renderer, x * TEST_MULTISAMPLING, y * TEST_MULTISAMPLING, x * TEST_MULTISAMPLING, (y + len) * TEST_MULTISAMPLING);
}

void pixel (uint16_t x, uint16_t y) {
    SDL_Rect rect = {
        .x = x * TEST_MULTISAMPLING,
        .y = y * TEST_MULTISAMPLING,
        .w = TEST_MULTISAMPLING,
        .h = TEST_MULTISAMPLING
    };
    SDL_RenderFillRect(sdl_renderer, &rect);
}

extern const char hdl_font[464];

void text (uint16_t x, uint16_t y, const char *text, uint8_t fontSize) {

    int len = strlen(text);
    int line = 0;
    int acol = 0;

    for (int g = 0; g < len; g++) {
		// Starting character in single quotes
		int off = (text[g] - '!') * 8;

        if (text[g] == '\n') {
			line++;
			acol = 0;
			continue;
		}
		else if (text[g] == ' ') {
			acol++;
		}

		if (off < 0 && off > sizeof(hdl_font) / 8)
			continue;

		
		for (int py = 0; py < 8; py++) {
			for (int px = 0; px < 8; px++) {
				if ((hdl_font[off + py] >> (7 - px)) & 1) {
                    SDL_Rect rect = {
                        .x = (x + (px + acol * 10) * fontSize) * TEST_MULTISAMPLING,
                        .y = (y + (py + line * 10) * fontSize) * TEST_MULTISAMPLING,
                        .w = fontSize * TEST_MULTISAMPLING,
                        .h = fontSize * TEST_MULTISAMPLING
                    };
                    SDL_RenderFillRect(sdl_renderer, &rect);
				}
			}
		}
		acol++;

    }
}

void renderScreen () {
    SDL_RenderPresent(sdl_renderer);
}

void printHelp () {
    printf("HDL-VIEWER - HDL Viewer\r\n");
    printf("Usage: \r\n");
    printf("\thdl-viewer [options] <file>\r\n");
    printf("Options:\r\n");
    printf("\t-h\t\tPrint this help\r\n");
}

int time_hours = 19;
int time_minutes = 28;
uint8_t hide_battery = 0;

int buildPage (const char *iFile, struct HDL_Interface *interface) {

    char *extension = NULL;
    char *inputFile = (char*)iFile;
    int i;
    for(i = strlen(inputFile) - 1; i > 0; i--) {
        if(inputFile[i] == '.') {
            extension = (char*)inputFile + i;
            break;
        }
    }

    // Only check extension, if none, .bin assumed
    if(extension != NULL) {
        if(strcmp(extension, ".bin") == 0) {

        }
        else if(strcmp(extension, ".hdl") == 0) {
            // Compile first
            char cmdbuff[256];
            sprintf(cmdbuff, "hdl-cmp -o /tmp/hdl-output.bin %s", inputFile);
            int err = system(cmdbuff);
            if(err) {
                printf("Error: Failed to compile .hdl file\r\n");
                return 1;
            } 
            printf("Compiled temporary file to /tmp/hdl-output.bin\r\n");
            inputFile = "/tmp/hdl-output.bin";
        }
        else {
            printf("Error: Unknown extension '%s'\r\n", extension);
            return 1;
        }

    }

    FILE *file = fopen(inputFile, "r");
    if(file == NULL) {
        printf("Error: Could not open '%s'\r\n", inputFile);
        return 1;
    }
    // Find file length
    int len = 0;
    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *pageBuffer = malloc(len);
    if(file == NULL) {
        printf("Error: Out of memory\r\n");
        return 1;
    }

    fread(pageBuffer, 1, len, file);

    // Free interface first
    HDL_Free(interface);

    int err = HDL_Build(interface, pageBuffer, len);

    free(pageBuffer);
    fclose(file);

    if(err) {
        printf("Failed to build page\r\n");
        return 1;
    }
    else {
        printf("Built page '%s'\r\n", inputFile);

        HDL_Update(interface);
    }

    return 0;
}

int main (int argc, char *argv[]) {

    if(argc < 2) {
        printf("Error: Expected an HDL output file\r\n");
        return 1;
    }

    printf("**HDL-VIEWER**\r\n* Press 'r' to reload\r\n");

    char *filename = NULL;

    /*
        0: expect file or option
        
    */
    uint8_t arg_state = 0;
    for(int i = 1; i < argc; i++) {
        switch(arg_state) {
            case 0:
            {
                // Expect file or option
                if(argv[i][0] == '-') {
                    // Option
                    switch(argv[i][1]) {
                        case 'h':
                        {
                            // Print help
                            printHelp();
                            return 0;
                        }
                    }
                }
                else {
                    // File
                    if(filename != NULL) {
                        printf("Error: Display expects only single input file\r\n");
                        return 1;
                    }
                    else {
                        filename = argv[i];
                    }
                }
                break;
            }
        }
    }

    if(filename == NULL) {
        printf("Error: Input file expected\r\n");

        return 1;
    }


    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    sdl_window = SDL_CreateWindow("HDL VIEWER",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       80 * TEST_MULTISAMPLING, 128 * TEST_MULTISAMPLING, 0);

    sdl_renderer = SDL_CreateRenderer(sdl_window, 0, 0);

    // Initialize HDL
    hdl_interface = HDL_CreateInterface(80, 128, HDL_COLORS_MONO, 0);

    hdl_interface.f_clear = clearScreen;
    hdl_interface.f_render = renderScreen;
    hdl_interface.f_vline = vLine;
    hdl_interface.f_hline = hLine;
    hdl_interface.f_text = text;
    hdl_interface.f_pixel = pixel;

    HDL_SetBinding(&hdl_interface, "HOURS", 1, &time_hours);
    HDL_SetBinding(&hdl_interface, "MINS", 2, &time_minutes);
    HDL_SetBinding(&hdl_interface, "HIDEBAT", 3, &hide_battery);


    buildPage(filename, &hdl_interface);

    SDL_Event ev;
    int close = 0;
    while(!close) {
        while(SDL_PollEvent(&ev)) {
            switch(ev.type) {
                case SDL_QUIT:
                    close = 1;
                    break;
                case SDL_KEYDOWN:
                {
                    switch(ev.key.keysym.sym) {
                        case SDLK_r:
                        {
                            printf("Reloading %s...\r\n", filename);
                            buildPage(filename, &hdl_interface);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    HDL_Free(&hdl_interface);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}