#include "main.h"

#include <stdio.h>
#include <stdlib.h>

#include "gba.h"

//images
#include "start.h"
#include "win.h"
#include "enemy1.h"
#include "enemy2.h"
#include "enemy3.h"
#include "heart.h"
#include "lose.h"


enum gba_state {
  START,
  PLAY,
  WIN,
  LOSE,

};

//global vars
struct gameState gameState = {3, 0, 1, 3}; //tracks variables in game
struct projectileTracker pt = {3, 0, 0}; //tracks projectile attributes

//colliders
struct collider pCollider; //player collider
struct collider bCollider; //bob collider

//sprites
struct sprite player = {WIDTH/2,HEIGHT/2, 9, 9, &pCollider};
struct sprite bob = {WIDTH/2 + 50, HEIGHT/2 + 50, ENEMY1_WIDTH, ENEMY1_HEIGHT, &bCollider};
const u16 *bobImage = enemy1;

//projectiles
struct projectile projectiles[3];

int main(void) {
  // Manipulate REG_DISPCNT here to set Mode 3. //
  REG_DISPCNT = MODE3 | BG2_ENABLE;

  // Save current and previous state of button input.
  u32 previousButtons = BUTTONS;
  u32 currentButtons = BUTTONS;

  // Load initial application state
  enum gba_state state = START;

  //SET UP GAME 
  //set collider position
  updateCollider(&player);
  updateCollider(&bob);

  //instantiate projectiles
  for (int i = 0; i < pt.maxProjectiles; i++){
    struct collider c = {100, 100, 4, 4}; 
    struct projectile proj = {4, 100, 100, 2, 0, c};
    projectiles[i] = proj;
  } 

  
  while (1) {
    currentButtons = BUTTONS; // Load the current state of the buttons

    //restart game if back is pressed
    if (KEY_JUST_PRESSED(BUTTON_SELECT, currentButtons, previousButtons)){
      state = START;
    }
    switch (state) {
      case START:
        waitForVBlank();
        drawFullScreenImageDMA(start);

        //prepare for game
        if (KEY_JUST_PRESSED(BUTTON_START, currentButtons, previousButtons)){
          waitForVBlank();
          fillScreenDMA(BLACK);
          resetGame();
          state = PLAY;
        }
        break;
      case PLAY:
        if (gameState.win){
          state = WIN;
        }
        if (gameState.lives == 0){
          state = LOSE;
        }
        playGame(currentButtons, previousButtons);
        break;
      case WIN:
        waitForVBlank();
        drawFullScreenImageDMA(win);
        break;
      case LOSE:
        waitForVBlank();
        drawFullScreenImageDMA(lose);
        break;
    }
    previousButtons = currentButtons; // Store the current state of the buttons
  }
  return 0;
}

//resets game variables
void resetGame(void){
  gameState.lives = 3;
  player.x = 120;
  player.y = 80;
  bob.x = 160;
  bob.y = 100;
  updateBobImage(enemy1, ENEMY1_WIDTH, ENEMY1_HEIGHT);
  
  updateCollider(&player);
  gameState.win = 0;
  gameState.bobLives = 3;
  gameState.bobSpeed= 1;

  for (int i = 0; i < pt.maxProjectiles; i++){
    projectiles[i].active = 0;
  }
}

//clears the old position of bob and player
void clearScreen(struct sprite *player, struct sprite *bob){
  drawCenteredRect(player->x, player->y, player->width, player->height, BLACK);
  drawCenteredRect(bob->x, bob->y, bob->width, bob->height, BLACK);
  drawRectDMA(2, 36, 33, HEART_HEIGHT, BLACK);
}

//updates a projectiles collider
void updateColliderProj(struct projectile *projectile){
  int halfSize =  projectile->size / 2;
  projectile->collider.rBound = projectile->x + halfSize;
  projectile->collider.lBound = projectile->x -halfSize;
  projectile->collider.uBound = projectile->y - halfSize;
  projectile->collider.dBound = projectile->y + halfSize;
}

//updates a sprites collider
void updateCollider(struct sprite *sprite){
  int halfWidth = sprite -> width / 2;
  int halfHeight = sprite -> height / 2;
  int x = sprite->x;
  int y = sprite->y;

  sprite->collider->rBound = x + halfWidth;
  sprite->collider->lBound = x - halfWidth;
  sprite->collider->uBound = y - halfHeight;
  sprite->collider->dBound = y + halfHeight;
}

