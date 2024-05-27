#ifndef DANMAKU_H
#define DANMAKU_H

#define GAME_FRAME_DELAY 33333
#define UI_WIDTH 200
#define UI_MARGIN 20

#define MAX_PLAYER_BULLETS 50
#define PLAYER_SPEED 7
#define PLAYER_BULLET_SPEED 30
#define SHOT_COOLDOWN 50000

#define MAX_MOBS 20
#define MAX_ENEMY_BULLETS 100
#define ENEMY_BULLET_SPEED 5
#define MOB_SHOT_COOLDOWN 50000
#define MOB_SPAWN_INTERVAL 5000000
#define MOB_SPEED 4

typedef struct GameObject GameObject;
typedef void (*CollisionCallback)(GameObject *a, GameObject *b);

struct GameObject {
    int x, y;
    int width, height;
    unsigned int color;
    const unsigned int *sprite;
    int active;
    CollisionCallback onCollision;
    unsigned int lastShotTime;
    unsigned int speedX, speedY;
};

extern GameObject player;
extern GameObject playerBullets[MAX_PLAYER_BULLETS];
extern GameObject enemiesBullets[MAX_ENEMY_BULLETS];
extern GameObject mobs[MAX_MOBS];

void gameInit();
void gameLoop();

#endif // DANMAKU_H