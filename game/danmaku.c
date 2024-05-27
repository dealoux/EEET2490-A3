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
#include "../resources/spriteBoss.h"
#include "../resources/spriteItem.h"

GameObject player;
GameObject playerBullets[MAX_PLAYER_BULLETS];
GameObject mobs[MAX_MOBS];
GameObject bosses[MAX_BOSSES];

unsigned int score = 0;
unsigned int lastShotTime = 0;
unsigned int playerPower = 0;
unsigned int lastMobSpawnTime = 0;
int initialPlayerX = SCREEN_WIDTH / 2;
int initialPlayerY = SCREEN_HEIGHT - 100;

unsigned int randSeed = 0;

unsigned int rand() {
    randSeed = randSeed * 1103515245 + 12345;
    return (randSeed / 65536) % 32768;
}

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
    if (player.hp <= 0) player.active = 0;
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
            mobs[i].y += MOB_SPEED;
            if (mobs[i].y > SCREEN_HEIGHT) {
                mobs[i].active = 0; // Deactivate mob if it goes off-screen
            }
        }
    }
}

void updateBosses() {
    for (int i = 0; i < MAX_BOSSES; i++) {
        if (bosses[i].active) {
            bosses[i].y += MOB_SPEED;
            if (bosses[i].y > SCREEN_HEIGHT) {
                bosses[i].active = 0; // Deactivate boss if it goes off-screen
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

    for (int i = 0; i < MAX_BOSSES; i++) {
        if (bosses[i].active) {
            checkCollision(&player, &bosses[i]);
        }
    }

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            for (int j = 0; j < MAX_MOBS; j++) {
                if (mobs[j].active) {
                    checkCollision(&playerBullets[i], &mobs[j]);
                }
            }
            for (int j = 0; j < MAX_BOSSES; j++) {
                if (bosses[j].active) {
                    checkCollision(&playerBullets[i], &bosses[j]);
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
    snprintf(buffer, sizeof(buffer), "%d", player.hp);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 1);

    // Draw Power
    drawString(offsetX, offsetY += UI_MARGIN, "Power", 0xFFFFFFFF, 1);
    snprintf(buffer, sizeof(buffer), "%d", playerPower);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 1);
}

void onPlayerHit(GameObject *player, GameObject *enemy) {
    printf("Player hit by enemy!\n");
    player->hp--; // Decrease player HP
    if (player->hp > 0) {
        // Respawn player at initial position if HP is not 0
        player->x = initialPlayerX;
        player->y = initialPlayerY;
    } else {
        player->active = 0; // Deactivate player if HP is 0
    }
    enemy->active = 0; // Deactivate enemy on collision
}

void onBulletHit(GameObject *bullet, GameObject *enemy) {
    printf("Bullet hit enemy!\n");
    bullet->active = 0;
    enemy->hp--; // Decrease enemy HP
    if (enemy->hp <= 0) {
        enemy->active = 0; // Deactivate enemy if HP is 0
        score += (enemy->width == SPRITE_BOSS_WIDTH) ? 50 : 10; // Increase score based on enemy type
    }
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
        } else if (input == ' ' && (currentTime - lastShotTime) > SHOT_COOLDOWN) {
            activateBullet(playerBullets, MAX_PLAYER_BULLETS, player.x + player.width / 2 - SPRITE_PLAYER_BULLET_WIDTH / 2, player.y, SPRITE_PLAYER_BULLET_WIDTH, SPRITE_PLAYER_BULLET_HEIGHT, spritePlayerBullet, 0, -PLAYER_BULLET_SPEED);
            lastShotTime = currentTime; // Reset the cooldown timer
        }
    }
}

void spawnEnemyWave() {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < MAX_MOBS; j++) {
            if (!mobs[j].active) {
                mobs[j].x = rand() % (SCREEN_WIDTH - SPRITE_MOB_WIDTH);
                mobs[j].y = -SPRITE_MOB_HEIGHT * (i + 1);
                mobs[j].width = SPRITE_MOB_WIDTH;
                mobs[j].height = SPRITE_MOB_HEIGHT;
                mobs[j].sprite = spriteMob;
                mobs[j].active = 1;
                mobs[j].speedX = 0;
                mobs[j].speedY = MOB_SPEED;
                mobs[j].hp = MOB_HP;
                break;
            }
        }
    }

    if (rand() % 10 == 0) { // Occasionally spawn a boss
        for (int j = 0; j < MAX_BOSSES; j++) {
            if (!bosses[j].active) {
                bosses[j].x = rand() % (SCREEN_WIDTH - SPRITE_BOSS_WIDTH);
                bosses[j].y = -SPRITE_BOSS_HEIGHT;
                bosses[j].width = SPRITE_BOSS_WIDTH;
                bosses[j].height = SPRITE_BOSS_HEIGHT;
                bosses[j].sprite = spriteBoss;
                bosses[j].active = 1;
                bosses[j].speedX = 0;
                bosses[j].speedY = MOB_SPEED;
                bosses[j].hp = BOSS_HP;
                break;
            }
        }
    }
}

void gameInit() {
    printf("Initializing player...\n");
    player.x = initialPlayerX;
    player.y = initialPlayerY;
    player.width = SPRITE_PLAYER_WIDTH;
    player.height = SPRITE_PLAYER_HEIGHT;
    player.active = 1;
    player.sprite = spritePlayer;
    player.onCollision = onPlayerHit;
    player.hp = PLAYER_HP;

    printf("Initializing player bullets...\n");
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = 0;
        playerBullets[i].width = SPRITE_PLAYER_BULLET_WIDTH;
        playerBullets[i].height = SPRITE_PLAYER_BULLET_HEIGHT;
        playerBullets[i].sprite = spritePlayerBullet;
        playerBullets[i].onCollision = onBulletHit;
    }

    printf("Initializing enemies...\n");
    for (int i = 0; i < MAX_MOBS; i++) {
        mobs[i].active = 0;
        mobs[i].width = SPRITE_MOB_WIDTH;
        mobs[i].height = SPRITE_MOB_HEIGHT;
        mobs[i].sprite = spriteMob;
        mobs[i].hp = MOB_HP;
    }

    printf("Initializing bosses...\n");
    for (int i = 0; i < MAX_BOSSES; i++) {
        bosses[i].active = 0;
        bosses[i].width = SPRITE_BOSS_WIDTH;
        bosses[i].height = SPRITE_BOSS_HEIGHT;
        bosses[i].sprite = spriteBoss;
        bosses[i].hp = BOSS_HP;
    }

    printf("Initializing variables...\n");
    score = 0;
    playerPower = 0;
    lastMobSpawnTime = 0;
    randSeed = 0;
}

void gameLoop() {
    printf("Entering game loop...\n");

    while (1) {
        unsigned int currentTime = get_system_time();

        handleInput();

        updatePlayer();
        updateBullets(playerBullets, MAX_PLAYER_BULLETS);
        updateMobs();
        updateBosses();
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
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) {
                drawGameObject(&mobs[i]);
            }
        }
        for (int i = 0; i < MAX_BOSSES; i++) {
            if (bosses[i].active) {
                drawGameObject(&bosses[i]);
            }
        }

        drawUI();
        wait_msec(GAME_FRAME_DELAY);

        // Check for game over
        if (!player.active) {
            printf("Game Over!\n");
            break;
        }
    }
}