//updates bob's location & checks for collisions b/w walls
void updatebob(struct sprite *bob){
  static int hDir = 1;
  static int vDir = 1;
  if (checkCollisions(bob, &player)) {
    hDir = -hDir;
    vDir = -vDir;
  } else {
    if (bob->collider->lBound <= 0 || bob->collider->rBound >= 239){
      hDir = -hDir;
    }
    if (bob->collider->uBound <= 0 ||bob->collider->dBound >= 160){
      vDir = -vDir;
    }
  }
  bob->x += (hDir * gameState.bobSpeed);
  bob->y += (vDir * gameState.bobSpeed);
  updateCollider(bob);
}

//updates the players location
void updatePlayer(struct sprite *player, char dir, int speed){
  int xMov = 0;
  int yMov = 0;
  switch (dir) {
    case 'R':
      xMov = speed;
      break;
    case 'L':
      xMov -= speed;
    break;
    case 'U':
      yMov -= speed;
    break;
    case 'D':
      yMov += speed;
    break;
  }
  //check if player will collide with bob
  struct collider *pc = player->collider;
  struct collider newPos = {pc->rBound + xMov, pc->lBound + xMov, pc->uBound + yMov, pc->dBound + yMov};
  if(checkColliders(&newPos, bob.collider)){
    return;
  } 
  //check for collision w/ walls
  if (newPos.rBound >= 240 ||  newPos.lBound  < 0){
    xMov = 0;
  }
  if (newPos.dBound >= 160 ||  newPos.uBound < 0){
    yMov = 0;
  }
  player->x += xMov;
  player->y += yMov;
  updateCollider(player);
}

//draws collider bounds on sprites (for debugging)
void drawCollider(struct sprite *sprite){
  setPixel(sprite->y,sprite->x, MAGENTA);
  setPixel(sprite->y,sprite->collider->rBound, CYAN);
  setPixel(sprite->y,sprite->collider->lBound, WHITE);
  setPixel(sprite->collider->uBound, sprite->x,  BLUE);
  setPixel(sprite->collider->dBound, sprite->x, YELLOW);
}

//draws collider bounds for projectile (for debugging)
void drawColliderProj(struct projectile *sprite){
  setPixel(sprite->y,sprite->x, MAGENTA);
  setPixel(sprite->y,sprite->collider.rBound, CYAN);
  setPixel(sprite->y,sprite->collider.lBound, WHITE);
  setPixel(sprite->collider.uBound, sprite->x,  BLUE);
  setPixel(sprite->collider.dBound, sprite->x, YELLOW);
}


//draws lives and hearts
void drawHearts(int lives){
  char lifeStr[] = "lives:";
  drawString(2,0,lifeStr, WHITE);
  for (int i = 0; i < lives; i++){
    drawImageDMA(2, 36 + (i*11), HEART_WIDTH, HEART_HEIGHT, heart);
  }
}

//check collision b/w 2 colliders
int checkColliders(struct collider *sc, struct collider *oc){
  //self bumps other from left
  if (sc->lBound < oc->rBound &&
        sc->rBound > oc->lBound &&
        sc->uBound < oc->dBound &&
        sc->dBound > oc->uBound){
    return 1;
  }
  return 0;
}

//checks for collisions between 2 sprites
int checkCollisions(struct sprite *self, struct sprite *other){
  struct collider *sc = self->collider;
  struct collider *oc = other->collider;
  return checkColliders(sc, oc);
}

//updates number of player's lives on a collision with bob
void updateHearts(void){
  if (checkCollisions(&player, &bob) && gameState.lives > 0){
    (&gameState)->lives = gameState.lives - 1;
  }
}

