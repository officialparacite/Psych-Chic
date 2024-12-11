#define _CRT_SECURE_NO_WARNINGS

// Built_in_library
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// User_defined_library
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

// Other_includes
#include "constants.h"

// Gravity
#define GRAVITY 0.2

// Meteor Objects
#define NUM_OBJECTS 10 

#define INITIAL_OBJECTS 10

// Global SDL Pointers
SDL_Window* window;
SDL_Renderer* renderer;

// Global Variables
int game_is_running = 0;
int is_game_over = 0;
int last_frame_time = 0;
int restart_requested = 0;
int current_level = 1;
int total_objects_to_collect = NUM_OBJECTS;
int collected_objects = 0;
int level_complete = 0;
int to_collect;

// Global textures
SDL_Texture* ball_texture;
SDL_Texture* block_texture;
SDL_Texture* object_texture;
SDL_Texture* background;

// Structs
struct Ball {
    float x, y;        // Position
    float vx, vy;      // Velocity
    float width, height; // Dimensions
}ball;

struct Block {
    float x, y;        // Position
    float vx, vy;      // Velocity
    float width, height; // Dimensions
}block;

struct Floor {
    float x, y;       
    float width, height;
}ground;

struct Meteor {
    float x, y;      
    float vx, vy;
    float width, height; 
}meteor;

typedef struct {
    float x, y;
    float speedY; // Vertical speed
    float gravity;
    int width, height;
    int is_active;
} Object;

// Struct Array for the Object struct
Object objects[NUM_OBJECTS];

// Function Declarations
int initializeWindow();
void setup();  // Initializes the values only once, this is not inside the game loop
void process_input();
void update();
void render();
void destroy_window();
void reset_game();
void increase_level_difficulty();

// Functionalities
bool check_collision(SDL_Rect a, SDL_Rect b);
void initializeObject(Object* obj);
void renderGameOverScreen(SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    
    game_is_running = initializeWindow();
    setup();

    // Game Loop
    while (game_is_running) {
        process_input();

        // If game is over, only render the game over screen
        if (is_game_over) {
            renderGameOverScreen(renderer);

            // Check for restart
            if (restart_requested) {
                reset_game();
            }
        }
        else if (level_complete) {
            // Level completed, continue to next level
            level_complete = 0;
        }
        else {
            update();
            render();
        }
    }

    destroy_window();
    
    return 0;
}

// Function definitions

