#include <stdio.h>
#include "raylib.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define PLAYER_SPEED 200
#define PLAYER_MAX_LIVES 3
#define PLAYER_MAX_SCORE 9999
#define PROJECTILE_SPEED 600
#define PROJECTILE_OFFSET_FROM_PLAYER 10
#define PROJECTILE_EXPLOSION_DURATION 0.5 // seconds
#define ENEMIES_COLS 1
#define ENEMIES_ROWS 1
#define MAX_NUM_OF_ENEMIES ENEMIES_ROWS * ENEMIES_COLS + 1
#define SPACE_BETWEEN_ENEMIES 22
#define ENEMY_HORIZONTAL_SPEED 150
#define ENEMY_VERTICAL_SPEED 500
#define ENEMY_DYING_DURATION 0.5

typedef enum EntityStateValue {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_MOVING,
    PLAYER_STATE_FIRING,
    PROJECTILE_STATE_ACTIVE,
    PROJECTILE_STATE_EXPLODING,
    PROJECTILE_STATE_INACTIVE,
    ENEMY_STATE_ACTIVE,
    ENEMY_STATE_DYING,
    ENEMY_STATE_DEAD,
} EntityStateValue;

typedef enum MovementDirectionValue {
    MOVE_RIGHT,
    MOVE_DOWN,
    MOVE_LEFT,
} MovementDirectionValue;

typedef struct EntityState {
    EntityStateValue value;
    double startTime;
    double elapsedTime;
} EntityState;

typedef struct Projectile {
    Rectangle body;
    EntityState state;
} Projectile;

typedef struct Player {
    Rectangle body;
    Projectile projectile;
    EntityState state;
    int lives;
    int score;
} Player;

typedef struct Enemy {
    Rectangle body;
    EntityState state;
    MovementDirectionValue dir;
} Enemy;

typedef struct Game {
    Player player;
    Enemy enemies[MAX_NUM_OF_ENEMIES];
    Rectangle map;
    Rectangle playerUIRect;
} Game;

void InitGame(Game *game);
void UpdateGame(Game *game);
void RenderGame(Game *game);
void SetEntityState(EntityState *state, int value);
void UpdateEntityState(EntityState *state);

int main() {
    Game game;

    InitGame(&game);
    SetTargetFPS(60);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Invaders");

    while (!WindowShouldClose()) {
        UpdateGame(&game);
        RenderGame(&game);
    }

    CloseWindow();

    return 0;
}

void InitGame(Game *game) {
    int screenLeftMargin = 10;
    int screenRightMargin = 10;
    int screenTopMargin = 10;
    int screenBottomMargin = 50;

    game->map.width = SCREEN_WIDTH - screenLeftMargin - screenRightMargin;
    game->map.height = SCREEN_HEIGHT - screenTopMargin - screenBottomMargin;
    game->map.x = screenLeftMargin;
    game->map.y = screenTopMargin;

    game->player.body.width = 50;
    game->player.body.height = 50;
    game->player.body.x = game->map.x;
    game->player.body.y = game->map.height + game->map.y - game->player.body.height;
    game->player.lives = PLAYER_MAX_LIVES;
    game->player.score = 0;
    game->player.state.value = PLAYER_STATE_IDLE;

    game->player.projectile.state.value = PROJECTILE_STATE_INACTIVE;
    game->player.projectile.body.width = 5;
    game->player.projectile.body.height = 20;
    game->player.projectile.body.x = game->player.body.x + game->player.body.width / 2 - game->player.projectile.body.width / 2;
    game->player.projectile.body.y = game->player.body.y - game->player.projectile.body.height - PROJECTILE_OFFSET_FROM_PLAYER;

    game->playerUIRect.width = SCREEN_WIDTH - screenLeftMargin - screenRightMargin;
    game->playerUIRect.height = screenBottomMargin;
    game->playerUIRect.x = screenLeftMargin;
    game->playerUIRect.y = game->map.y + game->map.height;

    for (int i = 0; i < MAX_NUM_OF_ENEMIES - 1; i++) {
        game->enemies[i].body.width = 50;
        game->enemies[i].body.height = 50;
        game->enemies[i].body.x = game->map.x;
        game->enemies[i].body.y = game->map.y;
        game->enemies[i].dir = MOVE_RIGHT;
        game->enemies[i].state.value = ENEMY_STATE_ACTIVE;
    }
}

