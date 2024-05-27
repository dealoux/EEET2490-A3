#include "danmaku.h"
#include "../gcclib/stddef.h"
#include "../cli/printf.h"
#include "../kernel/utils.h"
#include "../uart/uart1.h"
#include "../resources/spritePlayer.h"
#include "../resources/spritePlayerBullet.h"
#include "../resources/spriteMob.h"
#include "../resources/spriteEnemyBullet.h"
#include "../resources/spriteBoss.h"
#include "../resources/spriteItem.h"
#include "../resources/spriteBackground.h"

GameObject player;
GameObject bullets[MAX_BULLETS];
GameObject enemies[MAX_ENEMIES];

void checkCollision(GameObject *a, GameObject *b) {
    if (a->x < b->x + b->width &&
        a->x + a->width > b->x &&
        a->y < b->y + b->height &&
        a->y + a->height > b->y) {
        if (a->onCollision) {
            a->onCollision(a, b);
        }
        if (b->onCollision) {
            b->onCollision(b, a);
        }
    }
}

void handleInput() {
    char input = uart_getc();
    if (input == 'w') {
        player.y -= 5; // Move up
    } else if (input == 's') {
        player.y += 5; // Move down
    } else if (input == 'a') {
        player.x -= 5; // Move left
    } else if (input == 'd') {
        player.x += 5; // Move right
    } else if (input == ' ') {
        // Fire bullet
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].x = player.x + player.width / 2 - bullets[i].width / 2;
                bullets[i].y = player.y;
                bullets[i].active = 1;
                break;
            }
        }
    }
}

void updatePlayer() {
    // Additional player update logic if needed
}

void updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y -= BULLET_SPEED;
            if (bullets[i].y < 0) {
                bullets[i].active = 0; // Deactivate bullet if it goes off-screen
            }
        }
    }
}

void updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            enemies[i].y += ENEMY_SPEED;
            if (enemies[i].y > SCREEN_HEIGHT) {
                enemies[i].active = 0; // Deactivate enemy if it goes off-screen
            }
        }
    }
}

void handleCollisions() {
    // Check for collisions between player and enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            checkCollision(&player, &enemies[i]);
        }
    }

    // Check for collisions between bullets and enemies
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].active) {
                    checkCollision(&bullets[i], &enemies[j]);
                }
            }
        }
    }
}

void drawGameObject(GameObject *obj) {
    if (obj->sprite != NULL) {
        drawImage(obj->sprite, obj->x, obj->y, obj->width, obj->height);
    } else {
        drawRectARGB32(obj->x, obj->y, obj->x + obj->width, obj->y + obj->height, obj->color, 1);
    }
}

void onPlayerHit(GameObject *player, GameObject *enemy) {
    // Handle player being hit by enemy
    printf("Player hit by enemy!\n");
    // For example, deactivate the player or reduce health
    player->active = 0;
}

void onBulletHit(GameObject *bullet, GameObject *enemy) {
    // Handle bullet hitting enemy
    printf("Bullet hit enemy!\n");
    bullet->active = 0;
    enemy->active = 0;
}

void onEnemyHit(GameObject *enemy, GameObject *player) {
    // Handle enemy hitting player
    printf("Enemy hit player!\n");
    enemy->active = 0;
    player->active = 0;
}

void gameInit() {
    // Initialize player
    player.x = SCREEN_WIDTH / 2;
    player.y = SCREEN_HEIGHT - 100;
    player.width = SPRITE_PLAYER_WIDTH;
    player.height = SPRITE_PLAYER_HEIGHT;
    player.active = 1;
    player.sprite = spritePlayer;
    player.onCollision = onPlayerHit;

    // Initialize player bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
        bullets[i].width = SPRITE_PLAYER_BULLET_WIDTH;
        bullets[i].height = SPRITE_PLAYER_BULLET_HEIGHT;
        bullets[i].sprite = spritePlayerBullet;
        bullets[i].onCollision = onBulletHit;
    }

    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = 0;
        enemies[i].width = SPRITE_MOB_WIDTH;
        enemies[i].height = SPRITE_MOB_HEIGHT;
        enemies[i].sprite = spriteMob;
        enemies[i].onCollision = onEnemyHit;
    }
}

void gameLoop() {
    while (1) {
        handleInput(); // Capture player inputs

        updatePlayer();
        updateBullets();
        updateEnemies();
        handleCollisions();

        // clear screen
        drawRectARGB32(0, 0, 2000, 2000, 0x00000000, 1);

        drawGameObject(&player);
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                drawGameObject(&bullets[i]);
            }
        }
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                drawGameObject(&enemies[i]);
            }
        }

        wait_msec(10000); // Adjust frame timing as needed
    }
}