int initializeWindow() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Error Initializing SDL: %s\n", SDL_GetError());
        return FALSE;
    }

    window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_BORDERLESS
    );
    if (!window) {
        fprintf(stderr, "Error Initializing Window: %s\n", SDL_GetError());
        SDL_Quit();
        return FALSE;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Error Initializing Renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "Error Initializing SDL_image: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    SDL_Surface* bg_surface = IMG_Load("assets/forest_bg.png");
    if (!bg_surface) {
        fprintf(stderr, "Failed to load background image: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    background = SDL_CreateTextureFromSurface(renderer, bg_surface);
    SDL_FreeSurface(bg_surface);
    if (!background) {
        fprintf(stderr, "Failed to create background texture: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf Initialization Error: %s\n", TTF_GetError());
        SDL_DestroyTexture(background);
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    return TRUE;
}

int renderTextWithFont(SDL_Renderer* renderer, const char* fontPath, const char* text, int x, int y, Uint8 r, Uint8 g, Uint8 b, int fontSize) {
    // Open the specified font
    TTF_Font* font = TTF_OpenFont("assets/Pixeltype.ttf", fontSize);
    if (!font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        return -1;
    }

    SDL_Color color = { r, g, b, 255 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        printf("TTF_RenderText_Blended: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        TTF_CloseFont(font);
        return -1;
    }

    // Center the text horizontally
    int centerX = x - (surface->w / 2);
    int centerY = y - (surface->h / 2);

    SDL_Rect dstrect = { centerX, centerY, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);

    // Clean up
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);

    return 0;
}

SDL_Texture* load_texture(const char* file_path, int* width, int* height) {
    SDL_Surface* surface = IMG_Load(file_path);
    if (!surface) {
        fprintf(stderr, "Error loading image: %s\n", IMG_GetError());
        return NULL;
    }

    if (width) *width = surface->w;
    if (height) *height = surface->h;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

// Render Text
int renderText(SDL_Renderer* renderer, const char* text, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
    // Open a larger font to ensure full text visibility
    TTF_Font* font = TTF_OpenFont("assets/Pixeltype.ttf", 36); // Reduced font size from 50
    if (!font) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        return -1;
    }

    SDL_Color color = { r, g, b, 255 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color); // Changed to Blended for better quality
    if (!surface) {
        printf("TTF_RenderText_Blended: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        TTF_CloseFont(font);
        return -1;
    }

    // Center the text horizontally
    int centerX = x - (surface->w / 2);
    int centerY = y - (surface->h / 2);

    SDL_Rect dstrect = { centerX, centerY, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);

    // Clean up
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);

    return 0;
}

void setup() {

    to_collect = 10 + (current_level - 1) * 2;

    // Reset collected objects
    is_game_over = 0;
    collected_objects = 0;
    level_complete = 0;

    // Ball
    ball.x = 500;
    ball.y = 500;
    ball.width = 15;
    ball.height = 15;
    ball.vx = 200;
    ball.vy = 150;

    // Block
    block.width = 90;
    block.height = 40;
    block.vx = 200;
    block.vy = 100;
    block.x = 400;
    block.y = WINDOW_HEIGHT - block.height;

    // Ground
    ground.width = WINDOW_WIDTH;
    ground.height = 20; // Small height for ground
    ground.x = 0;
    ground.y = WINDOW_HEIGHT - ground.height; // Place at bottom of the screen

    // Meteor
    meteor.x = 200;       // Start position
    meteor.y = 0;         // Top of the screen
    meteor.width = 30;    // Width of the meteor
    meteor.height = 30;   // Height of the meteor

    int ball_width, ball_height;
    int block_width, block_height;
    int object_width, object_height;
    
    ball_texture = load_texture("assets/bag.png", &ball_width, &ball_height);
    block_texture = load_texture("assets/chickenidle2.png", &block_width, &block_height);

    object_texture = load_texture("assets/egg.png", &object_width, &object_height);

    // Update block dimensions to match the loaded texture
    ball.width = ball_width;
    ball.height = ball_height;

    // Update block dimensions to match the loaded texture
    block.width = block_width;
    block.height = block_height;

    // Adjust block position to align with the ground
    block.x = 400;
    block.y = WINDOW_HEIGHT - block.height - ground.height; // Ensure it sits just above the ground

    if (current_level == 1) {
        total_objects_to_collect = INITIAL_OBJECTS;
    }
    else {
        total_objects_to_collect = INITIAL_OBJECTS + (current_level * 2);
    }

    // Create an array of objects
    for (int i = 0; i < NUM_OBJECTS; i++) {
        if (i < total_objects_to_collect) {
            initializeObject(&objects[i]);
            objects[i].is_active = 1;

            // Ensure each object has a unique starting position
            // Add some randomness to prevent overlapping
            objects[i].x = rand() % (WINDOW_WIDTH - objects[i].width);
            objects[i].y = -100 - (i * 50); // Stagger the starting heights
        }
        else {
            objects[i].is_active = 0;
        }

        // Update object dimensions to match the loaded texture
        objects[i].width = objects[i].width;
        objects[i].height = objects[i].height;
    }

}
void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            game_is_running = FALSE;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                game_is_running = FALSE;
            }

            // Add restart functionality
            if (is_game_over && event.key.keysym.sym == SDLK_RETURN) {
                restart_requested = 1;
            }
            break;
        }
    }
}


