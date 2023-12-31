/**
 * GUNPY
 * This game is an homage to the “GUNPEY”.
 * 2023 chokmah.jp
 */

#include <Arduboy2.h>
#include <EEPROM.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "Block.h"
#include "Font4x6.h"
#include "Images.h"
#include "Message.h"
#include "Player.h"
#include "global_variables.h"

Arduboy2 arduboy;

// version
const char version[] = "0.92";

// debugging mode
// #define DEBUG_MODE

const int8_t ROWS = 5;
const int8_t COLS = 10;

// Direction of connection check
const int8_t CHECK_FROM_R = 0;
const int8_t CHECK_FROM_L = 1;

const int8_t ADD_BAR_SIZE = 69;

// Number of points per deleted block.
const int16_t SCORE_TABLE[51] PROGMEM = {
    0,    0,    0,    0,    0,    5,    10,   15,   20,   25,
    30,   40,   50,   60,   70,   80,   100,  120,  140,  160,
    180,  200,  250,  300,  350,  400,  450,  500,  550,  600,
    650,  750,  850,  950,  1000, 1200, 1350, 1500, 1600, 1800,
    2000, 2200, 2500, 2800, 3100, 3400, 3800, 4200, 4500, 5000,
};

// Number of seconds before adding a block at each level (MAX at level 10)
const int8_t ADD_INTERVAL[11] = {13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3};
// The score corresponding to the level that can be obtained when all cleared.
const int16_t ALL_CLEAR_SCORE[11] = {100, 100, 200, 300, 400, 500,
                                     600, 700, 800, 900, 1000};
// Number of erased cells before upgrade
const int16_t LEVEL_UP_LINE[10] = {
    30,  // 0->1
    60,  // 1->2
    100, // 2->3
    140, // 3->4
    180, // 4->5
    220, // 5->6
    260, // 6->7
    300, // 7->8
    350, // 8->9
    400, // 9->10
};

// Number of frames (for performance)
int8_t frameNum = 0;
// Current scene
int8_t nowScene = 0;
// Initial level
int8_t initLevel = 0;
// Current level
int8_t nowLevel = 0;
// Current score
int32_t nowScore = 0;
// Record score
int32_t highScore = -1;
//? Because I don't know much about EEPROm processing, I stagger it by about 120
// offset first.
int16_t epr_address = EEPROM_STORAGE_SPACE_START + 120;
// High score display (for game ending screen)
bool isHighScore = false;

// Number of frames before adding a block.
int16_t remainAddFrame = 0;
float remainAddFrameInit = 0;
// Column size before adding a block
int8_t remainAddBar = 0;

//
Block *blocks[COLS][ROWS] = {};
// Temporary storage area for lines used for connection checking (STD:: If only
// vector could be used …)
Block *chainedBlocks[50] = {nullptr};
int8_t chainedBlockIdx = 0;

Player *player;

// The moment when the block is replaced is set to be inoperable. The variable
// for this.
int8_t isBlockMove = 0;
// Check whether the line in is connected to the side.
bool isCrossLine = false;
int16_t totalRemoveCount = 0;

// Block being replaced
int8_t swpFromUpC = -1;
int8_t swpFromUpR = -1;
int8_t swpFromBottomC = -1;
int8_t swpFromBottomR = -1;

// Block erasure performance
int16_t blockRemovalCount = 0;
// Add or not
bool isAdd = false;

// Event message in the ↙ corner
Message *messages[3] = {new Message(0, 0, 0), new Message(0, 0, 0),
                        new Message(0, 0, 0)};

// For the end of the game
int8_t gameOverAnim = 0;

// High score reset
int16_t highScoreResetCount = 0;

void setup()
{
    arduboy.begin();
    arduboy.setFrameRate(Setting::FPS);
    arduboy.initRandomSeed();

    if (!isInitEEPROM())
    {
        initEEPROM();
    }

    nowScene = Setting::SCENE_TITLE;

    frameNum = Setting::FPS;

    player = new Player();

    // Fill the chessboard with building blocks
    // In fact, I miss new every time, but no matter how I delete it, I don't
    // know why there will be a memory leak, so I use it repeatedly after
    // filling it at first.
    for (int8_t r = 0; r < ROWS; r++)
    {
        for (int8_t c = 0; c < COLS; c++)
        {
            blocks[c][r] = new Block(Blc::TYPE_1);
        }
    }

#ifdef DEBUG_MODE
    // For debugging. Initial configuration
    remainAddFrame = 999;
    blocks[7][0]->isLive = true;
    blocks[7][0]->type = 0;
    blocks[7][1]->isLive = true;
    blocks[7][1]->type = 1;
    blocks[6][2]->isLive = true;
    blocks[6][2]->type = 1;
    blocks[5][2]->isLive = true;
    blocks[5][2]->type = 0;
    blocks[5][4]->isLive = true;
    blocks[5][4]->type = 1;
    blocks[5][0]->isLive = true;
    blocks[5][0]->type = 1;
    blocks[4][1]->isLive = true;
    blocks[4][1]->type = 0;
    blocks[4][2]->isLive = true;
    blocks[4][2]->type = 1;
    blocks[4][3]->isLive = true;
    blocks[4][3]->type = 0;
    blocks[3][0]->isLive = true;
    blocks[3][0]->type = 2;
    blocks[3][1]->isLive = true;
    blocks[3][1]->type = 1;
    blocks[2][1]->isLive = true;
    blocks[2][1]->type = 2;
    blocks[2][2]->isLive = true;
    blocks[2][2]->type = 2;

#endif
}

