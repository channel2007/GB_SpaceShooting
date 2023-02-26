// 解析度：160x144

#include <gb/gb.h>
#include <stdio.h>
#include <gb/console.h>
#include <stdio.h>
#include <string.h>
#include <rand.h>

#include "GB-Menu_data.c"
#include "GB-Menu_map.c"

#include "img.c"

#define TIME_MAX            8               // 時脈最大數量.

#define MENU_EFFECT_MAX     5
#define ENEMY_MAX           6               // 敵人最大數量.
#define ENEMY_BULLET_MAX    3               // 敵人子彈.

#define GAME_MODE_MENU       5              // 片頭.
#define GAME_MODE_STARE      6              // 開始遊戲.
#define GAME_MODE_PLAY      10              // 遊戲模式.
#define GAME_MODE_OVER      20              // GameOver模式.

//----------------------------------------------------------------------------
// 特效結構.
//----------------------------------------------------------------------------
struct GameEffects {
    UBYTE use;          // 是否使用.
	UBYTE blockId[5];   // 區塊編號(0~255).    
    INT8  nb;           // 精靈編號(0~39).
    UBYTE ani;          // 正在播放的區塊編號.
	UINT8 x;            // 精靈X座標.
	UINT8 y;            // 精靈Y座標.
	UINT8 width;        // 精靈寬.
	UINT8 height;       // 精靈高.
};
struct  GameEffects menuEffects[MENU_EFFECT_MAX];


//----------------------------------------------------------------------------
// 角色結構.
//----------------------------------------------------------------------------
struct GameCharacter {
    UBYTE use;          // 是否使用.
	UBYTE blockId[2];   // 區塊編號(0~255).    
    INT8  nb;           // 精靈編號(0~39).
    UBYTE ani;          // 正在播放的區塊編號.
	UINT8 x;            // 精靈X座標.
	UINT8 y;            // 精靈Y座標.
	UINT8 width;        // 精靈寬.
	UINT8 height;       // 精靈高.
    UINT8 score;        // 得分.
};
struct GameCharacter enemy1[ENEMY_MAX];                 // 敵人1.
struct GameCharacter enemy2[ENEMY_MAX*2];               // 敵人2.
struct GameCharacter enemy3[ENEMY_MAX*2];               // 敵人3.
struct GameCharacter enemyBullet[ENEMY_BULLET_MAX];     // 敵人子彈.


struct GameCharacter ufo;                               // UFO.
struct GameCharacter role;                              // 主角.
struct GameCharacter roleTitle;                         // 抬頭主角.
struct GameCharacter roleBullet;                        // 主角子彈.
struct GameCharacter boom;                              // 爆炸.

// 輸入.
joypads_t joypads;

// 時脈.
unsigned int vbl_cnt;

// 0:遊戲-敵人動畫時脈、目錄-字閃爍.
// 1:敵人子彈時脈.
// 2:爆炸停留時間.
// 3:UFO移動速度.
// 4:UFO出現時間.
// 5:敵人移動時脈.
// 6:敵人子彈發射時脈.
// 7:GAME OVER後自動玩時間.
unsigned int clock_cnt[TIME_MAX];

// 分數.
unsigned int score = 0;
unsigned int hiScore = 0;

// 主角數量.
UBYTE roleNumber = 2;
// 敵人數量.
UBYTE enemyNumber = ENEMY_MAX*5;
// 敵人移動方向(0:右移 1:左移).
UBYTE enemyDir = 0;
// 敵人移動列.
BYTE enemyMoveCol = 4;
// 敵人移動群組座標(用在判斷敵人群下降用).
UINT8 enemyGroupX=0;
UINT8 enemyGroupY=0;

// 遊戲模式.
//UBYTE gameMode = GAME_MODE_PLAY;
UBYTE gameMode = GAME_MODE_MENU;

// 主角動畫.
UBYTE roleAni = 0;

// 自動玩.
// 0:無.
// 1:自動玩.
UBYTE autoplay = 0;

// 自動玩亂數位置.
UINT16 roleRandX = 0;

//extern const unsigned char * song_Data[];

//----------------------------------------------------------------------------
// 大敵軍、主角爆炸音效.
//----------------------------------------------------------------------------
void playSound01(){
    if(autoplay==1){return;}

    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    NR52_REG = 0x80;

    NR41_REG = 0x18;
    NR42_REG = 0xF2;
    NR43_REG = 0x5F;
    NR44_REG = 0xC0;
}

//----------------------------------------------------------------------------
// 小敵軍爆炸音效.
//----------------------------------------------------------------------------
void playSound02(){
    if(autoplay==1){return;}

    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    NR52_REG = 0x80;

    NR41_REG = 0x28;
    NR42_REG = 0x82;
    NR43_REG = 0x11;
    NR44_REG = 0x80;
}

//----------------------------------------------------------------------------
// 發射子彈音效.
//----------------------------------------------------------------------------
void playSound03(){
    if(autoplay==1){return;}

    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    NR52_REG = 0x80;

    NR10_REG = 0x3D; 
    NR11_REG = 0xC3;
    NR12_REG = 0x33;
    NR13_REG = 0x6D;
    NR14_REG = 0x86;
}

//----------------------------------------------------------------------------
// 開始遊戲音效.
//----------------------------------------------------------------------------
void playSound04(){
    if(autoplay==1){return;}

    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    NR52_REG = 0x80;

    NR10_REG = 0x1D; 
    NR11_REG = 0x8E;
    NR12_REG = 0x87;
    NR13_REG = 0x41;
    NR14_REG = 0x86;
}