void update() {

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // Calculate the time elapsed since the last frame
    int current_time = SDL_GetTicks();
    float delta_time = (current_time - last_frame_time) / 1000.0f;

    // Update the last frame time for the next frame
    last_frame_time = current_time;

    // Ball / Bag Speed Multiplier
    float ball_speed_multiplier = 1.0f;

    if (key_state[SDL_SCANCODE_SPACE]) {
        ball_speed_multiplier = 3.0f; // Triple the speed when Shift is held
    }

    // Update ball position using delta_time
    ball.x += ball.vx * ball_speed_multiplier * delta_time;
    ball.y -= ball.vy * ball_speed_multiplier * delta_time;

    // Increase block speed by adjusting block.vx
    block.vx = 400.0f;  // Make block move faster (adjust value as needed)

    // Adjust player speed when Shift is pressed
    float speed_multiplier = 1.0f; // Default speed
    if (key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT]) {
        speed_multiplier = 3.0f; // Triple the speed when Shift is held
    }

    // Continuous movement based on key state
    if (key_state[SDL_SCANCODE_A]) {
        block.x -= block.vx * speed_multiplier * delta_time;
    }
    if (key_state[SDL_SCANCODE_D]) {
        block.x += block.vx * speed_multiplier * delta_time;
    }

    // Ensure block / player stays within the window bounds
    if (block.x < 0) {
        block.x = 0;
    }
    if (block.x + block.width > WINDOW_WIDTH) {
        block.x = WINDOW_WIDTH - block.width;
    }

    // Ball boundary checks
    if (ball.x < 0 || ball.x + ball.width > WINDOW_WIDTH) {
        ball.vx *= -1; // Reverse horizontal velocity
        if (ball.x < 0) ball.x = 0;
        if (ball.x + ball.width > WINDOW_WIDTH) ball.x = WINDOW_WIDTH - ball.width;
        /*  Check if the ball's right edge (ball.x + ball.width) exceeds the window's width (WINDOW_WIDTH).
            If it does, reposition the ball so its right edge aligns exactly with the window boundary.
            This prevents the ball from moving outside the visible area of the screen.
        */
    }

    if (ball.y < 0 || ball.y + ball.height > WINDOW_HEIGHT) {
        ball.vy *= -1; // Reverse vertical velocity
        if (ball.y < 0) ball.y = 0;
        if (ball.y + ball.height > WINDOW_HEIGHT) ball.y = WINDOW_HEIGHT - ball.height;
        /*  Check if the ball's top edge (ball.y) or bottom edge (ball.y + ball.height) exceeds the window's boundaries.
            If the top edge goes above 0, reposition the ball so its top edge aligns with the top boundary.
            If the bottom edge exceeds the window height (WINDOW_HEIGHT), reposition the ball so its bottom edge aligns with the bottom boundary.
            This prevents the ball from moving outside the visible area of the screen.
        */
    }

    // Update ball position using delta_time
    //block.x += block.vx * delta_time;
    //block.y -= block.vy * delta_time;

    // Block boundary checks
    if (block.x < 0 || block.x + block.width > WINDOW_WIDTH) {
        //block.vx *= -1; // Reverse horizontal velocity
        if (block.x < 0) block.x = 0;
        if (block.x + block.width > WINDOW_WIDTH) block.x = WINDOW_WIDTH - block.width;
    }

    if (block.y < 0 || block.y + block.height > WINDOW_HEIGHT) {
        //block.vy *= -1; // Reverse vertical velocity
        if (block.y < 0) block.y = 0;
        if (block.y + block.height > WINDOW_HEIGHT) block.y = WINDOW_HEIGHT - block.height;
    }

    // Collision detection between the ball and the block
    SDL_Rect ball_rect = { (int)ball.x, (int)ball.y, (int)ball.width, (int)ball.height };
    SDL_Rect block_rect = { (int)block.x, (int)block.y, (int)block.width, (int)block.height };
    SDL_Rect ground_rect = { (int)ground.x, (int)ground.y, (int)ground.width, (int)ground.height };

    if (check_collision(ball_rect, block_rect)) {
        // Determine the collision direction
        float overlapLeft = (ball.x + ball.width) - block.x;
        float overlapRight = (block.x + block.width) - ball.x;
        float overlapTop = (ball.y + ball.height) - block.y;
        float overlapBottom = (block.y + block.height) - ball.y;

        // Find the smallest overlap to determine the side of the collision
        float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
        float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

        if (minOverlapX < minOverlapY) {
            // Horizontal collision
            ball.vx *= -1; // Reverse horizontal velocity
            if (overlapLeft < overlapRight) {
                ball.x = block.x - ball.width; // Reposition to the left of the block
            }
            else {
                ball.x = block.x + block.width; // Reposition to the right of the block
            }
        }
        else {
            // Vertical collision
            ball.vy *= -1; // Reverse vertical velocity
            if (overlapTop < overlapBottom) {
                ball.y = block.y - ball.height; // Reposition above the block
            }
            else {
                ball.y = block.y + block.height; // Reposition below the block
            }
        }
    }
    if (check_collision(ball_rect, ground_rect)) {
        is_game_over = 1;
        //ball.vy *= -1; // Reverse vertical velocity
        ball.y = ground.y - ball.height; // Align the ball above the ground
    }

    // Meteor Gravity

    meteor.vy += GRAVITY;
    meteor.y += meteor.vy;

    if (meteor.y + meteor.height > WINDOW_HEIGHT - ground.height) {
        meteor.y = WINDOW_HEIGHT - meteor.height - ground.height;
        meteor.vy = 0; // Stop movement on hitting the ground
    }

    //printf("Total Objects: %d\n", total_objects_to_collect); 

    // Not able to fix the issue with total_objects_to_collect variable that kept crashing the program so introduced some patching techniques and a new variable called to_collect instead

    if (total_objects_to_collect > NUM_OBJECTS) {
        total_objects_to_collect = NUM_OBJECTS;
    }

    //total_objects_to_collect = 10 + (current_level * 2);

    // Update objects and check collisions
    for (int i = 0; i < total_objects_to_collect; i++) {
        if (!objects[i].is_active) continue;

        objects[i].speedY += objects[i].gravity;
        objects[i].y += objects[i].speedY;

        // Collision with ball
        SDL_Rect object_rect = { (int)objects[i].x, (int)objects[i].y, (int)objects[i].width, (int)objects[i].height };
        //SDL_Rect ball_rect = { (int)ball.x, (int)ball.y, (int)ball.width, (int)ball.height };

        if (check_collision(ball_rect, object_rect)) {
            objects[i].is_active = 0; // Make the object disappear
            collected_objects++; // Increment collected objects
        }

        if (check_collision(block_rect, object_rect)) {
            if (objects[i].is_active) {  // Only end game for active objects
                is_game_over = 1;
                printf("Game over due to block-object collision\n");
            }
        }

        // Reset object if it hits the ground
        if (objects[i].y + objects[i].height > ground.y) {
            initializeObject(&objects[i]);
            objects[i].is_active = 1;
        }
    }

    if (is_game_over) {
        printf("Game Over\n");
        return;
        //renderGameOverScreen(renderer);
        //game_is_running = 0; // End the game loop
    }

    if (collected_objects >= to_collect) {
        is_game_over = 0;
        current_level++;
        level_complete = 1;

        // Increase difficulty
        increase_level_difficulty();

        // Reset game state for next level
        setup();
        printf("Collected Objects: %d, Total Objects: %d\n", collected_objects, total_objects_to_collect);
        printf("Level completed! Moving to Level %d\n", current_level);
        return;
    }

    // Forced object generation if not enough objects are active
    int active_objects_count = 0;
    for (int i = 0; i < total_objects_to_collect; i++) {
        if (objects[i].is_active) {
            active_objects_count++;
        }
    }

    // If not enough active objects, generate more
    if (active_objects_count < total_objects_to_collect) {
        for (int i = 0; i < total_objects_to_collect; i++) {
            if (!objects[i].is_active) {
                initializeObject(&objects[i]);
                objects[i].is_active = 1;

                // Ensure unique positioning
                objects[i].x = rand() % (WINDOW_WIDTH - objects[i].width);
                objects[i].y = -100 - (i * 50);
            }
        }
    }

    // Cap the frame rate using SDL_Delay
    int delay_time = (last_frame_time + FRAME_TARGET_TIME) - SDL_GetTicks();
    if (delay_time > 0) {
        SDL_Delay(delay_time);
    }
}