void loop()
{

    if (!arduboy.nextFrame())
        return;
    arduboy.pollButtons();

    // Frame number processing (for performance)
    if (--frameNum <= 0)
    {
        frameNum = Setting::FPS;
    }

    if (nowScene == Setting::SCENE_TITLE)
    {
        sceneTitle();
    }
    else if (nowScene == Setting::SCENE_PLAY)
    {
        scenePlay();
    }
    else if (nowScene == Setting::SCENE_PAUSE)
    {
        scenePause();
    }
    else if (nowScene == Setting::SCENE_GAMEOVER)
    {
        sceneGameover();
    }
    else if (nowScene == Setting::SCENE_HELP)
    {
        sceneHelp();
    }
    else if (nowScene == Setting::SCENE_HELP2)
    {
        sceneHelp2();
    }
}

//---------------------------------------------------------------//
// Title scene.
//---------------------------------------------------------------//
void sceneTitle()
{

    if (arduboy.justPressed(A_BUTTON))
    {
        initGame();
        nowScene = Setting::SCENE_PLAY;
    }
    else if (arduboy.pressed(UP_BUTTON))
    {
        highScoreResetCount++;
        if (highScoreResetCount > Setting::FPS * 10 && highScore != 0)
        {
            saveHighScore(0);
            highScore = 0;
            highScoreResetCount = 0;
        }
    }
    else if (arduboy.justPressed(LEFT_BUTTON))
    {
        if (initLevel > 0)
        {
            initLevel--;
        }
    }
    else if (arduboy.justPressed(RIGHT_BUTTON))
    {
        if (initLevel < 10)
        {
            initLevel++;
        }
    }
    else if (arduboy.justPressed(B_BUTTON))
    {
        nowScene = Setting::SCENE_HELP;
    }
    else
    {
        highScoreResetCount = 0;
    }

    if (highScore == -1)
    {
        highScore = loadHighScore();
    }

    arduboy.clear();

    arduboy.drawBitmap(0, 0, gunpytitle, 128, 64, WHITE);

    // Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, Arduboy2::width(),
    // Arduboy2::height());
    Font4x6 tinyfont = Font4x6();
    if (highScore > 0)
    {
        tinyfont.setCursor(0, 2);
        tinyfont.print(F("HighScore:"));
        tinyfont.print(highScore);
    }

    if (frameNum < (Setting::FPS / 2))
    {
        tinyfont.setCursor(24, 36);
        tinyfont.print(F("PRESS [A] BUTTON"));
    }

    tinyfont.setCursor(0, 48);
    tinyfont.print(F("InitLv:"));
    tinyfont.print(initLevel);
    tinyfont.print(F(" "));
    if (initLevel > 0)
    {
        tinyfont.print(F("<"));
    }
    if (initLevel < 10)
    {
        tinyfont.print(F(">"));
    }

    tinyfont.setCursor(0, 56);
    tinyfont.print(F("[B] Help"));

    tinyfont.setCursor(89, 56);
    tinyfont.print(F("ver:"));
    tinyfont.print(version);

    arduboy.display();
}

//---------------------------------------------------------------//
// Help scene.
//---------------------------------------------------------------//
void sceneHelp()
{
    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON))
    {
        nowScene = Setting::SCENE_HELP2;
    }

    arduboy.clear();
    int8_t offsetY = 0;

    Font4x6 tinyfont = Font4x6();
    tinyfont.setCursor(42, offsetY);
    tinyfont.print(F("[EXPLAIN]"));

    offsetY += 9;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("Rule:Connect lines from"));
    offsetY += 7;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("left to right to erase."));

    offsetY += 8;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("[A]Swap cells"));
    offsetY += 7;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("[B]Add next row instantly"));

    offsetY += 8;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("Tips:Clearing additional"));
    offsetY += 7;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("lines triggers ADD,"));
    offsetY += 7;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("doubling points earned."));

    arduboy.display();
}

//---------------------------------------------------------------//
// Help2 scene.
//---------------------------------------------------------------//
void sceneHelp2()
{
    if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON))
    {
        nowScene = Setting::SCENE_TITLE;
    }

    arduboy.clear();
    int8_t offsetY = 0;

    Font4x6 tinyfont = Font4x6();
    tinyfont.setCursor(42, offsetY);
    tinyfont.print(F("[EXPLAIN]"));

    offsetY += 9;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("Tips2:Clearing all lines"));
    offsetY += 7;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("earns a bonus."));

    offsetY += 12;
    tinyfont.setCursor(45, offsetY);
    tinyfont.print(F("[CREDIT]"));
    offsetY += 9;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("by chokmah.jp"));
    offsetY += 9;
    tinyfont.setCursor(2, offsetY);
    tinyfont.print(F("Thanks for playing."));

    arduboy.display();
}

