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
GameObject playerBullets[MAX_PLAYER_BULLETS];
GameObject enemyBullets[MAX_ENEMY_BULLETS];
GameObject mobs[MAX_MOBS];

unsigned int lastShotTime = 0;

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

void activateBullet(GameObject *bulletPool, int poolSize, int x, int y, int width, int height, const unsigned int *sprite) {
    for (int i = 0; i < poolSize; i++) {
        if (!bulletPool[i].active) {
            bulletPool[i].x = x;
            bulletPool[i].y = y;
            bulletPool[i].width = width;
            bulletPool[i].height = height;
            bulletPool[i].sprite = sprite;
            bulletPool[i].active = 1;
            break;
        }
    }
}

void updatePlayer() {
    // Additional player update logic if needed
}

void updateBullets(GameObject *bulletPool, int poolSize, int speed, int direction) {
    for (int i = 0; i < poolSize; i++) {
        if (bulletPool[i].active) {
            bulletPool[i].y += speed * direction;
            if (bulletPool[i].y < -bulletPool[i].height || bulletPool[i].y > SCREEN_HEIGHT + bulletPool[i].height) {
                bulletPool[i].active = 0; // Deactivate bullet if it goes off-screen
            }
        }
    }
}

void updateMobs() {
    for (int i = 0; i < MAX_MOBS; i++) {
        if (mobs[i].active) {
            mobs[i].y += ENEMY_SPEED;
            if (mobs[i].y > SCREEN_HEIGHT) {
                mobs[i].active = 0; // Deactivate enemy if it goes off-screen
            }
        }
    }
}

void handleCollisions() {
    for (int i = 0; i < MAX_MOBS; i++) {
        if (mobs[i].active) {
            checkCollision(&player, &mobs[i]);
        }
    }

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            for (int j = 0; j < MAX_MOBS; j++) {
                if (mobs[j].active) {
                    checkCollision(&playerBullets[i], &mobs[j]);
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
    printf("Player hit by enemy!\n");
    player->active = 0;
}

void onBulletHit(GameObject *bullet, GameObject *enemy) {
    printf("Bullet hit enemy!\n");
    bullet->active = 0;
    enemy->active = 0;
}

void onEnemyHit(GameObject *enemy, GameObject *player) {
    printf("Enemy hit player!\n");
    enemy->active = 0;
    player->active = 0;
}

void handleInput() {
    if (uart_isReadByteReady()) {
        char input = uart_getc();
        unsigned int currentTime = get_system_time();
        if (input == 'w') {
            player.y -= 5; // Move up
        } else if (input == 's') {
            player.y += 5; // Move down
        } else if (input == 'a') {
            player.x -= 5; // Move left
        } else if (input == 'd') {
            player.x += 5; // Move right
        } else if (input == ' ' && (currentTime - lastShotTime) > SHOT_COOLDOWN) {
            // Fire bullet if cooldown has passed
            activateBullet(playerBullets, MAX_PLAYER_BULLETS, player.x + player.width / 2 - SPRITE_PLAYER_BULLET_WIDTH / 2, player.y, SPRITE_PLAYER_BULLET_WIDTH, SPRITE_PLAYER_BULLET_HEIGHT, spritePlayerBullet);
            lastShotTime = currentTime; // Reset the cooldown timer
        }
    }
}

void gameInit() {
    printf("Initializing player...\n");
    player.x = SCREEN_WIDTH / 2;
    player.y = SCREEN_HEIGHT - 100;
    player.width = SPRITE_PLAYER_WIDTH;
    player.height = SPRITE_PLAYER_HEIGHT;
    player.active = 1;
    player.sprite = spritePlayer;
    player.onCollision = onPlayerHit;

    printf("Initializing player bullets...\n");
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = 0;
        playerBullets[i].width = SPRITE_PLAYER_BULLET_WIDTH;
        playerBullets[i].height = SPRITE_PLAYER_BULLET_HEIGHT;
        playerBullets[i].sprite = spritePlayerBullet;
        playerBullets[i].onCollision = onBulletHit;
    }

    printf("Initializing enemy bullets...\n");
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].active = 0;
        enemyBullets[i].width = SPRITE_ENEMY_BULLET_WIDTH;
        enemyBullets[i].height = SPRITE_ENEMY_BULLET_HEIGHT;
        enemyBullets[i].sprite = spriteEnemyBullet;
        enemyBullets[i].onCollision = onEnemyHit;
    }

    printf("Initializing enemies...\n");
    for (int i = 0; i < MAX_MOBS; i++) {
        mobs[i].active = 0;
        mobs[i].width = SPRITE_MOB_WIDTH;
        mobs[i].height = SPRITE_MOB_HEIGHT;
        mobs[i].sprite = spriteMob;
        mobs[i].onCollision = onEnemyHit;
    }
}

void gameLoop() {
    printf("Entering game loop...\n");

    while (1) {
        handleInput();

        updatePlayer();
        updateBullets(playerBullets, MAX_PLAYER_BULLETS, PLAYER_BULLET_SPEED, -1);
        updateBullets(enemyBullets, MAX_ENEMY_BULLETS, ENEMY_BULLET_SPEED, 1);
        updateMobs();
        handleCollisions();

        drawRectARGB32(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00000000, 1);

        drawGameObject(&player);
        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
            if (playerBullets[i].active) {
                drawGameObject(&playerBullets[i]);
            }
        }
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (enemyBullets[i].active) {
                drawGameObject(&enemyBullets[i]);
            }
        }
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                drawGameObject(&mobs[i]);
            }
        }

        wait_msec(GAME_FRAME_DELAY);
    }
}