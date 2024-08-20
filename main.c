#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define PLAYER_SPEED 200
#define PLAYER_MAX_LIVES 3
#define PLAYER_MAX_SCORE 9999
#define PROJECTILE_SPEED 600
#define PROJECTILE_OFFSET_FROM_PLAYER 10
#define PROJECTILE_EXPLOSION_DURATION 0.5 // seconds
#define ENEMIES_COLS 11
#define ENEMIES_ROWS 5
#define MAX_NUM_OF_ENEMIES ENEMIES_ROWS * ENEMIES_COLS
#define ENEMY_VERTICAL_MAX_DISTANCE 50
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

typedef enum MoveDirValue {
    MOVE_NOTSET,
    MOVE_RIGHT,
    MOVE_DOWN,
    MOVE_LEFT,
} MoveDirValue;

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
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    EntityState state;
    MoveDirValue dir;
    MoveDirValue previousDir;
    Vector2 moveStartPosition;
    float distanceTraveled;
} Enemy;

typedef struct EnemyFlock {
    Enemy entities[MAX_NUM_OF_ENEMIES];
    Rectangle boundaries;
    MoveDirValue moveDirection;
} EnemyFlock;

typedef struct Game {
    Player player;
    EnemyFlock enemyFlock;
    Rectangle boundaries;
    Rectangle playerUIRect;
} Game;

float maxForce = 0.01;
float maxHSpeed = 100;
float maxVSpeed = 100;
float enemyDistance = 10;
float flockAwarenessDistance = 200;

void InitGame(Game *game);
void UpdateGame(Game *game);
void RenderGame(Game *game);
void SetEntityState(EntityState *state, int value);
void UpdateEntityState(EntityState *state);
void UpdateEnemyDistanceTraveled(Enemy *enemy);
void UpdateEnemyFlock(Game *game, EnemyFlock *flock);
void RenderEnemyFlock(Game *game, EnemyFlock *flock);
void InitFlock(Game *game, Vector2 startPosition);

int main() {
    Game game;

    InitGame(&game);
    SetTargetFPS(144);

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

    game->boundaries.x = screenLeftMargin;
    game->boundaries.y = screenTopMargin;
    game->boundaries.width = SCREEN_WIDTH - screenLeftMargin - screenRightMargin;
    game->boundaries.height = SCREEN_HEIGHT - screenTopMargin - screenBottomMargin;

    game->player.body.width = 50;
    game->player.body.height = 50;
    game->player.body.x = game->boundaries.x;
    game->player.body.y = game->boundaries.y + game->boundaries.height - game->player.body.height;
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
    game->playerUIRect.y = game->boundaries.y + game->boundaries.height;

    Vector2 startPosition = {game->boundaries.x, game->boundaries.y};
    InitFlock(game, startPosition);
}

void UpdateGame(Game *game) {
    Vector2 playerPosition = { game->player.body.x, game->player.body.y };

    if (IsKeyDown(KEY_LEFT)) {
        playerPosition.x -= PLAYER_SPEED * GetFrameTime();
    }

    if (IsKeyDown(KEY_RIGHT)) {
        playerPosition.x += PLAYER_SPEED * GetFrameTime();
    }

    if (IsKeyPressed(KEY_ONE)) {
        maxForce += 5;
    }

    if (IsKeyPressed(KEY_TWO)) {
        maxForce -= 5;
        if (maxForce < 1) {
            maxForce = 1;
        }
    }

    if (IsKeyPressed(KEY_THREE)) {
        maxVSpeed += 5;
    }

    if (IsKeyPressed(KEY_FOUR)) {
        maxVSpeed -= 5;
        if (maxVSpeed < 5) {
            maxVSpeed = 5;
        }
    }

    if (IsKeyPressed(KEY_FIVE)) {
        maxHSpeed += 5;
    }

    if (IsKeyPressed(KEY_SIX)) {
        maxHSpeed -= 5;
        if (maxHSpeed < 5) {
            maxHSpeed = 5;
        }
    }

    if (IsKeyPressed(KEY_SEVEN)) {
        flockAwarenessDistance += 5;
    }

    if (IsKeyPressed(KEY_EIGHT)) {
        flockAwarenessDistance -= 5;
        if (flockAwarenessDistance < 5) {
            flockAwarenessDistance = 5;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        InitFlock(game, GetMousePosition());
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

    if (game->player.body.x < game->boundaries.x) {
        game->player.body.x = game->boundaries.x;
    } else if (game->player.body.x + game->player.body.width >= game->boundaries.x + game->boundaries.width) {
        game->player.body.x = game->boundaries.x + game->boundaries.width - game->player.body.width;
    }

    UpdateEntityState(&game->player.state);

    if (game->player.projectile.state.value != PROJECTILE_STATE_ACTIVE) {
        UpdateEntityState(&game->player.projectile.state);
    }

    // Player projectile
    if (game->player.projectile.state.value == PROJECTILE_STATE_ACTIVE) {
        game->player.projectile.body.y -= PROJECTILE_SPEED * GetFrameTime();

        // Out of bounds
        if (game->player.projectile.body.y <= game->boundaries.y) {
            game->player.projectile.body.y = game->boundaries.y;
            SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_INACTIVE);
        }
    } else if (game->player.projectile.state.value == PROJECTILE_STATE_EXPLODING) {
        if (game->player.projectile.state.elapsedTime >= PROJECTILE_EXPLOSION_DURATION) {
            SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_INACTIVE);
        }
    }

    UpdateEnemyFlock(game, &game->enemyFlock);
}