//----------------------------------------------------------------------------
// 累加時脈.
//----------------------------------------------------------------------------
void clockIVbl(){ vbl_cnt++;}

//----------------------------------------------------------------------------
// 設定時脈.
// id :時脈編號(200:全部時脈).
//----------------------------------------------------------------------------
void clockSet(UBYTE id){
    unsigned int cnt;    
    CRITICAL { cnt = vbl_cnt;}
    if(id==200){        
        for(int i=0; i<TIME_MAX; i++){
            clock_cnt[i] = cnt;
        }
    }else{
        clock_cnt[id] = cnt;
    }
}

//----------------------------------------------------------------------------
// 初始時脈.
//----------------------------------------------------------------------------
void clockInit(){
    CRITICAL {
        vbl_cnt = 0;
        add_VBL(clockIVbl);
    }
    TMA_REG = 0x00U;
    TAC_REG = 0x04U;
    set_interrupts(VBL_IFLAG);    
    clockSet(200);
}

//----------------------------------------------------------------------------
// 更新時脈.
//----------------------------------------------------------------------------
UBYTE clockGet(unsigned int id, unsigned int tm){
    unsigned int cnt, d;
    CRITICAL { cnt = vbl_cnt;}
    d = cnt - clock_cnt[id];
    if(d>=tm){
        clock_cnt[id] = cnt;
        return 1;
    }else{
        return 0;
    }
}

//----------------------------------------------------------------------------
// 開啟精靈.
//----------------------------------------------------------------------------
void showSprite(struct GameCharacter* character, UINT8 x, UINT8 y){
    character->use = 1;
    character->x = x;
    character->y = y;
    move_sprite( character->nb, character->x, character->y);
}

//----------------------------------------------------------------------------
// 關閉精靈.
// character:精靈結構.
// two:兩個區塊結構的精靈.
//----------------------------------------------------------------------------
void hideSprite(struct GameCharacter* character, UBYTE two){
    character->use = 0;
    character->x = 200;
    character->y = 200;
    move_sprite( character->nb, character->x, character->y);
    if(two==1)
        move_sprite( character->nb+1, character->x, character->y);
}

//----------------------------------------------------------------------------
// 增加分數.
//----------------------------------------------------------------------------
void addScore(UINT8 s){
    score += s;
    gotoxy( 1, 1); printf( "%d", score);
    gotoxy( 7, 1); printf( "%d", hiScore);
    gotoxy(17, 1); printf("x%d", roleNumber);
}

//----------------------------------------------------------------------------
// 遊戲結束.
//----------------------------------------------------------------------------
void gameOver(){
    gotoxy(5, 8); printf("GAME OVER");
    
    // 紀錄最高分數.
    if(score > hiScore){
        hiScore = score;
        addScore(0);
    }
    // 關閉敵人.
    for(UBYTE i=0; i<ENEMY_MAX; i++){        
        hideSprite(&enemy1[i], 0);
        hideSprite(&enemy2[i], 0);
        hideSprite(&enemy2[i+ENEMY_MAX], 0);
        hideSprite(&enemy3[i], 0);
        hideSprite(&enemy3[i+ENEMY_MAX], 0);
    }
    // 關閉敵人子彈.
    for(UBYTE i=0; i<ENEMY_BULLET_MAX; i++)
        hideSprite(&enemyBullet[i], 0);

    // 關閉主角子彈.
    roleBullet.use = 0;
    hideSprite(&roleBullet,0);

    // 關閉精靈.
    hideSprite(&ufo, 1);
    hideSprite(&role, 1);
    hideSprite(&roleBullet, 0);
    hideSprite(&boom, 0);

    // 初始時脈.
    clockSet(7);

    // 設定GameOver模式.
    gameMode = GAME_MODE_OVER;
}

//----------------------------------------------------------------------------
// 設定爆炸位置.
//----------------------------------------------------------------------------
void boomSet(UINT8 x, UINT8 y){
    boom.use = 1;
    boom.x = x;
    boom.y = y;    
    move_sprite(boom.nb, boom.x, boom.y);
    clockSet(2);    
}

//----------------------------------------------------------------------------
// 爆炸更新.
//----------------------------------------------------------------------------
void boomUpdate(){
    if(boom.use == 1 && clockGet(2, 30))
        hideSprite(&boom,0);
}


//----------------------------------------------------------------------------
// Press Start文字精靈.
//----------------------------------------------------------------------------
void menuFont(){
    // .
    for(UBYTE i=0; i<7; i++){
        enemy1[i].use = 1;        
        enemy1[i].width = 8;
        enemy1[i].height = 8;
        //(52,105)
        enemy1[i].x =  60 + (enemy1[i].width*i);
        enemy1[i].y = 121;
        enemy1[i].nb = i;
        enemy1[i].blockId[0] = 15+i;
        enemy1[i].blockId[1] = 15+i;
        enemy1[i].ani = 1;
        enemy1[i].score = 0;
        set_sprite_tile( i, enemy1[i].blockId[0]);
        move_sprite( enemy1[i].nb, enemy1[i].x, enemy1[i].y);
    }
}

