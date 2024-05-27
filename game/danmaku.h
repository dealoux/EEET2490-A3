#ifndef DANMAKU_H
#define DANMAKU_H

#define GAME_FRAME_DELAY 33333
#define UI_WIDTH 200
#define UI_MARGIN 20

#define PLAYER_HP 3
#define MAX_PLAYER_BULLETS 50
#define PLAYER_SPEED 7
#define PLAYER_BULLET_SPEED 30
#define SHOT_COOLDOWN 50000

#define MOB_HP 2
#define MAX_MOBS 30
#define BOSS_HP 8
#define MAX_BOSSES 2
#define MOB_SPAWN_INTERVAL 5000000
#define MOB_SPEED 4

typedef struct GameObject GameObject;
typedef void (*CollisionCallback)(GameObject *a, GameObject *b);

struct GameObject {
    int x, y;
    int width, height;
    unsigned int color;
    const unsigned int *sprite;
    unsigned int hp;
    int active;
    CollisionCallback onCollision;
    unsigned int speedX, speedY;
};

extern GameObject player;
extern GameObject playerBullets[MAX_PLAYER_BULLETS];
extern GameObject mobs[MAX_MOBS];
extern GameObject bosses[MAX_BOSSES];

void gameInit();
void gameLoop();

#endif // DANMAKU_H