//---------------------------------------------------------------//
// Playing scene. (In game.)
//---------------------------------------------------------------//
void scenePlay()
{

    arduboy.clear();

    // Additional block processing decision
    if (blockRemovalCount == 0)
    {
        if (remainAddFrame > 0)
        {
#ifdef DEBUG_MODE
// Do not add a time limit line when debugging.
#else
            remainAddFrame--;
#endif
        }
    }

    if (isBlockMove > 0)
    {
        // Key operation is not accepted in block replacement movement.
        isBlockMove--;
        if (isBlockMove == 0)
        {
            bool tmpUpIsLive = blocks[swpFromUpC][swpFromUpR]->isLive;
            int8_t tmpUpType = blocks[swpFromUpC][swpFromUpR]->type;
            blocks[swpFromUpC][swpFromUpR]->isLive =
                blocks[swpFromBottomC][swpFromBottomR]->isLive;
            blocks[swpFromUpC][swpFromUpR]->type =
                blocks[swpFromBottomC][swpFromBottomR]->type;
            blocks[swpFromBottomC][swpFromBottomR]->isLive = tmpUpIsLive;
            blocks[swpFromBottomC][swpFromBottomR]->type = tmpUpType;
            swpFromUpC = -1;
            swpFromUpR = -1;
            swpFromBottomC = -1;
            swpFromBottomR = -1;
        }
    }
    else
    {
        if (remainAddFrame <= 0)
        {
            // Block addition processing
            addBlock();
        }
        else
        {
            // Player operation processing (user operation is not accepted in
            // block append processing)
            if (player->isFocusPause)
            {
                if (arduboy.justPressed(LEFT_BUTTON))
                {
                    player->defocusPause();
                }
                else if (arduboy.justPressed(A_BUTTON))
                {
                    nowScene = Setting::SCENE_PAUSE;
                }
            }
            else
            {
                if (arduboy.justPressed(UP_BUTTON))
                {
                    if (player->y > 0)
                    {
                        player->set(player->x, player->y - 1);
                    }
                }
                else if (arduboy.justPressed(DOWN_BUTTON))
                {
                    if (player->y < COLS - 2)
                    {
                        player->set(player->x, player->y + 1);
                    }
                }
                else if (arduboy.justPressed(LEFT_BUTTON))
                {
                    if (player->x > 0)
                    {
                        player->set(player->x - 1, player->y);
                    }
                }
                else if (arduboy.justPressed(RIGHT_BUTTON))
                {
                    if (player->x < ROWS - 1)
                    {
                        player->set(player->x + 1, player->y);
                    }
                    else
                    {
                        // If you right-click on the right end again, the Pause
                        // button will focus.
                        player->focusPause();
                    }
                }
                else if (arduboy.justPressed(A_BUTTON))
                {
                    // Block exchange
                    if ((!blocks[player->y][player->x]->isLive ||
                         (blocks[player->y][player->x]->isLive &&
                          !blocks[player->y][player->x]->isRemoving)) &&
                        (!blocks[player->y + 1][player->x]->isLive ||
                         blocks[player->y + 1][player->x]->isLive &&
                             !blocks[player->y + 1][player->x]->isRemoving))
                    {
                        // Can't move in the removal process.
                        swpFromUpC = player->y;
                        swpFromUpR = player->x;
                        swpFromBottomC = player->y + 1;
                        swpFromBottomR = player->x;
                        isBlockMove = 3;
                    }
                }
                else if (arduboy.justPressed(B_BUTTON))
                {
                    // Block manual addition processing
                    if (blockRemovalCount == 0)
                    {
                        addBlock();
                    }
                }
            }
        }

        // Check and handle
        initBlockChained();
        for (int8_t c = 0; c < COLS; c++)
        {
            // Scan the block and confirm whether it is connected.
            checkChained(c, 0, CHECK_FROM_L);
            // Box marks that can be eliminated after scanning.
        }
    }

    if (isCrossLine)
    {
        if (blockRemovalCount == 0)
        {
            for (int8_t r = 0; r < ROWS; r++)
            {
                for (int8_t c = 0; c < COLS; c++)
                {
                    if (!blocks[c][r]->isLive)
                        continue;
                    if (blocks[c][r]->isChained)
                    {
                        blocks[c][r]->isRemoving = true;
                    }
                }
            }
            blockRemovalCount = Setting::FPS * 3;
        }
        else
        {
            for (int8_t r = 0; r < ROWS; r++)
            {
                for (int8_t c = 0; c < COLS; c++)
                {
                    if (!blocks[c][r]->isLive)
                        continue;
                    if (blocks[c][r]->isChained && !blocks[c][r]->isRemoving)
                    {
                        blocks[c][r]->isRemoving = true;
                        // ADD performance
                        if (!isAdd)
                        {
                            putMessage(Setting::MSG_HIT_ADD, 0, 0);
                            isAdd = true;
                        }
                    }
                }
            }
        }
    }

    if (blockRemovalCount > 0)
    {
        blockRemovalCount--;
    }

    if (blockRemovalCount == 1)
    {
        int8_t remBlockNum = 0;
        for (int8_t r = 0; r < ROWS; r++)
        {
            for (int8_t c = 0; c < COLS; c++)
            {
                if (!blocks[c][r]->isLive)
                    continue;
                if (blocks[c][r]->isRemoving)
                {
                    removeBlock(c, r);
                    remBlockNum++;
                }
            }
        }

        if (remBlockNum > 0)
        {
            // Add points
            int16_t addingScore = pgm_read_word(&SCORE_TABLE[remBlockNum]);
            if (isAdd)
            {
                addingScore *= 2;
                isAdd = false;
            }
            nowScore += addingScore;
            // Message drawing
            putMessage(Setting::MSG_HIT, remBlockNum, addingScore);

            if (nowLevel < 10)
            {
                if (LEVEL_UP_LINE[nowLevel] < totalRemoveCount)
                {
                    nowLevel++;
                    // Upgrade performance
                    putMessage(Setting::MSG_LEVEL_UP, 0, 0);
                }
            }
        }

        bool isAllClear = true;
        for (int8_t r = 0; r < ROWS && isAllClear; r++)
        {
            for (int8_t c = 0; c < COLS && isAllClear; c++)
            {
                if (blocks[c][r]->isLive)
                {
                    isAllClear = false;
                    break;
                }
            }
        }
        if (isAllClear)
        {
            nowScore += ALL_CLEAR_SCORE[nowLevel];
            // All clear performance
            putMessage(Setting::MSG_ALL_CLEAR, 0, 0);
        }
    }

    // The block removal judgment in this cycle has ended, so initialization is
    // performed.
    clearChainedBlocks();
    isCrossLine = false;

    drawBg();
    drawUI();
    drawPlayer();
    drawBlocks();

    arduboy.display();
}

