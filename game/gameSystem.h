#define MAX_BULLETS 50
#define MAX_ENEMIES 20
#define BULLET_SPEED 5
#define ENEMY_SPEED 2

typedef void (*CollisionCallback)(GameObject *a, GameObject *b);

typedef struct {
    int x, y;
    int width, height;
    unsigned int color;
    int active;
    CollisionCallback onCollision
} GameObject;

GameObject player;
GameObject bullets[MAX_BULLETS];
GameObject enemies[MAX_ENEMIES];

void checkCollision(GameObject *a, GameObject *b);
void updatePlayer();
void updateBullets();
void updateEnemies();
void handleCollisions();
void drawGameObject(GameObject *obj);
void gameInit();
void gameLoop();