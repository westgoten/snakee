#include <SDL.h>
#include <time.h>

// Colors
#define RED 204, 0, 0, SDL_ALPHA_OPAQUE
#define BROWN 153, 153, 102, SDL_ALPHA_OPAQUE
#define ORANGE 255, 153, 0, SDL_ALPHA_OPAQUE
#define BROWN_ISH 179, 60, 0, SDL_ALPHA_OPAQUE

#define FPS 15

// Store window dimensions (fullscreen)
int WINDOW_WIDTH = 0;
int WINDOW_HEIGHT = 0;

typedef enum Direction {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	NONE
} Dir;

typedef struct Player {
	SDL_Rect head;
	SDL_Rect *body;
	int length;
	int gap;
	int vel;
} Snake;

typedef struct Food {
	SDL_Rect rect;
	int count;
} Food;

SDL_Rect walls[4] = {};

void initialize_snake(Snake *coral, int grid_size[2]);

void move_body(Snake *coral);

void draw_snake(SDL_Renderer *rend, Snake *coral);

SDL_bool has_collided(Snake *coral);

void placing_food(Food *orange, Snake *coral, int grid_size[2]);

void eat_and_grow(Snake *coral, Food *orange, int grid_size[2]);

void initialize_walls(Snake *coral);

void draw_walls(SDL_Renderer *rend);

int main(int argc, char *argv[]) {
	// Initialize video and handle errors
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	// Create window and handle errors
	SDL_Window *window = NULL;
	window = SDL_CreateWindow("Snakee",
							  SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED,
							  WINDOW_WIDTH,
							  WINDOW_HEIGHT,
							  SDL_WINDOW_FULLSCREEN);
	if (!window) {
		SDL_Log("Unable to create window: %s", SDL_GetError());
		SDL_DestroyWindow(window);
		return 1;
	}

	SDL_GetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);

	// Create renderer and handle errors
	SDL_Renderer *rend = NULL;
	rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (!rend) {
		SDL_Log("Unable to create renderer: %s", SDL_GetError());
		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(window);
		return 1;
	}

	// Setup snake and food dimensions and positions

	Snake coral;
	int grid_size[2] = {}; // Stores screen's grid size in number of blocks
	initialize_snake(&coral, grid_size);

	Food orange;
	orange.count = 0;
	orange.rect.w = coral.head.w;
	orange.rect.h = coral.head.h;
	srand(time(NULL));
	placing_food(&orange, &coral, grid_size);

	// Initial snake's direction
	Dir chosen_dir = NONE, actual_dir = RIGHT;

	SDL_bool running = SDL_TRUE;
	SDL_Event event;
	// Game loop
	while (running) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					running = SDL_FALSE;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.scancode) {
						case SDL_SCANCODE_UP:
							chosen_dir = UP;
							break;
						case SDL_SCANCODE_DOWN:
							chosen_dir = DOWN;
							break;
						case SDL_SCANCODE_LEFT:
							chosen_dir = LEFT;
							break;
						case SDL_SCANCODE_RIGHT:
							chosen_dir = RIGHT;
							break;
						case SDL_SCANCODE_ESCAPE:
							running = SDL_FALSE;
							break;
						default:
							chosen_dir = NONE;
					}
					break;
			}
		}

		// Update snake movement direction
		if (chosen_dir == UP && actual_dir != DOWN) {
			actual_dir = UP;
		}
		if (chosen_dir == DOWN && actual_dir != UP) {
			actual_dir = DOWN;
		}
		if (chosen_dir == LEFT && actual_dir != RIGHT) {
			actual_dir = LEFT;
		}
		if (chosen_dir == RIGHT && actual_dir != LEFT) {
			actual_dir = RIGHT;
		}

		// Move the snake

		move_body(&coral);

		switch (actual_dir) {
			case UP:
				coral.head.y -= coral.vel;
				break;
			case DOWN:
				coral.head.y += coral.vel;
				break;
			case LEFT:
				coral.head.x -= coral.vel;
				break;
			case RIGHT:
				coral.head.x += coral.vel;
				break;
		}

		// Interaction between the snake and the food
		eat_and_grow(&coral, &orange, grid_size);

		// Detect if the snake's head collided with its body or
		// the boundaries
		if (has_collided(&coral))
			running = SDL_FALSE;

		// Draw the background
		SDL_SetRenderDrawColor(rend, BROWN);
		SDL_RenderClear(rend);

		// Draw sprites
		SDL_SetRenderDrawColor(rend, ORANGE);
		SDL_RenderFillRect(rend, &orange.rect);

		draw_snake(rend, &coral);

		// Draw boundaries
		draw_walls(rend);

		// Update screen
		SDL_RenderPresent(rend);

		// Set frame rate
		SDL_Delay(1000/FPS);
	}

	// Clean up and exits the program

	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void initialize_snake(Snake *coral, int grid_size[2]) {
	coral->gap = (int)(WINDOW_WIDTH / 225.0f);
	coral->length = 10;

	coral->body = malloc(coral->length * sizeof(SDL_Rect));

	coral->vel = (int)(WINDOW_WIDTH / 30.0f); // Grid block size
	coral->head.w = coral->vel - coral->gap;
	coral->head.h = coral->head.w;

	grid_size[0] = WINDOW_WIDTH / coral->vel;  // Number of blocks in grid's x-axis
	grid_size[1] = WINDOW_HEIGHT / coral->vel; // Number of blocks in grid's y-axis

	initialize_walls(coral);

	// Put snake's head on the middle of the screen
	// + (coral->gap / 2) places the head on the center of a grid block
	// + walls[0].w and walls[2].h move away the head from the wall
	coral->head.x = (grid_size[0] / 2) * coral->vel + (coral->gap / 2) + walls[0].w;
	coral->head.y = (grid_size[1] / 2) * coral->vel + (coral->gap / 2) + walls[2].h;

	// Put the first body piece behind the head
	coral->body[0].w = coral->head.w;
	coral->body[0].h = coral->head.h;
	coral->body[0].x = coral->head.x - coral->body[0].w - coral->gap;
	coral->body[0].y = coral->head.y;

	// Put the remaining body pieces one behind the other
	int i;
	for (i = 1; i < coral->length; i++) {
		coral->body[i].w = coral->head.w;
		coral->body[i].h = coral->head.h;
		coral->body[i].x = coral->body[i-1].x - coral->body[i].w - coral->gap;
		coral->body[i].y = coral->head.y;
	}
}

