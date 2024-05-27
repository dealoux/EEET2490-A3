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
GameObject powerItems[MAX_POWER_ITEMS];
GameObject scoreItems[MAX_SCORE_ITEMS];

unsigned int score = 0;
unsigned int lastShotTime = 0;
unsigned int playerPower = 1;
unsigned int lastMobSpawnTime = 0;
unsigned int playerRespawnTime = 0;
int initialPlayerX = SCREEN_WIDTH / 2;
int initialPlayerY = SCREEN_HEIGHT - 100;

unsigned int randSeed = 0;

unsigned int rand() {
    randSeed = randSeed * 1103515245 + 12345;
    return (randSeed / 65536) % 32768;
}

void handle_timer_interrupt() {
    SYSTEM_TIMER_CS = (1 << 1); // Clear the timer interrupt flag
    SYSTEM_TIMER_C1 += 1000000; // Schedule the next timer interrupt (1 second)

    unsigned int currentTime = SYSTEM_TIMER_CLO;

    // Handle your timed tasks here
    if (currentTime >= playerRespawnTime) {
        // End invulnerability period
        playerRespawnTime = 0;
    }
}

void __attribute__((interrupt("IRQ"))) irq_handler() {
    if (SYSTEM_TIMER_CS & (1 << 1)) {
        handle_timer_interrupt();
    }
}

void checkCollision(GameObject *a, GameObject *b) {
    if (!a->active || !b->active) return;
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

void activateItem(GameObject *itemPool, int poolSize, int x, int y, int width, int height, const unsigned int *sprite) {
    for (int i = 0; i < poolSize; i++) {
        if (!itemPool[i].active) {
            itemPool[i].x = x;
            itemPool[i].y = y;
            itemPool[i].width = width;
            itemPool[i].height = height;
            itemPool[i].sprite = sprite;
            itemPool[i].active = 1;
            itemPool[i].speedX = 0;
            itemPool[i].speedY = ITEM_SPEED;
            break;
        }
    }
}

void updateGameObject(GameObject *obj, int screenHeight) {
    if (obj->active) {
        obj->y += obj->speedY;
        if (obj->y > screenHeight) {
            obj->active = 0;
        }
    }
}

void updatePlayer() {
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
                bulletPool[i].active = 0;
            }
        }
    }
}

void updateGameObjects(GameObject *pool, int poolSize, int screenHeight) {
    for (int i = 0; i < poolSize; i++) {
        updateGameObject(&pool[i], screenHeight);
    }
}

void handleCollisions() {
    unsigned int currentTime = SYSTEM_TIMER_CLO;
    if (currentTime - playerRespawnTime < 2000000) {
        return;
    }

    for (int i = 0; i < MAX_MOBS; i++) {
        checkCollision(&player, &mobs[i]);
    }

    for (int i = 0; i < MAX_BOSSES; i++) {
        checkCollision(&player, &bosses[i]);
    }

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            for (int j = 0; j < MAX_MOBS; j++) {
                checkCollision(&playerBullets[i], &mobs[j]);
            }
            for (int j = 0; j < MAX_BOSSES; j++) {
                checkCollision(&playerBullets[i], &bosses[j]);
            }
        }
    }

    for (int i = 0; i < MAX_POWER_ITEMS; i++) {
        checkCollision(&player, &powerItems[i]);
    }

    for (int i = 0; i < MAX_SCORE_ITEMS; i++) {
        checkCollision(&player, &scoreItems[i]);
    }
}

void drawHealthBar(GameObject *obj) {
    if (obj->hp > 0) {
        int barWidth = obj->width;
        int barHeight = 4;
        int greenBarWidth = (obj->hp * barWidth) / obj->maxHp;
        drawRectARGB32(obj->x, obj->y + obj->height + 2, obj->x + greenBarWidth, obj->y + obj->height + 2 + barHeight, 0xFF00FF00, 1);
    }
}

void drawGameObject(GameObject *obj) {
    if (obj->active) {
        if (obj->sprite != NULL) {
            drawImage(obj->sprite, obj->x, obj->y, obj->width, obj->height);
        } else {
            drawRectARGB32(obj->x, obj->y, obj->x + obj->width, obj->y + obj->height, obj->color, 1);
        }
        drawHealthBar(obj);
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
    int offsetX = SCREEN_WIDTH - UI_WIDTH / 1.6 - UI_MARGIN * 2;
    int offsetXValue = offsetX + UI_MARGIN * 6;
    int offsetY = SCREEN_HEIGHT / 2 - UI_MARGIN * 3;

    char buffer[10]; // Buffer to hold the string representation of numbers

    // Draw Score
    drawString(offsetX, offsetY, "Score", 0xFFFFFFFF, 2);
    snprintf(buffer, sizeof(buffer), "%d", score);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 2);

    // Draw HP
    drawString(offsetX, offsetY += UI_MARGIN, "HP", 0xFFFFFFFF, 2);
    snprintf(buffer, sizeof(buffer), "%d", player.hp);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 2);

    // Draw Power
    drawString(offsetX, offsetY += UI_MARGIN, "Power", 0xFFFFFFFF, 2);
    snprintf(buffer, sizeof(buffer), "%d", playerPower);
    drawString(offsetXValue, offsetY, buffer, 0xFFFFFFFF, 2);
}

void onPlayerHit(GameObject *player, GameObject *enemy) {
    if (!enemy->active) return; // Ensure the enemy is active
    if (enemy->sprite != spriteItemArray[SCORE] && enemy->sprite != spriteItemArray[POWER]) {
        printf("Player hit by enemy!\n");
        player->hp--; // Decrease player HP
        if (player->hp > 0) {
            // Respawn player at initial position if HP is not 0
            player->x = initialPlayerX;
            player->y = initialPlayerY;
            playerRespawnTime = SYSTEM_TIMER_CLO + 2000000; // 2 seconds invulnerability
        } else {
            player->active = 0; // Deactivate player if HP is 0
        }
        enemy->active = 0; // Deactivate enemy on collision
    }
}

