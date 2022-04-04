#ifndef MAIN_H
#define MAIN_H

#include "gba.h"

//images
#include "start.h"
#include "win.h"
#include "enemy1.h"
#include "enemy2.h"
#include "enemy3.h"
#include "heart.h"
#include "lose.h"

//structs
struct collider {
    int rBound;
    int lBound;
    int uBound;
    int dBound;
}; 

struct sprite{
    int x;
    int y;
    int width;
    int height;
    struct collider *collider;
};

struct projectile {
    int size;
    int x;
    int y;
    int speed;
    int active;
    struct collider collider;
};

struct gameState {
    int lives;
    int win;
    int bobSpeed;
    int bobLives;
};

struct projectileTracker{
    int maxProjectiles; 
    int curProjectiles;
    int next;
};



//function declarations
void playGame(u32 currentButtons, u32 previousButtons);
void updatePlayer(struct sprite *player, char dir, int speed);
void updateCollider(struct sprite *sprite);
int checkCollisions(struct sprite *self, struct sprite *other);
int checkColliders(struct collider *sc, struct collider *oc);
void resetGame(void);
void updateBobImage(const u16 *image,int width, int height);
void updateColliderProj(struct projectile *projectile);

#endif