//---------------------------------------------------------------//
// Pause scene.
//---------------------------------------------------------------//
void scenePause()
{

    if (arduboy.justPressed(A_BUTTON))
    {
        nowScene = Setting::SCENE_PLAY;
    }

    arduboy.fillRect(29, 0, 70, 60, BLACK);
    arduboy.drawRect(29, 0, 71, 61, WHITE);
    arduboy.setCursor(50, 28);
    arduboy.print(F("PAUSE"));

    arduboy.display();
}

//---------------------------------------------------------------//
// Gameover scene.
//---------------------------------------------------------------//
void sceneGameover()
{

    if (gameOverAnim < Setting::FPS)
    {

        arduboy.fillRect(0, 0, 128, (64 / Setting::FPS + 1) * gameOverAnim,
                         BLACK);
        arduboy.display();

        gameOverAnim++;

        if (gameOverAnim == Setting::FPS)
        {
            // High score update check
            if (highScore <= nowScore)
            {
                isHighScore = true;
                saveHighScore(nowScore);
                highScore = nowScore;
            }
        }
    }
    else
    {

        if (arduboy.justPressed(A_BUTTON))
        {
            nowScene = Setting::SCENE_TITLE;
        }

        if (isHighScore)
        {
            arduboy.setCursor(30, 14);
            arduboy.print(F("HIGH SCORE !!"));
        }

        arduboy.setCursor(40, 28);
        arduboy.print(F("GAME OVER"));

        arduboy.setCursor(55 - (6 * digits(nowScore)), 48);
        arduboy.print(F("SCORE:"));
        arduboy.print(nowScore);
        arduboy.display();
    }
}

void drawBg()
{

    // arduboy.drawRect(29, 0, 71, 61, WHITE);

    // for (int8_t i = 0; i < 4; i++) {
    //   arduboy.drawLine(43 + (14 * i), 1, 43 + (14 * i), 59, WHITE);
    // }
    // for (int8_t i = 0; i < 9; i++) {
    //   arduboy.drawLine(30, 6 + (6 * i), 98, 6 + (6 * i), WHITE);
    // }

    // arduboy.drawRect(29, 60, 71, 4, WHITE);
    // Add block remaining time bar drawing
    arduboy.drawBitmap(0, 0, gunpypic, 128, 64, WHITE);

    float percentage = (float)remainAddFrame / (float)remainAddFrameInit;
    int currentLength = (int)(69 * percentage);
    arduboy.drawRect(30, 61, currentLength, 2, WHITE);
}

void drawUI()
{
    // Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, Arduboy2::width(),
    // Arduboy2::height());

    Font4x6 tinyfont = Font4x6();

    // Level display
    // arduboy.drawLine( 0, 0, 28, 0, WHITE );
    tinyfont.setCursor(22, 4);
    // tinyfont.print(F("Lv:"));
    if (nowLevel == 10)
    {
        tinyfont.setCursor(7, 4);
        tinyfont.print(F("Max"));
    }
    else
    {
        tinyfont.print(nowLevel);
    }
    // Score display
    // arduboy.drawLine( 0, 9, 28, 9, WHITE );
    tinyfont.setCursor(5 * (6 - digits(nowScore)) - 3, 18);
    tinyfont.print(nowScore);
    // arduboy.drawLine( 0, 18, 28, 18, WHITE );
    if (highScore < nowScore)
        highScore = nowScore;
    tinyfont.setCursor(5 * (6 - digits(highScore)) + 97, 16);
    tinyfont.print(highScore);

    // Message display
    for (int8_t i = 0; i < 3; i++)
    {
        if (messages[i]->isLive)
        {
            int8_t offsetX = 0;
            if (messages[i]->anim < 10)
            {
                offsetX = -4 - (5 * (10 - messages[i]->anim));
            }
            int8_t offsetYoutput = 30 + (i * 14) + 4;
            if (messages[i]->type == Setting::MSG_HIT)
            {
                tinyfont.setCursor(2 + offsetX, offsetYoutput);
                tinyfont.print(messages[i]->blockNum);
                tinyfont.print(F("HIT"));
                tinyfont.setCursor(2 + offsetX, offsetYoutput + 7);
                tinyfont.print(F("+"));
                tinyfont.print(messages[i]->score);
            }
            else if (messages[i]->type == Setting::MSG_LEVEL_UP)
            {
                tinyfont.setCursor(2 + offsetX, offsetYoutput + 4);
                tinyfont.print(F("Lv UP"));
            }
            else if (messages[i]->type == Setting::MSG_ALL_CLEAR)
            {
                tinyfont.setCursor(2 + offsetX, offsetYoutput);
                tinyfont.print(F("ALL!!"));
                tinyfont.setCursor(2 + offsetX, offsetYoutput + 7);
                tinyfont.print(F("+"));
                tinyfont.print(ALL_CLEAR_SCORE[nowLevel]);
            }
            else if (messages[i]->type == Setting::MSG_HIT_ADD)
            {
                tinyfont.setCursor(2 + offsetX, offsetYoutput);
                tinyfont.print(F("ADD!!"));
                tinyfont.setCursor(2 + offsetX, offsetYoutput + 7);
                tinyfont.print(F("x2!!"));
            }

            if (messages[i]->doAnim() == 0)
            {
                removeMessage(messages[i]->index);
            }
        }
    }

    // Pause button Coffee cup.
    // arduboy.drawBitmap(105, 23, IMG_PAUSE, 16, 16, WHITE);

    bool isWarn = false;
    for (int8_t i = 0; i < ROWS; i++)
    {
        if (blocks[0][i]->isLive)
        {
            isWarn = true;
            break;
        }
    }
    if (isWarn && frameNum < Setting::FPS / 2)
    {
        tinyfont.setCursor(102, 1);
        tinyfont.print(F("<WARN"));
    }

#ifdef DEBUG_MODE
    arduboy.setCursor(0, 45);
    arduboy.print("RAM:\n");
    arduboy.print(freeMemory());

    arduboy.setCursor(100, 45);
    arduboy.print("t");
    arduboy.print(totalRemoveCount);
#endif
}