//draws projectiles 
void updateProjectiles(void){
  for (int i = 0; i < pt.maxProjectiles; i++) {
    //skip projectile if not active
    if (!projectiles[i].active){
      continue;
    }

    //clear old projectile
    drawCenteredRect(projectiles[i].x, projectiles[i].y, projectiles[i].size, projectiles[i].size, BLACK);
    //update proj position
    projectiles[i].y -= projectiles[i].speed;
    updateColliderProj(&projectiles[i]);
  
    //check to see if hit wall & deactive if true
    if(projectiles[i].collider.uBound <= 0){
      projectiles[i].active = 0;
      pt.curProjectiles--;
      continue;
    }
    
    //draw active projectiles
    drawCenteredRect(projectiles[i].x, projectiles[i].y, projectiles[i].size, projectiles[i].size, WHITE);
  }
}

//activates a new projectile
void makeProjectile(void){
 if (pt.curProjectiles < pt.maxProjectiles && !(projectiles[pt.next].active)){
   //set next proj active
    projectiles[pt.next].x = player.x;
    projectiles[pt.next].y = player.y - 4;
    projectiles[pt.next].active = 1;
    pt.curProjectiles += 1;
    updateColliderProj(&projectiles[pt.next]);

    //track the next available space
    if (pt.next == pt.maxProjectiles - 1){
      pt.next = 0;
    } else {
      pt.next++;
    }
  }
}

//updates game if a projectile collides with bob
void checkHit(void){
  for (int i = 0; i < pt.maxProjectiles; i++){
    //if collision b/w player and bob
    if (projectiles[i].active && checkColliders(&projectiles[i].collider, bob.collider)){
      projectiles[i].active = 0;
      pt.curProjectiles--;
      gameState.bobLives--;

      //change bob's mode
      if (gameState.bobLives == 2){
        updateBobImage(enemy2, ENEMY2_WIDTH, ENEMY2_HEIGHT);
        gameState.bobSpeed = 2;
      } else if (gameState.bobLives == 1){
        updateBobImage(enemy3, ENEMY3_WIDTH, ENEMY3_HEIGHT);
        gameState.bobSpeed = 3;
      }
      drawCenteredRect(projectiles[i].x, projectiles[i].y, projectiles[i].size, projectiles[i].size, BLACK);
    }
    //bob's lives = 0 -> move to win state
    if (gameState.bobLives == 0) {
      gameState.win = 1;
      return;
    }
  }
}

//updates the current image of bob and sprite values
void updateBobImage(const u16 *image,int width, int height){
  bobImage = image;
  bob.width = width;
  bob.height = height;
  updateCollider(&bob);
}


//update function for the game
void updateGame(struct sprite *player, struct sprite *bob, char dir){
  waitForVBlank();

  //drawing functions
  clearScreen(player, bob);
  updatePlayer(player, dir, 1);
  updateProjectiles();
  updatebob(bob);
  checkHit();
  updateHearts();

  //draw shapes
  drawCenteredImage(bob->y, bob->x, bob->width, bob->height, bobImage);
  // drawCollider(bob);

  drawCenteredRect(player->x, player->y, player->width, player->height, GREEN);
  // drawCollider(player);
  
  drawHearts(gameState.lives);


  //Debugging projectiles
  // for (int i = 0; i < pt.maxProjectiles; i++){
  //   if (projectiles[i].active){
  //     drawRectDMA(30,5 +(6*i),5,5,CYAN);
  //   }
  //   if (!projectiles[i].active){
  //     drawRectDMA(30,5 +(6*i),5,5,BLUE);
  //   }
  // }
  // for (int i = 0; i < pt.maxProjectiles; i++){
  //   drawRectDMA(40,5 +(6*i),5,5,BLUE);
  // }
  // for (int i = 0; i < pt.next; i++){
  //   drawRectDMA(40,5 +(6*i),5,5,MAGENTA);
  // }

}

//main function for game
void playGame(u32 currentButtons, u32 previousButtons){
  char dir = 'a';
  if (KEY_DOWN(BUTTON_LEFT, currentButtons)){
    dir = 'L';
  } else if (KEY_DOWN(BUTTON_RIGHT, currentButtons)) {
    dir = 'R';
  } else if (KEY_DOWN(BUTTON_UP, currentButtons)){
    dir = 'U';
  } else if (KEY_DOWN(BUTTON_DOWN, currentButtons)){
    dir = 'D';
  } else {

  }
  if(KEY_JUST_PRESSED(BUTTON_L, currentButtons, previousButtons)){
    makeProjectile();
  }
  updateGame(&player, &bob, dir);
  
}