//----------------------------------------------------------------------------
// 初始目錄精靈.
//----------------------------------------------------------------------------
void menuInit(){
    // 文字精靈(6).
    menuFont();

    /*
    //--------------------------------------------------------------------
    // 特效.
    for(UBYTE i=0; i<5; i++){
        menuEffects[i].use = 1;        
        menuEffects[i].width = 8;
        menuEffects[i].height = 8;
        menuEffects[i].x = 50;
        menuEffects[i].y = 28;
        menuEffects[i].nb = i+7;
        menuEffects[i].blockId[0] = 22;
        menuEffects[i].blockId[1] = 23;
        menuEffects[i].blockId[2] = 24;
        menuEffects[i].blockId[3] = 25;
        menuEffects[i].blockId[4] = 26;
               
        // 0.
        if(i==0){
            menuEffects[i].ani = 1;
            menuEffects[i].x = 50;
            menuEffects[i].y = 28;
        // 1.
        }else if(i==1){
            menuEffects[i].ani = 0;
            menuEffects[i].x = 32;
            menuEffects[i].y = 58;
        // 2.
        }else if(i==2){
            menuEffects[i].ani = 2;
            menuEffects[i].x = 24;
            menuEffects[i].y = 84;
        // 3.
        }else if(i==3){
            menuEffects[i].ani = 1;
            menuEffects[i].x = 139;
            menuEffects[i].y = 54;
        // 4.
        }else if(i==4){
            menuEffects[i].ani = 3;
            menuEffects[i].x = 142;
            menuEffects[i].y = 76;

        }
        set_sprite_tile( menuEffects[i].nb, menuEffects[i].blockId[menuEffects[i].ani]);
        move_sprite( menuEffects[i].nb, menuEffects[i].x, menuEffects[i].y);

    }
    */
}

//----------------------------------------------------------------------------
// 更新特效動畫.
//----------------------------------------------------------------------------
void menuAni(struct GameEffects* effects){
    if(effects->use == 1){        
        effects->ani++; 
        if(effects->ani > 4)
            effects->ani = 0;        
        set_sprite_tile( effects->nb, effects->blockId[effects->ani]);
    }
}

//----------------------------------------------------------------------------
// // 主目錄.
//----------------------------------------------------------------------------
UBYTE menuFS=1;
UBYTE menuStareFs=0;
UBYTE menuLoop=1;
void MenuLoop(){
    // 設定背景(填入背景資料).
    set_bkg_data(0, 85, Menu_data);
    set_bkg_tiles(0, 0, 20, 18, Menu_map);
    SHOW_BKG;
    DISPLAY_ON;

    // 初始目錄精靈.    
    menuInit();
    SHOW_SPRITES;

    // 等待按開始繼續.
    //waitpad(J_START);

    while(menuLoop){ 
        // poll joypads
        joypad_ex(&joypads);

        /*
        // 特效.      
        if(clockGet(1, 7)){
            // 更新特效動畫.
            for(UBYTE i=0; i<5; i++){
                menuAni(&menuEffects[i]);
            }            
        }
        */

        // 片頭.
        if(gameMode == GAME_MODE_MENU){
            // 閃爍字      
            if(clockGet(0, 60)){
                menuFS++;
                if(menuFS>1)
                    menuFS = 0;
                if(menuFS==0){
                    // 關閉.
                    for(UBYTE i=0; i<7; i++){
                        hideSprite(&enemy1[i], 0);
                    }
                }else{
                    // Press Start文字精靈.
                    menuFont();
                }
            }

            // 開始按鈕.
            if (joypads.joy0 & J_START) {
                gameMode = GAME_MODE_STARE;
                // 開始遊戲音效.
                playSound04();
            }

        // 開始遊戲. 
        }else if(gameMode == GAME_MODE_STARE){
            // 快速閃爍字      
            if(clockGet(0, 5)){
                menuFS++;
                if(menuFS>1)
                    menuFS = 0;
                if(menuFS==0){
                    // 關閉.
                    for(UBYTE i=0; i<7; i++){
                        hideSprite(&enemy1[i], 0);
                    }
                }else{
                    // Press Start文字精靈.
                    menuFont();
                }                
                menuStareFs++;
                if(menuStareFs>10){
                    // 進入遊戲.
                    gameMode = GAME_MODE_PLAY;
                    menuLoop = 0;
                    HIDE_SPRITES;
                }
            }
        }
    }

}