void render() {
    //SDL_SetRenderDrawColor(renderer, 0, 75, 35, 255);
    //SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, background, NULL, NULL);

    // Draw a ball rect
    SDL_Rect ball_rect = {
        (int)ball.x,
        (int)ball.y,
        (int)ball.width,
        (int)ball.height
    };
    SDL_RenderCopy(renderer, ball_texture, NULL, &ball_rect);
    //SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    //SDL_RenderFillRect(renderer, &ball_rect); 

    // Draw a block rect
    SDL_Rect block_rect = {
        (int)block.x,
        (int)block.y,
        (int)block.width,
        (int)block.height
    };
    SDL_RenderCopy(renderer, block_texture, NULL, &block_rect);
    //SDL_SetRenderDrawColor(renderer, 145, 32, 22, 255);
    //SDL_RenderFillRect(renderer, &block_rect);

    // Draw Ground Rect
    SDL_Rect ground_rect = {
        (int)ground.x,
        (int)ground.y,
        (int)ground.width,
        (int)ground.height
    };

    SDL_SetRenderDrawColor(renderer, 135, 42, 32, 255);
    SDL_RenderFillRect(renderer, &ground_rect);

    for (int i = 0; i < NUM_OBJECTS; i++) {
        if(objects[i].is_active){
        SDL_Rect obj_rect = {
            (int)objects[i].x,
            (int)objects[i].y,
            objects[i].width,
            objects[i].height
        };
        SDL_RenderCopy(renderer, object_texture, NULL, &obj_rect);
        //SDL_RenderFillRect(renderer, &obj_rect);
        }
    }

    // Render level and collected objects information
    char level_text[50];
    char objects_text[50];

    snprintf(level_text, sizeof(level_text), "LEVEL: %d", current_level);
    snprintf(objects_text, sizeof(objects_text), "EGGS: %d/%d", collected_objects, to_collect);

    renderTextWithFont(
        renderer,
        "assets/Pixeltype.ttf",
        level_text,
        90, 50, 255, 255, 255, 24
    );

    renderTextWithFont(
        renderer,
        "assets/Pixeltype.ttf",
        objects_text,
        100, 80, 255, 255, 255, 24
    );

    SDL_RenderPresent(renderer); // For Buffer Swap
}

