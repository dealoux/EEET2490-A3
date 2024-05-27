#include "danmaku.h"
#include "../gcclib/stddef.h"
#include "../gcclib/stdarg.h"
#include "../gcclib/stdint.h"
#include "../cli/printf.h"
#include "../kernel/utils.h"
#include "../uart/uart1.h"
#include "../resources/spritePlayer.h"
#include "../resources/spritePlayerBullet.h"
#include "../resources/spriteMob.h"
#include "../resources/spriteEnemyBullet.h"
#include "../resources/spriteBoss.h"
#include "../resources/spriteItem.h"

GameObject player;
GameObject playerBullets[MAX_PLAYER_BULLETS];
GameObject enemyBullets[MAX_ENEMY_BULLETS];
GameObject mobs[MAX_MOBS];

unsigned int score = 0;
unsigned int playerHP = 3;
unsigned int playerPower = 0;
unsigned int lastMobSpawnTime = 0;
int waveDirection = 1; // 1 for left to right, -1 for right to left

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

void activateBullet(GameObject *bulletPool, int poolSize, int x, int y, int width, int height, const unsigned int *sprite, int speedX, int speedY) {
    for (int i = 0; i < poolSize; i++) {
        if (!bulletPool[i].active) {
            bulletPool[i].x = x;
            bulletPool[i].y = y;
            bulletPool[i].width = width;
            bulletPool[i].height = height;
            bulletPool[i].sprite = sprite;
            bulletPool[i].active = 1;
            bulletPool[i].speedX = speedX;
            bulletPool[i].speedY = speedY;
            break;
        }
    }
}

void updatePlayer() {
    // Ensure player stays within screen boundaries
    if (player.x < 0) player.x = 0;
    if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;
    if (player.y < 0) player.y = 0;
    if (player.y + player.height > SCREEN_HEIGHT) player.y = SCREEN_HEIGHT - player.height;
}

void updateBullets(GameObject *bulletPool, int poolSize) {
    for (int i = 0; i < poolSize; i++) {
        if (bulletPool[i].active) {
            bulletPool[i].x += bulletPool[i].speedX;
            bulletPool[i].y += bulletPool[i].speedY;
            if (bulletPool[i].y < -bulletPool[i].height || bulletPool[i].y > SCREEN_HEIGHT + bulletPool[i].height ||
                bulletPool[i].x < -bulletPool[i].width || bulletPool[i].x > SCREEN_WIDTH + bulletPool[i].width) {
                bulletPool[i].active = 0; // Deactivate bullet if it goes off-screen
            }
        }
    }
}

void updateMobs() {
    for (int i = 0; i < MAX_MOBS; i++) {
        if (mobs[i].active) {
            mobs[i].x += mobs[i].speedX;
            if (mobs[i].x < -mobs[i].width || mobs[i].x > SCREEN_WIDTH) {
                mobs[i].active = 0; // Deactivate mob if it goes off-screen
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

// A simple implementation of snprintf for converting integers to strings
int snprintf(char *str, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int num = va_arg(args, int);
    int n = 0;
    int temp_num = num;

    // Calculate number of digits
    do {
        n++;
        temp_num /= 10;
    } while (temp_num != 0);

    // Check if the buffer size is sufficient
    if (n >= size) {
        va_end(args);
        return -1;
    }

    str[n] = '\0';
    while (n--) {
        str[n] = '0' + (num % 10);
        num /= 10;
    }

    va_end(args);
    return n;
}

void drawUI() {
    int offsetX = SCREEN_WIDTH - UI_WIDTH / 2 - UI_MARGIN * 2;
    int offsetXValue = offsetX + UI_MARGIN * 3;
    int offsetY = SCREEN_HEIGHT / 2 - UI_MARGIN * 3;

    char buffer[10]; // Buffer to hold the string representation of numbers

    // Draw Score
    drawString(offsetX, offsetY, "Score", 0xFFFFFFFF, 1);
    snprintf(buffer, sizeof(buffer), "%d", score);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 1);

    // Draw HP
    drawString(offsetX, offsetY += UI_MARGIN, "HP", 0xFFFFFFFF, 1);
    snprintf(buffer, sizeof(buffer), "%d", playerHP);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 1);

    // Draw Power
    drawString(offsetX, offsetY += UI_MARGIN, "Power", 0xFFFFFFFF, 1);
    snprintf(buffer, sizeof(buffer), "%d", playerPower);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 1);
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
            player.y -= PLAYER_SPEED; // Move up
        } else if (input == 's') {
            player.y += PLAYER_SPEED; // Move down
        } else if (input == 'a') {
            player.x -= PLAYER_SPEED; // Move left
        } else if (input == 'd') {
            player.x += PLAYER_SPEED; // Move right
        } else if (input == ' ' && (currentTime - player.lastShotTime) > SHOT_COOLDOWN) {
            activateBullet(playerBullets, MAX_PLAYER_BULLETS, player.x + player.width / 2 - SPRITE_PLAYER_BULLET_WIDTH / 2, player.y, SPRITE_PLAYER_BULLET_WIDTH, SPRITE_PLAYER_BULLET_HEIGHT, spritePlayerBullet, 0, -PLAYER_BULLET_SPEED);
            player.lastShotTime = currentTime; // Reset the cooldown timer
        }
    }
}

void spawnEnemyWave() {
    int startY = SCREEN_WIDTH /4 - SPRITE_MOB_HEIGHT;
    int speedX = (waveDirection == 1) ? MOB_SPEED : -MOB_SPEED;
    int startX = (waveDirection == 1) ? 0 : SCREEN_WIDTH - SPRITE_MOB_WIDTH;

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < MAX_MOBS; j++) {
            if (!mobs[j].active) {
                mobs[j].x = startX - i * SPRITE_MOB_WIDTH * waveDirection;
                mobs[j].y = startY;
                mobs[j].width = SPRITE_MOB_WIDTH;
                mobs[j].height = SPRITE_MOB_HEIGHT;
                mobs[j].sprite = spriteMob;
                mobs[j].active = 1;
                mobs[j].speedX = speedX;
                break;
            }
        }
    }
    waveDirection *= -1; // Change direction for the next wave
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
        unsigned int currentTime = get_system_time();

        handleInput();

        updatePlayer();
        updateBullets(playerBullets, MAX_PLAYER_BULLETS);
        updateBullets(enemyBullets, MAX_ENEMY_BULLETS);
        updateMobs();
        handleCollisions();

        if (currentTime - lastMobSpawnTime > MOB_SPAWN_INTERVAL) {
            spawnEnemyWave();
            lastMobSpawnTime = currentTime;
        }

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

        drawUI();
        wait_msec(GAME_FRAME_DELAY);
    }
}