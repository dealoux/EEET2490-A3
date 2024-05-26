#include "gameSystem.h"
#include "../kernel/utils.h"

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

void updatePlayer() {
    // Handle player movement and actions
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
    drawRectARGB32(obj->x, obj->y, obj->x + obj->width, obj->y + obj->height, obj->color, 1);
}

void onPlayerHit(GameObject *player, GameObject *enemy) {
    // Handle player being hit by enemy
    printf("Player hit by enemy!\n");
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
    player.y = SCREEN_HEIGHT - 50;
    player.width = 50;
    player.height = 50;
    player.color = 0xFF0000FF; // Red color
    player.active = 1;
    player.onCollision = onPlayerHit;

    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
        bullets[i].onCollision = onBulletHit;
    }

    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = 0;
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

        clearFrameBuffer(0x00000000);

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