void destroy_window() {
    SDL_DestroyTexture(ball_texture);
    SDL_DestroyTexture(block_texture);
    SDL_DestroyTexture(object_texture);
    SDL_DestroyTexture(background);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

/* Functionalities */

bool check_collision(SDL_Rect a, SDL_Rect b) {
    // Check if there is a gap between the two rectangles
    if (a.x + a.w <= b.x || // a is to the left of b
        a.x >= b.x + b.w || // a is to the right of b
        a.y + a.h <= b.y || // a is above b
        a.y >= b.y + b.h) { // a is below b
        return false; // No collision
    }
    return true; // Collision detected
}

// Function to initialize an object
void initializeObject(Object* obj) {
    // Ensure obj is not NULL
    if (obj == NULL) {
        fprintf(stderr, "Error: Trying to initialize NULL object\n");
        return;
    }

    // Add bounds checking
    obj->x = rand() % (WINDOW_WIDTH - 20); // Ensure width is not exceeded
    obj->y = -100;
    obj->speedY = 0;
    obj->gravity = (float)(rand() % 5 + 1) / 50.0f;

    // Ensure consistent object dimensions
    obj->width = 20;
    obj->height = 20;
    obj->is_active = 1;
}

// Game Over Screen

void renderGameOverScreen(SDL_Renderer* renderer) {
    SDL_RenderCopy(renderer, background, NULL, NULL);

    char game_over_text[100];
    snprintf(game_over_text, sizeof(game_over_text), "GAME OVER - LEVEL %d", current_level);

    // Game Over text with original font
    renderTextWithFont(
        renderer,
        "assets/Pixeltype.ttf",
        game_over_text,
        WINDOW_WIDTH / 2,
        WINDOW_HEIGHT / 2 - 100,
        255, 255, 255,
        36
    );

    // Restart instructions with a different font
    renderTextWithFont(
        renderer,
        "C:\\Users\\Paracite\\Desktop\\C Projects\\Project1\\Project1\\Minecraft.ttf",
        "PRESS ENTER TO RESTART",
        WINDOW_WIDTH / 2,
        WINDOW_HEIGHT / 2 + 50,
        200, 200, 200,
        24
    );

    SDL_RenderPresent(renderer);
}

// Add a reset function to reinitialize game state
void reset_game() {
    // Reset level and object collection
    current_level = 1;
    total_objects_to_collect = NUM_OBJECTS;
    collected_objects = 0;
    to_collect = 10;

    // Reset ball position and velocity
    ball.x = 500;
    ball.y = 500;
    ball.vx = 200;
    ball.vy = 150;

    // Reset block position
    block.x = 400;
    block.y = WINDOW_HEIGHT - block.height - ground.height;

    // Reset meteor
    meteor.x = 200;
    meteor.y = 0;
    meteor.vy = 0;

    // Reset objects
    for (int i = 0; i < total_objects_to_collect; i++) {
        initializeObject(&objects[i]);
    }

    // Reset game state flags
    is_game_over = 0;
    restart_requested = 0;
    level_complete = 0;
}

void increase_level_difficulty() {
    // Explicitly calculate objects based on current level
    total_objects_to_collect = 10 + (current_level * 2);

    // Cap the maximum objects if needed
    if (total_objects_to_collect > 20) {
        total_objects_to_collect = 20;
    }

    // Reset collected objects
    collected_objects = 0;

    // Reinitialize only the active objects needed for this level
    for (int i = 0; i < NUM_OBJECTS; i++) {
        if (i < total_objects_to_collect) {
            initializeObject(&objects[i]);
            objects[i].is_active = 1;

            // Stagger starting heights
            objects[i].x = rand() % (WINDOW_WIDTH - objects[i].width);
            objects[i].y = -100 - (i * 50);

            // Increase difficulty by increasing gravity more gradually
            objects[i].gravity *= 1.1f;
        }
        else {
            objects[i].is_active = 0;
        }
    }

    // Gradually increase speeds
    ball.vx *= 1.1f;
    ball.vy *= 1.1f;
    block.vx *= 1.1f;

    meteor.vy *= 1.1f;

    printf("Level difficulty increased. New level: %d, Total objects: %d\n", current_level, total_objects_to_collect);
}