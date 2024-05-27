#ifndef DANMAKU_H
#define DANMAKU_H

#define GAME_FRAME_DELAY 33333
#define UI_WIDTH 200
#define UI_MARGIN 20

#define PLAYER_HP 3
#define MAX_PLAYER_BULLETS 50
#define PLAYER_SPEED 8
#define PLAYER_BULLET_SPEED 30
#define SHOT_COOLDOWN 50000
#define PLAYER_MAX_POWER 3
#define PLAYER_INVULNERABILITY_TIME 2000000

#define MOB_HP 2
#define MAX_MOBS 30
#define BOSS_HP 8
#define MAX_BOSSES 2
#define MOB_SPAWN_INTERVAL 5000000
#define MOB_SPEED 4

#define MAX_POWER_ITEMS 4
#define MAX_SCORE_ITEMS 16
#define ITEM_SPEED 6

typedef struct GameObject GameObject;
typedef void (*CollisionCallback)(GameObject *a, GameObject *b);

struct GameObject {
    int x, y;
    int width, height;
    unsigned int color;
    const unsigned int *sprite;
    unsigned int hp;
    unsigned int maxHp;
    int active;
    CollisionCallback onCollision;
    unsigned int speedX, speedY;
};

extern GameObject player;
extern GameObject playerBullets[MAX_PLAYER_BULLETS];
extern GameObject mobs[MAX_MOBS];
extern GameObject bosses[MAX_BOSSES];
extern GameObject powerItems[MAX_POWER_ITEMS];
extern GameObject scoreItems[MAX_SCORE_ITEMS];

void gameInit();
void gameLoop();

#endif // DANMAKU_H