void drawPlayer()
{

    if (player->isFocusPause)
    {
        arduboy.fillRect(101, 55, 27, 9, BLACK);
        arduboy.drawBitmap(101, 55, pausepic, 27, 9, WHITE);
    }
    else
    {
        if (player->x < 0 || player->y < 0)
        {
            player->x = 0, player->y = 0;
        }
        arduboy.fillRect(30 + (14 * player->x), 1 + (6 * player->y), 13, 11,
                         WHITE);
        arduboy.drawLine(30 + (14 * player->x), 1 + (6 * player->y) + 5,
                         30 + (14 * player->x) + 12, 1 + (6 * player->y) + 5,
                         BLACK);
    }
}

void drawBlocks()
{
    for (int8_t r = 0; r < ROWS; r++)
    {
        for (int8_t c = 0; c < COLS; c++)
        {
            if (blocks[c][r]->isLive)
            {
                uint8_t color = 0;
                if ((r == player->x && c == player->y) ||
                    (r == player->x && c == player->y + 1))
                {
                    color = BLACK;
                }
                else
                {
                    color = WHITE;
                }

                int8_t movingReviseY = 0;
                if (swpFromUpC == c && swpFromUpR == r)
                {
                    movingReviseY = 2 * (4 - isBlockMove);
                }
                else if (swpFromBottomC == c && swpFromBottomR == r)
                {
                    movingReviseY = -2 * (4 - isBlockMove);
                }

                if (blocks[c][r]->isLive &&
                    (!blocks[c][r]->isRemoving ||
                     blocks[c][r]->isRemoving && blockRemovalCount % 4 < 2))
                {
                    if (blocks[c][r]->type == Blc::TYPE_1)
                    {
                        arduboy.drawLine(
                            30 + (14 * r), 1 + (6 * c) + movingReviseY,
                            30 + (14 * r) + 12, 1 + (6 * c) + 4 + movingReviseY,
                            color);
                    }
                    else if (blocks[c][r]->type == Blc::TYPE_2)
                    {
                        arduboy.drawLine(30 + (14 * r),
                                         1 + (6 * c) + 4 + movingReviseY,
                                         30 + (14 * r) + 12,
                                         1 + (6 * c) + movingReviseY, color);
                    }
                    else if (blocks[c][r]->type == Blc::TYPE_3)
                    {
                        arduboy.drawLine(
                            30 + (14 * r), 1 + (6 * c) + movingReviseY,
                            30 + (14 * r) + 6, 1 + (6 * c) + 2 + movingReviseY,
                            color);
                        arduboy.drawLine(30 + (14 * r) + 6,
                                         1 + (6 * c) + 2 + movingReviseY,
                                         30 + (14 * r) + 12,
                                         1 + (6 * c) + movingReviseY, color);
                    }
                    else if (blocks[c][r]->type == Blc::TYPE_4)
                    {
                        arduboy.drawLine(
                            30 + (14 * r), 1 + (6 * c) + 4 + movingReviseY,
                            30 + (14 * r) + 6, 1 + (6 * c) + 2 + movingReviseY,
                            color);
                        arduboy.drawLine(
                            30 + (14 * r) + 6, 1 + (6 * c) + 2 + movingReviseY,
                            30 + (14 * r) + 12, 1 + (6 * c) + 4 + movingReviseY,
                            color);
                    }
                }
            }
        }
    }
}

void initBlockChained()
{
    for (int8_t r = 0; r < ROWS; r++)
    {
        for (int8_t c = 0; c < COLS; c++)
        {
            if (blocks[c][r] != nullptr)
            {
                blocks[c][r]->isChained = false;
            }
        }
    }
}

