#ifndef DANMAKU_H
#define DANMAKU_H

#define GAME_FRAME_DELAY 33333

#define MAX_PLAYER_BULLETS 50
#define PLAYER_BULLET_SPEED 30
#define SHOT_COOLDOWN 50000

#define MAX_MOBS 20
#define MAX_ENEMY_BULLETS 100
#define ENEMY_BULLET_SPEED 10
#define ENEMY_SPEED 10

typedef struct GameObject GameObject;
typedef void (*CollisionCallback)(GameObject *a, GameObject *b);

struct GameObject {
    int x, y;
    int width, height;
    unsigned int color;
    const unsigned int *sprite;
    int active;
    CollisionCallback onCollision;
};

extern GameObject player;
extern GameObject playerBullets[MAX_PLAYER_BULLETS];
extern GameObject enemiesBullets[MAX_ENEMY_BULLETS];
extern GameObject mobs[MAX_MOBS];

void checkCollision(GameObject *a, GameObject *b);
void updatePlayer();
void updateBullets();
void updateMobs();
void handleCollisions();
void drawGameObject(GameObject *obj);
void gameInit();
void gameLoop();
void handleInput();

#endif // DANMAKU_H