//----------------------------------------------------------------------------
// 初始敵人.
// v:是否顯示精靈.
// n:是否重製主角位置.
//----------------------------------------------------------------------------
void enemyInit(UBYTE v, UBYTE n){
    UINT8 dx;

    // 敵人.
    for(UBYTE i=0; i<ENEMY_MAX; i++){
        //--------------------------------------------------------------------
        // 1.
        enemy1[i].use = v;        
        enemy1[i].width = 8;
        enemy1[i].height = 8;
        enemy1[i].x = 50 + ((enemy1[i].width + 6)*i);
        enemy1[i].y = 42;                
        enemy1[i].nb = i;
        enemy1[i].blockId[0] = 1;
        enemy1[i].blockId[1] = 2;
        enemy1[i].ani = 1;
        enemy1[i].score = 3;
        set_sprite_tile( i, enemy1[i].blockId[0]);

        //--------------------------------------------------------------------
        // 2.
        enemy2[i].use = v;
        enemy2[i].width = 8;
        enemy2[i].height = 8;
        enemy2[i].x = 50 + ((enemy2[i].width + 6)*i);
        enemy2[i].y = 54;
        dx = i + ENEMY_MAX;    
        enemy2[i].nb = dx;    
        enemy2[i].blockId[0] = 3;
        enemy2[i].blockId[1] = 4;
        enemy2[i].ani = 3;
        enemy2[i].score = 2;
        set_sprite_tile(dx, enemy2[i].blockId[0]);

        // 3
        enemy2[i+ENEMY_MAX].use = v;
        enemy2[i+ENEMY_MAX].width = 8;
        enemy2[i+ENEMY_MAX].height = 8;
        enemy2[i+ENEMY_MAX].x = 50 + ((enemy2[i+ENEMY_MAX].width + 6)*i);
        enemy2[i+ENEMY_MAX].y = 66;
        dx = i + (ENEMY_MAX*2);    
        enemy2[i+ENEMY_MAX].nb = dx;    
        enemy2[i+ENEMY_MAX].blockId[0] = 3;
        enemy2[i+ENEMY_MAX].blockId[1] = 4;
        enemy2[i+ENEMY_MAX].ani = 3;
        enemy2[i+ENEMY_MAX].score = 2;
        set_sprite_tile(dx, enemy2[i+ENEMY_MAX].blockId[0]);

        //--------------------------------------------------------------------
        // 4.
        enemy3[i].use = v;
        enemy3[i].width = 8;
        enemy3[i].height = 8;
        enemy3[i].x = 50 + ((enemy3[i].width + 6)*i);
        enemy3[i].y = 78;
        dx = i + (ENEMY_MAX*3);
        enemy3[i].nb = dx;    
        enemy3[i].blockId[0] = 5;
        enemy3[i].blockId[1] = 6;
        enemy3[i].ani = 5;
        enemy3[i].score = 1;
        set_sprite_tile(dx, enemy3[i].blockId[0]);

        // 5.
        enemy3[i+ENEMY_MAX].use = v;
        enemy3[i+ENEMY_MAX].width = 8;
        enemy3[i+ENEMY_MAX].height = 8;
        enemy3[i+ENEMY_MAX].x = 50 + ((enemy3[i].width + 6)*i);
        enemy3[i+ENEMY_MAX].y = 90;
        dx = i + (ENEMY_MAX*4);
        enemy3[i+ENEMY_MAX].nb = dx;    
        enemy3[i+ENEMY_MAX].blockId[0] = 5;
        enemy3[i+ENEMY_MAX].blockId[1] = 6;
        enemy3[i+ENEMY_MAX].ani = 5;
        enemy3[i+ENEMY_MAX].score = 1;
        set_sprite_tile(dx, enemy3[i+ENEMY_MAX].blockId[0]);
        
        //--------------------------------------------------------------------
        // 顯示精靈。
        if(v==1){
            // 設定精靈位置.
            move_sprite( enemy1[i].nb, enemy1[i].x, enemy1[i].y);
            move_sprite( enemy2[i].nb, enemy2[i].x, enemy2[i].y);
            move_sprite( enemy2[i+ENEMY_MAX].nb, enemy2[i+ENEMY_MAX].x, enemy2[i+ENEMY_MAX].y);
            move_sprite( enemy3[i].nb, enemy3[i].x, enemy3[i].y);
            move_sprite( enemy3[i+ENEMY_MAX].nb, enemy3[i+ENEMY_MAX].x, enemy3[i+ENEMY_MAX].y);
        }

    }
    enemyGroupX = enemy1[0].x;
    enemyGroupY = enemy1[0].y;

    //------------------------------------------------------------------------
    // 敵人子彈.
    for(UBYTE i=0; i<ENEMY_BULLET_MAX; i++){
        enemyBullet[i].use = 0;
        enemyBullet[i].width = 3;
        enemyBullet[i].height = 8;
        enemyBullet[i].x = 20;
        enemyBullet[i].y = 100;                
        enemyBullet[i].nb = 34+i;               // 34~36.
        enemyBullet[i].blockId[0] = 0;
        enemyBullet[i].blockId[1] = 0;
        enemyBullet[i].ani = 0;
        set_sprite_tile( enemyBullet[i].nb, enemyBullet[i].blockId[0]);        
    }
        
    //------------------------------------------------------------------------
    // ufo.
    ufo.use = 0;
    ufo.width = 16;
    ufo.height = 8;
    ufo.x = 10;
    ufo.y = 32;
    ufo.nb = 30;
    ufo.blockId[0] = 7;
    ufo.blockId[1] = 8;
    ufo.ani = 7;
    ufo.score = 5;
    set_sprite_tile(30, ufo.blockId[0]);
    set_sprite_tile(31, ufo.blockId[1]);

    //------------------------------------------------------------------------
    // 主角抬頭.
    roleTitle.use = 1;
    roleTitle.width = 16;
    roleTitle.height = 8;
    roleTitle.x = 135;
    roleTitle.y = 22;
    roleTitle.nb = 39;    
    roleTitle.blockId[0] = 14;
    roleTitle.blockId[1] = 14;
    roleTitle.ani = 14;    
    set_sprite_tile(roleTitle.nb, roleTitle.blockId[0]);
    move_sprite( roleTitle.nb,   roleTitle.x, roleTitle.y);
    
    //------------------------------------------------------------------------
    // 主角.
    role.use = v;
    role.width = 16;
    role.height = 8;
    if(n==1){
        role.x = 80;
        role.y = 150;
    }
    role.nb = 32;
    role.blockId[0] = 9;
    role.blockId[1] = 10;
    role.ani = 9;
    set_sprite_tile(32, role.blockId[0]);
    set_sprite_tile(33, role.blockId[1]);
    if(v==1){
        move_sprite( 32,   role.x, role.y);
        move_sprite( 33, role.x+8, role.y);
    }

    //------------------------------------------------------------------------
    // 主角子彈.
    roleBullet.use = 0;
    roleBullet.width =  1;
    roleBullet.height = 4;
    roleBullet.x = role.x+7;
    roleBullet.y = role.y-6;
    roleBullet.nb= 37;
    roleBullet.blockId[0] = 11;
    roleBullet.blockId[1] = 12;
    roleBullet.ani = 11;
    set_sprite_tile(roleBullet.nb, roleBullet.blockId[0]);

    //------------------------------------------------------------------------
    // 爆炸.
    boom.use = 0;
    boom.width = 8;
    boom.height = 8;
    boom.x = 100;
    boom.y = 50;
    boom.nb= 38;
    boom.blockId[0] = 13;
    boom.ani = 13;
    set_sprite_tile(boom.nb, boom.blockId[0]);
}