void UpdateGame(Game *game) {
    Vector2 playerPosition = { game->player.body.x, game->player.body.y };

    if (IsKeyDown(KEY_LEFT)) {
        playerPosition.x -= PLAYER_SPEED * GetFrameTime();
    }

    if (IsKeyDown(KEY_RIGHT)) {
        playerPosition.x += PLAYER_SPEED * GetFrameTime();
    }

    if (playerPosition.x != game->player.body.x || playerPosition.y != game->player.body.y) {
        game->player.body.x = playerPosition.x;
        game->player.body.y = playerPosition.y;
        SetEntityState(&game->player.state, PLAYER_STATE_MOVING);
    } else {
        SetEntityState(&game->player.state, PLAYER_STATE_IDLE);
    }

    if (IsKeyDown(KEY_SPACE)) {
        if (game->player.projectile.state.value == PROJECTILE_STATE_INACTIVE) {
            SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_ACTIVE);
            game->player.projectile.body.x = game->player.body.x + game->player.body.width / 2 - game->player.projectile.body.width / 2;
            game->player.projectile.body.y = game->player.body.y - game->player.projectile.body.height - PROJECTILE_OFFSET_FROM_PLAYER;
        }
    }

    if (game->player.body.x < game->map.x) {
        game->player.body.x = game->map.x;
    } else if (game->player.body.x + game->player.body.width >= game->map.x + game->map.width) {
        game->player.body.x = game->map.x + game->map.width - game->player.body.width;
    }

    UpdateEntityState(&game->player.state);

    if (game->player.projectile.state.value != PROJECTILE_STATE_ACTIVE) {
        UpdateEntityState(&game->player.projectile.state);
    }

    // Player projectile
    if (game->player.projectile.state.value == PROJECTILE_STATE_ACTIVE) {
        game->player.projectile.body.y -= PROJECTILE_SPEED * GetFrameTime();

        for (int i = 0; i < MAX_NUM_OF_ENEMIES; i++) {
            if (game->enemies[i].state.value != ENEMY_STATE_DEAD && game->enemies[i].state.value != ENEMY_STATE_DYING) {
                if (CheckCollisionRecs(game->player.projectile.body, game->enemies[i].body)) {
                    SetEntityState(&game->enemies[i].state, ENEMY_STATE_DYING);
                    SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_EXPLODING);
                }
            }
        }

        // Out of bounds
        if (game->player.projectile.body.y <= game->map.y) {
            game->player.projectile.body.y = game->map.y;
            SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_INACTIVE);
        }
    } else if (game->player.projectile.state.value == PROJECTILE_STATE_EXPLODING) {
        if (game->player.projectile.state.elapsedTime >= PROJECTILE_EXPLOSION_DURATION) {
            SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_INACTIVE);
        }
    }

    // Enemies
    for (int i = 0; i < MAX_NUM_OF_ENEMIES; i++) {
        if (game->enemies[i].state.value != ENEMY_STATE_DEAD) {
            UpdateEntityState(&game->enemies[i].state);
        }

        if (game->enemies[i].state.value == ENEMY_STATE_DYING) {
            if (game->enemies[i].state.elapsedTime >= ENEMY_DYING_DURATION) {
                SetEntityState(&game->enemies[i].state, ENEMY_STATE_DEAD);
            } else {
                // TODO: play dying animation
            }
        } else if (game->enemies[i].state.value == ENEMY_STATE_ACTIVE) {
            if (game->enemies[i].dir == MOVE_RIGHT) {
                game->enemies[i].body.x += ENEMY_HORIZONTAL_SPEED * GetFrameTime();

                if (game->enemies[i].body.x + game->enemies[i].body.width >= game->map.x + game->map.width) {
                    game->enemies[i].body.x = game->map.x + game->map.width - game->enemies[i].body.width;
                    game->enemies[i].dir = MOVE_DOWN;
                }
            } else if (game->enemies[i].dir == MOVE_DOWN) {
                game->enemies[i].body.y += ENEMY_VERTICAL_SPEED * GetFrameTime();

                // This is game over
                if (game->enemies[i].body.y + game->enemies[i].body.height >= game->map.y + game->map.height) {
                    SetEntityState(&game->enemies[i].state, ENEMY_STATE_DYING);
                    // TODO: Implement game over
                } else {
                    if (game->enemies[i].body.x <= game->map.x) {
                        game->enemies[i].dir = MOVE_RIGHT;
                    } else {
                        game->enemies[i].dir = MOVE_LEFT;
                    }
                }
            } else if (game->enemies[i].dir == MOVE_LEFT) {
                game->enemies[i].body.x -= ENEMY_HORIZONTAL_SPEED * GetFrameTime();

                if (game->enemies[i].body.x <= game->map.x) {
                    game->enemies[i].body.x = game->map.x;
                    game->enemies[i].dir = MOVE_DOWN;
                }
            }
        }
    }
}

void RenderGame(Game *game) {
    BeginDrawing();
    ClearBackground(BLACK);

    // Map
    DrawRectangleLines(game->map.x, game->map.y, game->map.width, game->map.height, DARKBROWN);

    // Player
    DrawRectangle(game->player.body.x, game->player.body.y, game->player.body.width, game->player.body.height, RED);

    // Enemies
    for (int i = 0; i < MAX_NUM_OF_ENEMIES; i++) {
        Enemy enemy = game->enemies[i];

        if (enemy.state.value == ENEMY_STATE_DEAD) {
            continue;
        }

        DrawRectangle(enemy.body.x, enemy.body.y, enemy.body.width, enemy.body.height, RED);
    }

    // Projectiles
    if (game->player.projectile.state.value == PROJECTILE_STATE_ACTIVE || game->player.projectile.state.value == PROJECTILE_STATE_EXPLODING) {
        DrawRectangle(
            game->player.projectile.body.x,
            game->player.projectile.body.y,
            game->player.projectile.body.width,
            game->player.projectile.body.height,
            YELLOW
        );
    }

    // UI
    char playerText[14 + PLAYER_MAX_LIVES + PLAYER_MAX_SCORE];
    sprintf(playerText, "Lives %d Score %d", game->player.lives, game->player.score);
    DrawText(playerText, game->playerUIRect.x, game->playerUIRect.y + 10, 20, YELLOW);

    EndDrawing();
}

void SetEntityState(EntityState *state, int value) {
    if (state->value != value) {
        state->value = value;
        state->startTime = GetTime();
    }
}

void UpdateEntityState(EntityState *state) {
    state->elapsedTime = GetTime() - state->startTime;
}