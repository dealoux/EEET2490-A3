#ifndef DANMAKU_H
#define DANMAKU_H

#define MAX_BULLETS 50
#define MAX_ENEMIES 20
#define BULLET_SPEED 5
#define ENEMY_SPEED 2

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
extern GameObject bullets[MAX_BULLETS];
extern GameObject enemies[MAX_ENEMIES];

void checkCollision(GameObject *a, GameObject *b);
void updatePlayer();
void updateBullets();
void updateEnemies();
void handleCollisions();
void drawGameObject(GameObject *obj);
void gameInit();
void gameLoop();
void handleInput();

#endif // DANMAKU_H