bool checkChained(int8_t c, int8_t r, int8_t d)
{

    // When there are no blocks in the cell, skip.
    if (!blocks[c][r]->isLive)
        return false;

    if (r == ROWS - 1)
    {
        isCrossLine = true;
        // Reach the right end, and set the isChained flag on all temporary
        // Chain.
        blocks[c][r]->isChained = true;
        fixChainedBlocks();
        return true;
    }

#ifdef DEBUG_MODE
    Serial.print("CELL[");
    Serial.print(c);
    Serial.print("][");
    Serial.print(r);
    Serial.print("] check start.\n");
#endif

    bool isChained = false;

    // ＼
    if (blocks[c][r]->type == Blc::TYPE_1)
    {
        if (d == CHECK_FROM_L)
        {
            // →
            if (blocks[c][r + 1]->isLive &&
                !isAlreadyCheckChained(blocks[c][r + 1]))
            {
                if (blocks[c][r + 1]->type == Blc::TYPE_2 ||
                    blocks[c][r + 1]->type == Blc::TYPE_4)
                {
                    pushChainedBlocks(blocks[c][r]);
                    isChained = true;
                    if (!blocks[c][r + 1]->isChained)
                    {
                        if (!checkChained(c, r + 1, CHECK_FROM_L))
                        {
                            isChained = false;
                            popChainedBlocks();
                        }
                    }
                    else
                    {
                        blocks[c][r]->isChained = true;
                        fixChainedBlocks();
                    }
                }
            }
            // ↘
            if (c < COLS - 1)
            {
                if (blocks[c + 1][r + 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r + 1]))
                {
                    if (blocks[c + 1][r + 1]->type == Blc::TYPE_1 ||
                        blocks[c + 1][r + 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r + 1]->isChained)
                        {
                            if (!checkChained(c + 1, r + 1, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↓
            if (c < COLS - 1)
            {
                if (blocks[c + 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r]))
                {
                    if (blocks[c + 1][r]->type == Blc::TYPE_2 ||
                        blocks[c + 1][r]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r]->isChained)
                        {
                            if (!checkChained(c + 1, r, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
        else
        {
            // ↑
            if (c > 0)
            {
                if (blocks[c - 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r]))
                {
                    if (blocks[c - 1][r]->type == Blc::TYPE_2 ||
                        blocks[c - 1][r]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r]->isChained)
                        {
                            if (!checkChained(c - 1, r, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ←
            if (r > 0)
            {
                if (blocks[c][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c][r - 1]))
                {
                    if (blocks[c][r - 1]->type == Blc::TYPE_2 ||
                        blocks[c][r - 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c][r - 1]->isChained)
                        {
                            if (!checkChained(c, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↖
            if (c > 0 && r > 0)
            {
                if (blocks[c - 1][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r - 1]))
                {
                    if (blocks[c - 1][r - 1]->type == Blc::TYPE_1 ||
                        blocks[c - 1][r - 1]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r - 1]->isChained)
                        {
                            if (!checkChained(c - 1, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
    }
    // ／
    else if (blocks[c][r]->type == Blc::TYPE_2)
    {
        if (d == CHECK_FROM_L)
        {
            // →
            if (blocks[c][r + 1]->isLive &&
                !isAlreadyCheckChained(blocks[c][r + 1]))
            {
                if (blocks[c][r + 1]->type == Blc::TYPE_1 ||
                    blocks[c][r + 1]->type == Blc::TYPE_3)
                {
                    pushChainedBlocks(blocks[c][r]);
                    isChained = true;
                    if (!blocks[c][r + 1]->isChained)
                    {
                        if (!checkChained(c, r + 1, CHECK_FROM_L))
                        {
                            isChained = false;
                            popChainedBlocks();
                        };
                    }
                    else
                    {
                        blocks[c][r]->isChained = true;
                        fixChainedBlocks();
                    }
                }
            }
            // ↗
            if (c > 0)
            {
                if (blocks[c - 1][r + 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r + 1]))
                {
                    if (blocks[c - 1][r + 1]->type == Blc::TYPE_2 ||
                        blocks[c - 1][r + 1]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r + 1]->isChained)
                        {
                            if (!checkChained(c - 1, r + 1, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↑
            if (c > 0)
            {
                if (blocks[c - 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r]))
                {
                    if (blocks[c - 1][r]->type == Blc::TYPE_1 ||
                        blocks[c - 1][r]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r]->isChained)
                        {
                            if (!checkChained(c - 1, r, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
        else
        {
            // ↓
            if (c < COLS - 1)
            {
                if (blocks[c + 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r]))
                {
                    if (blocks[c + 1][r]->type == Blc::TYPE_1 ||
                        blocks[c + 1][r]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r]->isChained)
                        {
                            if (!checkChained(c + 1, r, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ←
            if (r > 0)
            {
                if (blocks[c][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c][r - 1]))
                {
                    if (blocks[c][r - 1]->type == Blc::TYPE_1 ||
                        blocks[c][r - 1]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c][r - 1]->isChained)
                        {
                            if (!checkChained(c, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↙
            if (c < COLS - 1 && r > 0)
            {
                if (blocks[c + 1][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r - 1]))
                {
                    if (blocks[c + 1][r - 1]->type == Blc::TYPE_2 ||
                        blocks[c + 1][r - 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r - 1]->isChained)
                        {
                            if (!checkChained(c + 1, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
    }
    // V
    else if (blocks[c][r]->type == Blc::TYPE_3)
    {
        if (d == CHECK_FROM_L)
        {
            // →
            if (blocks[c][r + 1]->isLive &&
                !isAlreadyCheckChained(blocks[c][r + 1]))
            {
                if (blocks[c][r + 1]->type == Blc::TYPE_1 ||
                    blocks[c][r + 1]->type == Blc::TYPE_3)
                {
                    pushChainedBlocks(blocks[c][r]);
                    isChained = true;
                    if (!blocks[c][r + 1]->isChained)
                    {
                        if (!checkChained(c, r + 1, CHECK_FROM_L))
                        {
                            isChained = false;
                            popChainedBlocks();
                        };
                    }
                    else
                    {
                        blocks[c][r]->isChained = true;
                        fixChainedBlocks();
                    }
                }
            }
            // ↗
            if (c > 0)
            {
                if (blocks[c - 1][r + 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r + 1]))
                {
                    if (blocks[c - 1][r + 1]->type == Blc::TYPE_2 ||
                        blocks[c - 1][r + 1]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r + 1]->isChained)
                        {
                            if (!checkChained(c - 1, r + 1, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↑
            if (c > 0)
            {
                if (blocks[c - 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r]))
                {
                    if (blocks[c - 1][r]->type == Blc::TYPE_1)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r]->isChained)
                        {
                            if (!checkChained(c - 1, r, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
        else
        {
            // ←
            if (r > 0)
            {
                if (blocks[c][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c][r - 1]))
                {
                    if (blocks[c][r - 1]->type == Blc::TYPE_2 ||
                        blocks[c][r - 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c][r - 1]->isChained)
                        {
                            if (!checkChained(c, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↖
            if (c > 0 && r > 0)
            {
                if (blocks[c - 1][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r - 1]))
                {
                    if (blocks[c - 1][r - 1]->type == Blc::TYPE_1 ||
                        blocks[c - 1][r - 1]->type == Blc::TYPE_4)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r - 1]->isChained)
                        {
                            if (!checkChained(c - 1, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↑
            if (c > 0)
            {
                if (blocks[c - 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c - 1][r]))
                {
                    if (blocks[c - 1][r]->type == Blc::TYPE_2)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c - 1][r]->isChained)
                        {
                            if (!checkChained(c - 1, r, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
        if (blocks[c][r]->isChained == true && c > 0 && blocks[c - 1][r]->type == Blc::TYPE_4)
        {
            blocks[c - 1][r]->isChained = true;
        }
    }

    // Λ
    else if (blocks[c][r]->type == Blc::TYPE_4)
    {
        if (d == CHECK_FROM_L)
        {
            // →
            if (blocks[c][r + 1]->isLive &&
                !isAlreadyCheckChained(blocks[c][r + 1]))
            {
                if (blocks[c][r + 1]->type == Blc::TYPE_2 ||
                    blocks[c][r + 1]->type == Blc::TYPE_4)
                {
                    pushChainedBlocks(blocks[c][r]);
                    isChained = true;
                    if (!blocks[c][r + 1]->isChained)
                    {
                        if (!checkChained(c, r + 1, CHECK_FROM_L))
                        {
                            isChained = false;
                            popChainedBlocks();
                        }
                    }
                    else
                    {
                        blocks[c][r]->isChained = true;
                        fixChainedBlocks();
                    }
                }
            }
            // ↘
            if (c < COLS - 1)
            {
                if (blocks[c + 1][r + 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r + 1]))
                {
                    if (blocks[c + 1][r + 1]->type == Blc::TYPE_1 ||
                        blocks[c + 1][r + 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r + 1]->isChained)
                        {
                            if (!checkChained(c + 1, r + 1, CHECK_FROM_L))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↓
            if (c < COLS - 1)
            {
                if (blocks[c + 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r]))
                {
                    if (blocks[c + 1][r]->type == Blc::TYPE_2)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r]->isChained)
                        {
                            if (!checkChained(c + 1, r, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
        else
        {
            // ←
            if (r > 0)
            {
                if (blocks[c][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c][r - 1]))
                {
                    if (blocks[c][r - 1]->type == Blc::TYPE_1 ||
                        blocks[c][r - 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c][r - 1]->isChained)
                        {
                            if (!checkChained(c, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↙
            if (c < COLS - 1 && r > 0)
            {
                if (blocks[c + 1][r - 1]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r - 1]))
                {
                    if (blocks[c + 1][r - 1]->type == Blc::TYPE_2 ||
                        blocks[c + 1][r - 1]->type == Blc::TYPE_3)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r - 1]->isChained)
                        {
                            if (!checkChained(c + 1, r - 1, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
            // ↓
            if (c < COLS - 1)
            {
                if (blocks[c + 1][r]->isLive &&
                    !isAlreadyCheckChained(blocks[c + 1][r]))
                {
                    if (blocks[c + 1][r]->type == Blc::TYPE_2)
                    {
                        pushChainedBlocks(blocks[c][r]);
                        isChained = true;
                        if (!blocks[c + 1][r]->isChained)
                        {
                            if (!checkChained(c + 1, r, CHECK_FROM_R))
                            {
                                isChained = false;
                                popChainedBlocks();
                            }
                        }
                        else
                        {
                            blocks[c][r]->isChained = true;
                            fixChainedBlocks();
                        }
                    }
                }
            }
        }
        if (blocks[c][r]->isChained == true && c < 9 && blocks[c + 1][r]->type == Blc::TYPE_3)
        {
            blocks[c + 1][r]->isChained = true;
        }
    }

    // Clear chainedBlocks at left end
    if (r == 0)
    {
        clearChainedBlocks();
    }

#ifdef DEBUG_MODE
    // delay(1000);
#endif

    return isChained;
}

void pushChainedBlocks(Block *b)
{
    chainedBlocks[chainedBlockIdx] = b;
    chainedBlockIdx++;
}

Block *popChainedBlocks()
{
    if (chainedBlockIdx == 0)
    {
        return nullptr;
    }
    Block *block = chainedBlocks[chainedBlockIdx];
    chainedBlocks[chainedBlockIdx--] = nullptr;
    return block;
}

bool isAlreadyCheckChained(Block *b)
{
    for (int8_t i = chainedBlockIdx - 1; i >= 0; i--)
    {
        if (chainedBlocks[i] == b)
        {
            return true;
        }
    }
    return false;
}

void fixChainedBlocks()
{
    for (int8_t i = chainedBlockIdx - 1; i >= 0; i--)
    {
        chainedBlocks[i]->isChained = true;
    }
}

void clearChainedBlocks()
{
    for (int8_t i = chainedBlockIdx; i >= 0; i--)
    {
        chainedBlocks[i] = nullptr;
    }
    chainedBlockIdx = 0;
}

// Calculate the number of digits of an integer value
int8_t digits(int32_t arg)
{
    const int32_t digit_table[] = {10, 100, 1000, 10000, 100000};
    const int8_t digit_table_size =
        sizeof(digit_table) / sizeof(digit_table[0]);
    int8_t count = 0;
    for (; count < digit_table_size; count++)
    {
        if (arg < digit_table[count])
        {
            break;
        }
    }
    return count + 1; // The number of digits is 1 added to the count.
}

void addBlock()
{

    // Offset the whole block upward.
    bool isContinueGame = slideAllBlocks();
    // Game end processing
    if (!isContinueGame)
    {
        nowScene = Setting::SCENE_GAMEOVER;
    }

    // Configuration site determines the principle.
    int8_t nums[] = {-1, -1, -1, -1, -1};
    int8_t offset = 0;

    // Definitely add a column that has none.
    for (int8_t r = 0; r < ROWS; r++)
    {
        bool isSetBlock = false;
        for (int8_t c = 0; c < COLS; c++)
        {
            if (blocks[c][r]->isLive)
            {
                isSetBlock = true;
                break;
            }
        }
        if (!isSetBlock)
        {
            nums[0] = r;
            offset = 1;
            break;
        }
    }

    int8_t putBlockNum = random(3, 5);
    getRandomPos(nums, putBlockNum, offset);
    for (int8_t i = 0; i < 5; i++)
    {
        if (nums[i] == -1)
            break;
        int8_t type = random(0, 4);
        blocks[9][nums[i]]->isLive = true;
        blocks[9][nums[i]]->type = type;
    }

    addReset();
}

/*
 * Reset Block Add Dataset
 */
void addReset()
{

    remainAddFrame = ADD_INTERVAL[nowLevel] * Setting::FPS;
    remainAddFrameInit = (float)remainAddFrame;
    remainAddBar = ADD_BAR_SIZE;
}

void removeBlock(int8_t c, int8_t r)
{
    blocks[c][r]->isLive = false;
    blocks[c][r]->isChained = false;
    blocks[c][r]->isRemoving = false;
    totalRemoveCount++;
}

void getRandomPos(int8_t *nums, int8_t num, int8_t offset)
{
    int8_t numsTable[] = {0, 1, 2, 3, 4};
    for (int8_t i = offset; i < num;)
    {
        int8_t index = random(0, 5);
        if (numsTable[index] == -1)
            continue;
        nums[i] = numsTable[index];
        numsTable[index] = -1;
        i++;
    }
}

/*
 * @return true: ok / false:gameover
 */
bool slideAllBlocks()
{

    for (int8_t c = 0; c < COLS; c++)
    {
        for (int8_t r = 0; r < ROWS; r++)
        {
            if (c == 0 && blocks[c][r]->isLive)
            {
                removeBlock(c, r);
#ifdef DEBUG_MODE
// Debugging mode will not enter the game.
#else
                return false;
#endif
            }
            else
            {
                if (c > 0)
                {
                    blocks[c - 1][r]->isLive = blocks[c][r]->isLive;
                    blocks[c - 1][r]->type = blocks[c][r]->type;
                    blocks[c][r]->isLive = false;
                    if (c == COLS - 1)
                    {
                        blocks[c][r]->isLive = false;
                    }
                }
            }
        }
    }
    return true;
}

void initGame()
{

#ifdef DEBUG_MODE
    return;
#endif

    for (int8_t r = 0; r < ROWS; r++)
    {
        for (int8_t c = 0; c < COLS; c++)
        {
            removeBlock(c, r);
        }
    }

    for (int8_t i = 0; i < 3; i++)
    {
        messages[i]->isLive = false;
    }

    // Initial stage of each number
    nowLevel = initLevel;
    nowScore = 0;
    remainAddFrame = 0;
    remainAddFrameInit = 0;
    remainAddBar = 0;
    isBlockMove = 0;
    isCrossLine = false;
    blockRemovalCount = 0;
    totalRemoveCount = 0;
    gameOverAnim = 0;
    isAdd = false;
    isHighScore = false;
    swpFromUpC = -1;
    swpFromUpR = -1;
    swpFromBottomC = -1;
    swpFromBottomR = -1;

    player->set(0, 0);

    // Initial configuration
    for (int8_t l = 0; l < 4; l++)
    {
        addBlock();
        int8_t t = random(0, 2);
        // Add a row block first.
        for (int8_t i = 0; i < t; i++)
        {
            slideAllBlocks();
        }
    }
}

void putMessage(int8_t tp, int8_t bNum, int16_t scr)
{
    for (int8_t i = 0; i < 3; i++)
    {
        if (!messages[i]->isLive)
        {
            messages[i]->isLive = true;
            messages[i]->index = i;
            messages[i]->type = tp;
            messages[i]->blockNum = bNum;
            messages[i]->score = scr;
            messages[i]->anim = Setting::MESSAGE_ANIM_CNT;
            break;
        }
    }
}

void removeMessage(int8_t idx)
{
    for (int8_t i = 0; i < 3; i++)
    {
        if (messages[i]->index == idx)
        {
            messages[i]->isLive = false;
            break;
        }
    }
}

void saveHighScore(int32_t saveValue)
{
    EEPROM.put(epr_address + 5, saveValue); // Save high score to EEPROM.
}

int32_t loadHighScore()
{
    int32_t loadValue = 0;
    EEPROM.get(epr_address + 5, loadValue); // Read high score from EEPROM
    return loadValue;
}

bool isInitEEPROM()
{
    char loadValue[5];
    EEPROM.get(epr_address, loadValue);
    return strcmp(loadValue, "GUNPY") == 0;
}

void initEEPROM()
{
    char header[] = "GUNPY";
    for (int8_t i = 0; i < 5; i++)
    {
        EEPROM.update(epr_address + i, header[i]);
    }
    int32_t initHightScore = 0;
    EEPROM.put(epr_address + 5, initHightScore);
}

int freeMemory()
{
    // Because the price has not changed, it seems meaningless.
    char *heapend = (char *)(&__malloc_heap_start) + (int)(&__malloc_margin);
    char *stackend = (char *)SP;
    return stackend - heapend;
}
