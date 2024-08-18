#include <stdio.h>
#include "raylib.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define PLAYER_SPEED 200
#define PLAYER_MAX_LIVES 3
#define PROJECTILE_SPEED 600
#define PROJECTILE_OFFSET_FROM_PLAYER 10
#define PROJECTILE_COLLISION_DURATION 0.5 // seconds

typedef enum ProjectileState {
    STATE_INACTIVE,
    STATE_ACTIVE,
    STATE_COLLIDING,
} ProjectileState;

typedef struct Projectile {
    Rectangle body;
    int isActive;
    ProjectileState state;
    double currentStateStartTime;
    double currentStateElapsedTime;
} Projectile;

typedef struct Player {
    Rectangle body;
    Projectile projectile;
    int lives;
    int isFiring;
} Player;

typedef struct Game {
    Player player;
    Rectangle map;
    Rectangle playerUIRect;
} Game;

void InitGame(Game *game);
void UpdateGame(Game *game);
void RenderGame(Game *game);

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
    game->player.isFiring = 0;

    game->player.projectile.state = STATE_INACTIVE;
    game->player.projectile.currentStateStartTime = -1;
    game->player.projectile.currentStateElapsedTime = -1;
    game->player.projectile.body.width = 2;
    game->player.projectile.body.height = 20;
    game->player.projectile.body.x = game->player.body.x + game->player.body.width / 2 - game->player.projectile.body.width / 2;
    game->player.projectile.body.y = game->player.body.y - game->player.projectile.body.height - PROJECTILE_OFFSET_FROM_PLAYER;

    game->playerUIRect.width = SCREEN_WIDTH - screenLeftMargin - screenRightMargin;
    game->playerUIRect.height = screenBottomMargin;
    game->playerUIRect.x = screenLeftMargin;
    game->playerUIRect.y = game->map.y + game->map.height;
}

void UpdateGame(Game *game) {
    if (IsKeyDown(KEY_LEFT)) {
        game->player.body.x -= PLAYER_SPEED * GetFrameTime();
    }

    if (IsKeyDown(KEY_RIGHT)) {
        game->player.body.x += PLAYER_SPEED * GetFrameTime();
    }

    if (IsKeyDown(KEY_SPACE)) {
        if (game->player.projectile.state == STATE_INACTIVE) {
            game->player.projectile.state = STATE_ACTIVE;
            game->player.projectile.currentStateStartTime = GetTime();
            game->player.projectile.body.x = game->player.body.x + game->player.body.width / 2 - game->player.projectile.body.width / 2;
            game->player.projectile.body.y = game->player.body.y - game->player.projectile.body.height - PROJECTILE_OFFSET_FROM_PLAYER;
        }
    }

    if (game->player.body.x < game->map.x) {
        game->player.body.x = game->map.x;
    } else if (game->player.body.x + game->player.body.width >= game->map.x + game->map.width) {
        game->player.body.x = game->map.x + game->map.width - game->player.body.width;
    }

    if (game->player.projectile.state != STATE_ACTIVE) {
        game->player.projectile.currentStateElapsedTime = GetTime() - game->player.projectile.currentStateStartTime;
    }

    if (game->player.projectile.state == STATE_ACTIVE) {
        game->player.projectile.body.y -= PROJECTILE_SPEED * GetFrameTime();

        if (game->player.projectile.body.y <= game->map.y) {
            game->player.projectile.body.y = game->map.y;
            game->player.projectile.state = STATE_COLLIDING;
            game->player.projectile.currentStateStartTime = GetTime();
        }
    } else if (game->player.projectile.state == STATE_COLLIDING) {
        if (game->player.projectile.currentStateElapsedTime >= PROJECTILE_COLLISION_DURATION) {
            game->player.projectile.state = STATE_INACTIVE;
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

    // Projectiles
    if (game->player.projectile.state == STATE_ACTIVE || game->player.projectile.state == STATE_COLLIDING) {
        DrawRectangle(
            game->player.projectile.body.x,
            game->player.projectile.body.y,
            game->player.projectile.body.width,
            game->player.projectile.body.height,
            YELLOW
        );
    }

    // UI
    char livesText[40];
    sprintf(livesText, "Lives %d", game->player.lives);
    DrawText(livesText, game->playerUIRect.x, game->playerUIRect.y + 10, 20, YELLOW);

    EndDrawing();
}