void RenderGame(Game *game) {
    BeginDrawing();
    ClearBackground(BLACK);

    // Map
    DrawRectangleLines(game->boundaries.x, game->boundaries.y, game->boundaries.width, game->boundaries.height, DARKBROWN);

    // Player
    DrawRectangle(game->player.body.x, game->player.body.y, game->player.body.width, game->player.body.height, RED);

    // Enemies
    RenderEnemyFlock(game, &game->enemyFlock);

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
    char playerText[50 + PLAYER_MAX_LIVES + PLAYER_MAX_SCORE];
    sprintf(playerText, "Lives %d Score %d Max force %0.1f Speed H%0.1f V%0.1f Flock awareness distance %0.1f", game->player.lives, game->player.score, maxForce, maxHSpeed, maxVSpeed, flockAwarenessDistance);
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

void UpdateEnemyDistanceTraveled(Enemy *enemy) {
    enemy->distanceTraveled = Vector2Distance(enemy->moveStartPosition, enemy->position);
}

void UpdateEnemyFlock(Game *game, EnemyFlock *enemyFlock) {
    for (int i = 0; i < MAX_NUM_OF_ENEMIES; i++) {
        Enemy *entity = &enemyFlock->entities[i];
        UpdateEntityState(&entity->state);

        if (entity->state.value == ENEMY_STATE_ACTIVE) {
            entity->distanceTraveled = Vector2Distance(entity->moveStartPosition, entity->position);

            if (entity->dir == MOVE_RIGHT) {
                entity->position.x += maxHSpeed * GetFrameTime();

                if (entity->body.x + entity->body.width >= game->boundaries.x + game->boundaries.width - 10) {
                    entity->previousDir = entity->dir;
                    entity->dir = MOVE_DOWN;
                    entity->moveStartPosition.x = entity->position.x;
                    entity->moveStartPosition.y = entity->position.y;
                }
            } else if (entity->dir == MOVE_LEFT) {
                entity->position.x -= maxHSpeed * GetFrameTime();

                if (entity->body.x <= game->boundaries.x + 10) {
                    entity->previousDir = entity->dir;
                    entity->dir = MOVE_DOWN;
                    entity->moveStartPosition.x = entity->position.x;
                    entity->moveStartPosition.y = entity->position.y;
                }
            } else if (entity->dir == MOVE_DOWN) {
                entity->position.y += maxVSpeed * GetFrameTime();

                if (entity->distanceTraveled >= entity->body.height + enemyDistance) {
                    if (entity->previousDir == MOVE_LEFT) {
                        entity->dir = MOVE_RIGHT;
                    } else if (entity->previousDir == MOVE_RIGHT) {
                        entity->dir = MOVE_LEFT;
                    }
                }
            }

            entity->body.x = entity->position.x - entity->body.width / 2;
            entity->body.y = entity->position.y - entity->body.height / 2;

            if (game->player.projectile.state.value == PROJECTILE_STATE_ACTIVE) {
                if (CheckCollisionRecs(game->player.projectile.body, entity->body)) {
                    SetEntityState(&entity->state, ENEMY_STATE_DYING);
                    SetEntityState(&game->player.projectile.state, PROJECTILE_STATE_EXPLODING);
                    game->player.score += 10;
                }
            }
        } else if (entity->state.value == ENEMY_STATE_DYING) {
            if (entity->state.elapsedTime >= ENEMY_DYING_DURATION) {
                SetEntityState(&entity->state, ENEMY_STATE_DEAD);
            }
        }

        entity->acceleration.x = 0;
        entity->acceleration.y = 0;
    }
}

void RenderEnemyFlock(Game *game, EnemyFlock *enemyFlock) {
    for (int i = MAX_NUM_OF_ENEMIES - 1; i >= 0; i--) {
        Enemy *entity = &enemyFlock->entities[i];
        if (entity->state.value != ENEMY_STATE_DEAD) {
            Color color = RED;

            if (i / ENEMIES_COLS == 1) {
                color = GREEN;
            } else if (i / ENEMIES_COLS > 1 && i / ENEMIES_COLS < 3) {
                color = YELLOW;
            }

            DrawRectangle(
                entity->body.x,
                entity->body.y,
                entity->body.width,
                entity->body.height,
                color
            );

            DrawCircleV(entity->position, 5, YELLOW);

            // DrawLineV(entity->moveStartPosition, entity->position, WHITE);
        }
    }

    // DrawRectangleLines(enemyFlock->boundaries.x, enemyFlock->boundaries.y, enemyFlock->boundaries.width, enemyFlock->boundaries.height, YELLOW);
}

void InitFlock(Game *game, Vector2 startPosition) {
    float flockWidth = game->enemyFlock.entities[0].body.width / 2 + game->enemyFlock.entities[0].body.width * ENEMIES_COLS + enemyDistance * ENEMIES_COLS;
    // float flockHeight = game->enemyFlock.entities[0].body.width / 2 + game->enemyFlock.entities[0].body.width * ENEMIES_ROWS + enemyDistance * ENEMIES_ROWS;

    for (int i = 0; i < MAX_NUM_OF_ENEMIES; i++) {
        Enemy *entity = &game->enemyFlock.entities[i];
        entity->body.width = 50;
        entity->body.height = 50;
        entity->position.x = startPosition.x + (flockWidth  / 2) + enemyDistance + entity->body.width / 2 + entity->body.width * (i % ENEMIES_COLS) + enemyDistance * (i % ENEMIES_COLS);
        entity->position.y = startPosition.y + enemyDistance + entity->body.height / 2 + entity->body.height  * (i / ENEMIES_COLS) + enemyDistance * (i / ENEMIES_COLS);
        entity->body.x = startPosition.x - entity->body.width / 2;
        entity->body.y = startPosition.y - entity->body.height / 2;
        entity->state.value = ENEMY_STATE_ACTIVE;
        entity->dir = MOVE_RIGHT;
    }
}