//----------------------------------------------------------------------------
// 更新敵人動畫.
//----------------------------------------------------------------------------
void enemyAni(struct GameCharacter* character,  UBYTE ani){
    if(character->use == 1){
        if(character->ani == ani){
            set_sprite_tile( character->nb, character->blockId[1]);            
            character->ani = character->blockId[1];
        }else{
            set_sprite_tile( character->nb, character->blockId[0]);
            character->ani = character->blockId[0];
        }
    }
}

//----------------------------------------------------------------------------
// 更新所有敵人動畫.
//----------------------------------------------------------------------------
void enemyAniUpdate(){
    
    // 敵人.
    for(UBYTE i=0; i<ENEMY_MAX; i++){        
        // 1.
        enemyAni(&enemy1[i], 1);
        // 2.
        enemyAni(&enemy2[i], 3);
        // 3.
        enemyAni(&enemy2[i+ENEMY_MAX], 3);
        // 4.
        enemyAni(&enemy3[i], 5);
        // 5.
        enemyAni(&enemy3[i+ENEMY_MAX], 5);
    }
}

//----------------------------------------------------------------------------
// 更新敵人子彈動畫.
//----------------------------------------------------------------------------
void enemyBulletUpdate(){    
    // 敵人子彈.
    for(UBYTE i=0; i<ENEMY_BULLET_MAX; i++){
        if(enemyBullet[i].use==1){
            // 子彈動畫.
            if(enemyBullet[i].ani == 11)
                enemyBullet[i].ani = enemyBullet[i].blockId[1];
            else                
                enemyBullet[i].ani = enemyBullet[i].blockId[0];            
            set_sprite_tile( enemyBullet[i].nb, enemyBullet[i].ani);
        }
    }

}

//----------------------------------------------------------------------------
// 初始敵人位置.
//----------------------------------------------------------------------------
void enemyPos(struct GameCharacter* character,  UINT8 x, UINT8 y){
    if(character->use == 1){
        character->x = x;
        character->y = y;                
        move_sprite( character->nb, character->x, character->y);
    }
}

//----------------------------------------------------------------------------
// 初始所有敵人位置.
//----------------------------------------------------------------------------
void enemyPosReset(){    
    // 敵人.
    for(UBYTE i=0; i<ENEMY_MAX; i++){
        // 1.
        enemyPos( &enemy1[i],  50 + ((enemy1[i].width + 6)*i), 42);
        // 2.
        enemyPos( &enemy2[i],  50 + ((enemy2[i].width + 6)*i), 54);
        // 3.
        enemyPos( &enemy2[i+ENEMY_MAX],  50 + ((enemy2[i+ENEMY_MAX].width + 6)*i), 66);
        // 4.
        enemyPos( &enemy3[i],  50 + ((enemy3[i].width + 6)*i), 78);
        // 5.
        enemyPos( &enemy3[i+ENEMY_MAX], 50 + ((enemy3[i].width + 6)*i), 90);

    }
    enemyGroupX = 50;
    enemyGroupY = 42;
}