void move_body(Snake *coral) {
	// To move the snake, each body piece needs to take the position of
	// the next

	int i;
	for (i = coral->length - 1; i > 0; i--) {
		coral->body[i].x = coral->body[i-1].x;
		coral->body[i].y = coral->body[i-1].y;
	}

	coral->body[0].x = coral->head.x;
	coral->body[0].y = coral->head.y;

	// The head is controlled by the player. It is the last to move
}

void draw_snake(SDL_Renderer *rend, Snake *coral) {
	SDL_SetRenderDrawColor(rend, RED);
	SDL_RenderFillRect(rend, &coral->head);

	int i;
	for (i = 0; i < coral->length; i++) {
		SDL_RenderFillRect(rend, &coral->body[i]);
	}
}

SDL_bool has_collided(Snake *coral) {
	SDL_bool collided = SDL_FALSE;

	// Check if the snake's head collided with the boundaries
	if (coral->head.x < walls[0].w || coral->head.x > walls[1].x - coral->head.w ||
		coral->head.y < walls[2].h || coral->head.y > walls[3].y - coral->head.h)
		collided = SDL_TRUE;

	// Check if the snake's head collided with its body
	int i;
	for (i = 0; i < coral->length; i++) {
		if (coral->head.x == coral->body[i].x && coral->head.y == coral->body[i].y)
			collided = SDL_TRUE;
	}

	return collided;
}

void placing_food(Food *orange, Snake *coral, int grid_size[2]) {
	int i;
	SDL_bool valid;

	do {
		valid = SDL_TRUE;

		// Get a random position for the food
		orange->rect.x = (rand() % grid_size[0]) * coral->vel + (coral->gap / 2) + walls[0].w;
		orange->rect.y = (rand() % grid_size[1]) * coral->vel + (coral->gap / 2) + walls[2].h;

		// Check if the snake is in that position

		for (i = 0; i < coral->length; i++)
			if (coral->body[i].x == orange->rect.x && coral->body[i].y == orange->rect.y)
				valid = SDL_FALSE;

		if (coral->head.x == orange->rect.x && coral->head.y == orange->rect.y)
			valid = SDL_FALSE;
	} while (!valid);
}

void eat_and_grow(Snake *coral, Food *orange, int grid_size[2]) {
	// Check if the food was eaten
	if (coral->head.x == orange->rect.x && coral->head.y == orange->rect.y) {
		orange->count++;

		// Update the rand() seed every 11 meals.
		// Try to increase randomness
		if (orange->count >= 10) {
			orange->count = 0;
			srand(time(NULL));
		}

		// Add a new body piece at the end of the snake's body

		coral->length++;
		coral->body = realloc(coral->body, coral->length * sizeof(SDL_Rect));

		coral->body[coral->length-1].w = coral->head.w;
		coral->body[coral->length-1].h = coral->head.h;
		coral->body[coral->length-1].x = coral->body[coral->length-2].x;
		coral->body[coral->length-1].y = coral->body[coral->length-2].y;

		// Update food's position
		placing_food(orange, coral, grid_size);
	}
}

void initialize_walls(Snake *coral) {
	// Left wall
	walls[0].w = (WINDOW_WIDTH % coral->vel) / 2;
	walls[0].h = WINDOW_HEIGHT;
	walls[0].x = 0;
	walls[0].y = 0;

	// Right wall
	walls[1].w = walls[0].w;
	walls[1].h = walls[0].h;
	walls[1].x = WINDOW_WIDTH - walls[1].w;
	walls[1].y = walls[0].y;

	// Upper wall
	walls[2].w = WINDOW_WIDTH;
	walls[2].h = (WINDOW_HEIGHT % coral->vel) / 2;
	walls[2].x = 0;
	walls[2].y = 0;

	// Bottom wall
	walls[3].w = walls[2].w;
	walls[3].h = walls[2].h;
	walls[3].x = walls[2].x;
	walls[3].y = WINDOW_HEIGHT - walls[3].h;
}

void draw_walls(SDL_Renderer *rend) {
	SDL_SetRenderDrawColor(rend, BROWN_ISH);
	int i;
	for (i = 0; i < 4; i++)
		SDL_RenderFillRect(rend, &walls[i]);
}