void onBulletHit(GameObject *bullet, GameObject *enemy) {
    printf("Bullet hit enemy!\n");
    bullet->active = 0;

    // Reduce HP of the enemy and deactivate if HP reaches 0
    enemy->hp--;
    if (enemy->hp <= 0) {
        enemy->active = 0;
        score += 10;

        // Drop items when a boss dies
        if (enemy->sprite == spriteBoss) {
            // Drop 1 power item
            activateItem(powerItems, MAX_POWER_ITEMS, enemy->x + 30, enemy->y + 20, SPRITE_ITEM_WIDTH, SPRITE_ITEM_HEIGHT, spriteItemArray[POWER]);
            // Drop 4 score items
            for (int i = 0; i < 4; i++) {
                activateItem(scoreItems, MAX_SCORE_ITEMS, enemy->x + i * 20, enemy->y, SPRITE_ITEM_WIDTH, SPRITE_ITEM_HEIGHT, spriteItemArray[SCORE]);
            }
        }
    }
}

void onPowerItemPickup(GameObject *player, GameObject *item) {
    printf("Player picked up a power item!\n");
    if (playerPower < PLAYER_MAX_POWER) {
        playerPower++;
    }
    item->active = 0;
}

void onScoreItemPickup(GameObject *player, GameObject *item) {
    printf("Player picked up a score item!\n");
    score += 10;
    item->active = 0;
}

void handleInput() {
    if (uart_isReadByteReady()) {
        char input = uart_getc();
        unsigned int currentTime = SYSTEM_TIMER_CLO;
        if (input == 'w') {
            player.y -= PLAYER_SPEED; // Move up
        } else if (input == 's') {
            player.y += PLAYER_SPEED; // Move down
        } else if (input == 'a') {
            player.x -= PLAYER_SPEED; // Move left
        } else if (input == 'd') {
            player.x += PLAYER_SPEED; // Move right
        } else if (input == ' ' && (currentTime - lastShotTime) > SHOT_COOLDOWN) {
            int bulletXOffsets[] = {0, -10, 10}; // Offsets for 1, 2, and 3 bullets
            int numBullets = playerPower;

            for (int i = 0; i < numBullets; i++) {
                int bulletX = player.x + player.width / 2 - SPRITE_PLAYER_BULLET_WIDTH / 2 + bulletXOffsets[i];
                activateBullet(playerBullets, MAX_PLAYER_BULLETS, bulletX, player.y, SPRITE_PLAYER_BULLET_WIDTH, SPRITE_PLAYER_BULLET_HEIGHT, spritePlayerBullet, 0, -PLAYER_BULLET_SPEED);
            }

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
                mobs[j].maxHp = MOB_HP;
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
                bosses[j].maxHp = BOSS_HP;
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
    player.maxHp = PLAYER_HP;

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
        mobs[i].maxHp = MOB_HP;
    }

    printf("Initializing bosses...\n");
    for (int i = 0; i < MAX_BOSSES; i++) {
        bosses[i].active = 0;
        bosses[i].width = SPRITE_BOSS_WIDTH;
        bosses[i].height = SPRITE_BOSS_HEIGHT;
        bosses[i].sprite = spriteBoss;
        bosses[i].hp = BOSS_HP;
        bosses[i].maxHp = BOSS_HP;
    }

    printf("Initializing power items...\n");
    for (int i = 0; i < MAX_POWER_ITEMS; i++) {
        powerItems[i].active = 0;
        powerItems[i].width = SPRITE_ITEM_WIDTH;
        powerItems[i].height = SPRITE_ITEM_HEIGHT;
        powerItems[i].sprite = spriteItemArray[POWER];
        powerItems[i].onCollision = onPowerItemPickup;
    }

    printf("Initializing score items...\n");
    for (int i = 0; i < MAX_SCORE_ITEMS; i++) {
        scoreItems[i].active = 0;
        scoreItems[i].width = SPRITE_ITEM_WIDTH;
        scoreItems[i].height = SPRITE_ITEM_HEIGHT;
        scoreItems[i].sprite = spriteItemArray[SCORE];
        scoreItems[i].onCollision = onScoreItemPickup;
    }

    printf("Initializing variables...\n");
    score = 0;
    playerPower = 1;
    lastMobSpawnTime = 0;
    randSeed = 0;
    playerRespawnTime = 0;

    init_system_timer(); // Initialize the system timer
    enable_interrupts(); // Enable interrupts
}

void gameLoop() {
    printf("Entering game loop...\n");

    while (1) {
        unsigned int currentTime = SYSTEM_TIMER_CLO;

        handleInput();

        updatePlayer();
        updateBullets(playerBullets, MAX_PLAYER_BULLETS);
        updateGameObjects(mobs, MAX_MOBS, SCREEN_HEIGHT);
        updateGameObjects(bosses, MAX_BOSSES, SCREEN_HEIGHT);
        updateGameObjects(powerItems, MAX_POWER_ITEMS, SCREEN_HEIGHT);
        updateGameObjects(scoreItems, MAX_SCORE_ITEMS, SCREEN_HEIGHT);
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
        for (int i = 0; i < MAX_POWER_ITEMS; i++) {
            if (powerItems[i].active) {
                drawGameObject(&powerItems[i]);
            }
        }
        for (int i = 0; i < MAX_SCORE_ITEMS; i++) {
            if (scoreItems[i].active) {
                drawGameObject(&scoreItems[i]);
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