//----------------------------------------------------------------------------
// 移動敵人子彈.
//----------------------------------------------------------------------------
void enemyBulletMove(){    
    UBYTE s,s1;
    unsigned int t;
    //printf("%d \n", x);

    // 敵人子彈.    
    for(UBYTE i=0; i<ENEMY_BULLET_MAX; i++){
        //.
        if(enemyBullet[i].use==1){
            // 子彈往下移動.
            enemyBullet[i].y+=1;
            if(enemyBullet[i].y>160)
                hideSprite(&enemyBullet[i], 0);

            // 判斷子彈是否有碰到主角.
            if( (enemyBullet[i].x>=role.x)&&(enemyBullet[i].x<= (role.x+role.width))&&
                (enemyBullet[i].y>=role.y)&&(enemyBullet[i].y<=(role.y+role.height))&&(autoplay == 0)){
                    // 播放音效.
                    playSound01();
                    // 關閉所有子彈.
                    for(UBYTE j=0; j<ENEMY_BULLET_MAX; j++)
                        hideSprite(&enemyBullet[j], 0);
                    // 設定爆炸位置.
                    boomSet(role.x+4, role.y);
                    // GameOver.
                    if(roleNumber==0){
                        gameOver();
                        return;
                    }
                    // 初始敵人位置.
                    enemyPosReset();
                    // 扣除主角數量.
                    roleNumber--;                    
                    // 重新定位主角位置.
                    role.x = 80;
                    role.y = 150;
                    move_sprite( 32,   role.x, role.y);
                    move_sprite( 33, role.x+8, role.y);
                    // 更新主角數量.
                     addScore(0);
                    return;
            }

            // 設定子彈位置.
            move_sprite( enemyBullet[i].nb, enemyBullet[i].x, enemyBullet[i].y);

        // 敵人發射子彈.
        }else{            
            // 依照敵人數量設定敵人子彈發射頻率.
            if(enemyNumber > ENEMY_MAX*4)
                t = 100;
            else if(enemyNumber > ENEMY_MAX*3)
                t = 80;
            else if(enemyNumber > ENEMY_MAX*2)
                t = 20;
            else if(enemyNumber > ENEMY_MAX)
                t = 10;
            else
                t =  5;
            if(clockGet(6, t)){
                s = (UBYTE)rand() % 3;
                // 敵人1.
                if(s==0){
                    s1 = (UBYTE)rand() % ENEMY_MAX;
                    if(enemy1[s1].use==1){
                        enemyBullet[i].x = enemy1[s1].x+4;
                        enemyBullet[i].y = enemy1[s1].y;
                        enemyBullet[i].use = 1;
                    }
                // 敵人2.
                }else if(s==1){
                    s1 = (UBYTE)rand() % (ENEMY_MAX*2);
                    if(enemy2[s1].use==1){
                        enemyBullet[i].x = enemy2[s1].x+4;
                        enemyBullet[i].y = enemy2[s1].y;
                        enemyBullet[i].use = 1;
                    }
                // 敵人3.
                }else if(s==2){
                    s1 = (UBYTE)rand() % (ENEMY_MAX*2);
                    if(enemy2[s1].use==1){
                        enemyBullet[i].x = enemy3[s1].x+4;
                        enemyBullet[i].y = enemy3[s1].y;
                        enemyBullet[i].use = 1;                                                
                    }
                }
                // UFO出現.                        
                if(ufo.use == 1){
                    if(enemyBullet[i].use==0){
                        enemyBullet[i].x = ufo.x+10;
                        enemyBullet[i].y = ufo.y+4;
                        enemyBullet[i].use = 1;
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
// 敵人下降.
//----------------------------------------------------------------------------
void enemyDown(){
    // 敵人移動列.
    enemyMoveCol--;
    if(enemyMoveCol<0){
        enemyMoveCol = 4;
        // 下降反轉.
        //gotoxy(1, 3); printf("%d", enemy1[0].x);
        if(enemyGroupX<=16 || enemyGroupX>=80){
            // 反轉.
            enemyDir = !enemyDir;
            // 下降.
            for(UBYTE i=0; i<ENEMY_MAX; i++){
                // 1.
                enemy1[i].y+=12;
                move_sprite(enemy1[i].nb,  enemy1[i].x, enemy1[i].y);
                if(enemy1[i].use==1&&enemy1[i].y>=150){gameOver(); return;} 

                // 2.
                enemy2[i].y+=12;
                move_sprite(enemy2[i].nb,  enemy2[i].x, enemy2[i].y);
                if(enemy2[i].use==1&&enemy2[i].y>=150){gameOver(); return;} 

                // 3.
                enemy2[i+ENEMY_MAX].y+=12;
                move_sprite(enemy2[i+ENEMY_MAX].nb,  enemy2[i+ENEMY_MAX].x, enemy2[i+ENEMY_MAX].y);
                if(enemy2[i+ENEMY_MAX].use==1&&enemy2[i+ENEMY_MAX].y>=150){gameOver(); return;} 

                // 4.
                enemy3[i].y+=12;
                move_sprite(enemy3[i].nb,  enemy3[i].x, enemy3[i].y);
                if(enemy3[i].use==1&&enemy3[i].y>=150){gameOver(); return;} 

                // 5.
                enemy3[i+ENEMY_MAX].y+=12;
                move_sprite(enemy3[i+ENEMY_MAX].nb,  enemy3[i+ENEMY_MAX].x, enemy3[i+ENEMY_MAX].y);
                if(enemy3[i+ENEMY_MAX].use==1&&enemy3[i+ENEMY_MAX].y>=150){gameOver(); return;} 
            }
            enemyGroupY+=12;
        }                
    }
}


//----------------------------------------------------------------------------
// 移動敵人.
//----------------------------------------------------------------------------
void enemyMove(struct GameCharacter* character, BYTE add){
    if(character->use==1){
        character->x += add;
        move_sprite(character->nb,  character->x, character->y);
    }    
}

//----------------------------------------------------------------------------
// 移動所有敵人.
//----------------------------------------------------------------------------
void enemysMove(){
    unsigned int t;

    // 依照敵人數量控制敵人移動速度.
    if(enemyNumber > ENEMY_MAX*4)
        t = 12;
    else if(enemyNumber > ENEMY_MAX*3)
        t = 8;
    else if(enemyNumber > ENEMY_MAX*2)
        t = 4;
    else if(enemyNumber > ENEMY_MAX)
        t = 2;
    else
        t = 1;
    //gotoxy(1, 3); printf("%d", t);    
    if(clockGet(5, t)){
        // 敵人移動方向(0:右移 1:左移).
        if(enemyDir == 0){
            // 5.
            if(enemyMoveCol==4){                
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy3[i+ENEMY_MAX], 2);
            // 4.
            }else if(enemyMoveCol==3){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy3[i],2);
            // 3.
            }else if(enemyMoveCol==2){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy2[i+ENEMY_MAX],2);
            // 2.
            }else if(enemyMoveCol==1){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy2[i],2);
            // 1.
            }else if(enemyMoveCol==0){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy1[i],2);
                enemyGroupX += 2;
            }
            // 敵人下降.
            enemyDown();
        // 左移.
        }else{
            // 5.
            if(enemyMoveCol==4){                
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy3[i+ENEMY_MAX],-2);
            // 4.
            }else if(enemyMoveCol==3){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy3[i],-2);
            // 3.
            }else if(enemyMoveCol==2){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy2[i+ENEMY_MAX],-2);
            // 2.
            }else if(enemyMoveCol==1){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy2[i],-2);
            // 1.
            }else if(enemyMoveCol==0){
                for(UBYTE i=0; i<ENEMY_MAX; i++)
                    enemyMove(&enemy1[i],-2);
                enemyGroupX -= 2;
            }
            // 敵人下降.
            enemyDown();
        }        
    }
}

//----------------------------------------------------------------------------
// 主角發射子彈.
//----------------------------------------------------------------------------
void roleBulletFire(){
    if(roleBullet.use == 0){
        // 發射子彈音效.
        playSound03();
        roleBullet.use = 1;
        roleBullet.x = role.x+7;
        roleBullet.y = role.y-3;
    }
}

//----------------------------------------------------------------------------
// 下一關遊戲.
// n:是否重製主角位置.
//----------------------------------------------------------------------------
void nextGame(UBYTE n){
    // 關閉UFO.
    hideSprite(&ufo,1);
    // 關閉爆炸.
    hideSprite(&boom,0);
    // 關閉敵人子彈.
    for(UBYTE i=0; i<ENEMY_BULLET_MAX; i++)
        hideSprite(&enemyBullet[i], 0);
    // 敵人數量.
    enemyNumber = ENEMY_MAX*5;
    // 初始敵人.
    enemyInit(1,n);
}

//----------------------------------------------------------------------------
// 判斷主角子彈是否打中敵人.
//----------------------------------------------------------------------------
UBYTE roleBulletCollision(struct GameCharacter* character){
    if(character->use == 1){
        if( (roleBullet.x>=character->x)&&(roleBullet.x<=(character->x+character->width))&&
            (roleBullet.y>=character->y)&&(roleBullet.y<=(character->y+character->height))){
            // 播放音效.
            playSound02();            
            // 關閉子彈.
            roleBullet.use = 0;
            hideSprite(&roleBullet,0);
            // 設定爆炸位置.
            boomSet(character->x, character->y);
            // 關閉敵人.
            hideSprite(character,0);                    
            // 加分數.
            addScore(character->score);
            // 扣除敵人數量.
            enemyNumber--;
            if(enemyNumber<=0)
                nextGame(0);        // 重新開始遊戲.
            return 1;
        }
    }
    return 0;
}

//----------------------------------------------------------------------------
// 移動主角子彈.
//----------------------------------------------------------------------------
void roleBulletMove(){
    if(roleBullet.use == 1){
        // 移動子彈.
        roleBullet.y -= 2;
        if(roleBullet.y<=3){
            // 關閉子彈.
            roleBullet.use = 0;
            hideSprite(&roleBullet,0);
            return;
        }

        // 子彈動畫.
        if(clockGet(1, 8)){
            if( roleBullet.ani == roleBullet.blockId[0]){
                roleBullet.ani = roleBullet.blockId[1];
            }else{
                roleBullet.ani = roleBullet.blockId[0];
            }
            set_sprite_tile(roleBullet.nb, roleBullet.ani);
        }

        // 判斷是否打到UFO.
        if(ufo.use == 1){
            if( (roleBullet.x>=ufo.x)&&(roleBullet.x<=(ufo.x+ufo.width))&&
                (roleBullet.y>=ufo.y)&&(roleBullet.y<=(ufo.y+ufo.height))){
                // 播放音效.
                playSound01();
                // 關閉子彈.
                roleBullet.use = 0;
                hideSprite(&roleBullet,0);
                // 設定爆炸位置.
                boomSet(ufo.x+4, ufo.y);
                // 關閉UFO.
                hideSprite(&ufo,1);
                // 初始UFO出現時間.
                clockSet(4);
                // 加分數.
                addScore(ufo.score);                
                return;
            }
        }

        // 判斷是否打到敵人.
        for(UBYTE i=0; i<ENEMY_MAX; i++){
            // 1.
            if(roleBulletCollision(&enemy1[i])==1)
                return;
            // 2.
            if(roleBulletCollision(&enemy2[i])==1)
                return;
            // 3.
            if(roleBulletCollision(&enemy2[i+ENEMY_MAX])==1)
                return;
            // 4.
            if(roleBulletCollision(&enemy3[i])==1)
                return;
            // 5.
            if(roleBulletCollision(&enemy3[i+ENEMY_MAX])==1)
                return;
        }
        // 顯示子彈位置.
        move_sprite(roleBullet.nb,  roleBullet.x, roleBullet.y);
    }
}

//----------------------------------------------------------------------------
// 更新UFO.
//----------------------------------------------------------------------------
void ufoUpdate(){
    //UINT16 s;
    if(clockGet(3, 2)){
        // 啟動.    
        if(ufo.use == 0){
            // 亂數出現ufo.
            //s = (UINT16)rand() % 20;
            if(clockGet(4, 360)){
                ufo.use = 1;
                ufo.x = 10;
                ufo.y = 32;
                // 初始時脈.
                clockSet(3);
            }
        // 移動.
        }else{
            // 移動時脈.        
            ufo.x += 1;
            // 關閉精靈.
            if(ufo.x > 160){
                hideSprite(&ufo,0);
                clockSet(4);
            }
            move_sprite( 30, ufo.x, ufo.y);
            move_sprite( 31, ufo.x+8, ufo.y);            
        }   
    } 
}

//----------------------------------------------------------------------------
// 開始遊戲.
//----------------------------------------------------------------------------
void gameStart(){    
    // 分數.
    score = 0;
    // 初始主角數量.
    roleNumber = 2;
    // 更新主角數量.
    gotoxy( 1, 1); printf( "     ");
    addScore(0);                
    // 繼續遊戲.                
    nextGame(1);
    // 初始時脈.
    clockSet(200);
    // 開始遊戲.
    gotoxy(5, 8); printf("         ");
    gotoxy(5, 13); printf("           ");

    // 關閉主角子彈.
    roleBullet.use = 0;
    roleBullet.x =   0;
    roleBullet.y = -100;
    // 顯示子彈位置.
    move_sprite(roleBullet.nb,  roleBullet.x, roleBullet.y);

    roleRandX = role.x;
    gameMode = GAME_MODE_PLAY;  
}

//----------------------------------------------------------------------------
// 主角移動畫.
//----------------------------------------------------------------------------
void roleAniMove(){
    roleAni++;
    if(roleAni>8)
        roleAni=0;
    if(roleAni < 4){
        set_sprite_tile(32, 9);
        set_sprite_tile(33, 10);
    }else{
        set_sprite_tile(32, 26);
        set_sprite_tile(33, 27);
    }
    move_sprite( 32,   role.x, role.y);
    move_sprite( 33, role.x+8, role.y);
}

//----------------------------------------------------------------------------
// 初始.
//----------------------------------------------------------------------------
void setup(){
    // 設定使用到的精靈.
    set_sprite_data(0, 28, TileLabel);
    // 初始時脈.
    clockInit();
    // init joypads
    joypad_init(4, &joypads);

    // 初始音效.
    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    NR52_REG = 0x80;
}

//----------------------------------------------------------------------------
// 主迴圈.
//----------------------------------------------------------------------------
void main(){
    // 初始.
    setup();

    // 主選單.
    if(gameMode == GAME_MODE_MENU){
        // 主目錄.
        MenuLoop();
    }

    // 初始敵人.
    enemyInit(0,1);

    //printf("Scrolling %d chars", sizeof(scroller_text) - 1);
    gotoxy(0, 0); printf(" SCORE HI-SCORE\n");
    gotoxy(0, 1); printf(" %d     %d         x%d", score, hiScore, roleNumber);

    // 遊戲結束.
    //gameOver();
    // 開始遊戲.
    gameStart();

    SHOW_SPRITES;

    while(1){       
       
        // poll joypads
        joypad_ex(&joypads);

        // 遊戲中.
        if(gameMode == GAME_MODE_PLAY){
            // 是否自動玩.
            // 否.
            if(autoplay == 0){
                for (UINT8 i = 0; i < joypads.npads; i++) {
                    UINT8 joy = joypads.joypads[i];
                    if (joy & J_LEFT){
                        role.x--;
                        if(role.x<8)
                            role.x = 8;                        
                        roleAniMove();          // 主角移動畫.
                    }
                    if (joy & J_RIGHT){
                        role.x++;
                        if(role.x>152)
                            role.x = 152;

                        roleAniMove();          // 主角移動畫.
                    }
                    if (joy & J_UP){}
                    if (joy & J_DOWN){}
                }
                // A.
                if (joypads.joy0 & J_A) {
                    roleBulletFire();               // 主角發射子彈.
                }
                // B.
                if (joypads.joy0 & J_B) {
                    roleBulletFire();               // 主角發射子彈.
                }
                // 開始按鈕.
                if (joypads.joy0 & J_START) {}

            // 是.
            }else if(autoplay == 1){
                // 主角發射子彈.
                roleBulletFire();

                // 自動玩亂數位置.
                if(roleRandX ==  role.x){
                    roleRandX = ((UINT16)rand() % 120) + 20;
                }         
                // 左移動.
                if(roleRandX < role.x){
                    role.x--;
                // 右移動.
                }else if(roleRandX > role.x){
                    role.x++;
                }
                // 主角移動畫.
                roleAniMove();

                // 閃爍字      
                if(clockGet(7, 60)){
                    menuFS++;
                    if(menuFS>1)
                        menuFS = 0;
                    if(menuFS==0){
                        gotoxy(5, 13); printf("Press Start");
                    }else{
                        gotoxy(5, 13); printf("           ");
                    }
                }

                // 開始按鈕.
                if (joypads.joy0 & J_START) {
                    autoplay = 0;
                    // 開始遊戲.
                    gameStart();
                }
            }
            
            // 更新敵人.
            if(clockGet(0, 50))
                enemyAniUpdate();
            // 更新敵人子彈.            
            enemyBulletUpdate();
                        
            // 爆炸更新.
            boomUpdate();
            // 移動敵人子彈.
            enemyBulletMove();
            // 移動主角子彈.
            roleBulletMove();            
            // 更新UFO.
            ufoUpdate();
            // 敵人移動.
            enemysMove();

        // Game Over.
        }else if(gameMode == GAME_MODE_OVER){             
            gotoxy(5, 13); printf("           ");
            // 進入自動玩模式.
            if(clockGet(7, 500)){
                // 自動玩.
                autoplay = 1;
                menuFS = 1;
                // 開始遊戲.
                gameStart();
            }
            
            // 開始按鈕.
            if (joypads.joy0 & J_START) { 
                // 開始遊戲.
                gameStart();
            }
        }        
        wait_vbl_done();        
    }

}