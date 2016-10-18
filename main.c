////////////////////////////////////////////////////////////////////////////////
// ECE 2534:                        Lab4 - Zombie Apocalypse
//
// Does it work?                    Yes, please see the game instructions for
//                                  more deteals.
//
// File name:                       main.c
// Written by:                      Joshua Knestaut
//
// Last modified:                   9 December 2015

#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include <math.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "ADXL345.h"


// Configure the microcontroller
#pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_8         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = OFF           // Debugger Disabled for Starter Kit

// Intrumentation for the logic analyzer (or oscilliscope)
#define MASK_DBG1  0x1;
#define MASK_DBG2  0x2;
#define MASK_DBG3  0x4;
#define DBG_ON(a)  LATESET = a
#define DBG_OFF(a) LATECLR = a
#define DBG_INIT() TRISECLR = 0x7

// Definitions for the ADC averaging. How many samples (should be a power
// of 2, and the log2 of this number to be able to shift right instead of
// divide to get the average.
#define NUM_ADC_SAMPLES 32
#define LOG2_NUM_ADC_SAMPLES 5

#define USE_OLED_DRAW_GLYPH

#ifdef USE_OLED_DRAW_GLYPH

void OledDrawGlyph(char ch);
#endif

//Global definitions of our user-defined characters
BYTE brick[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
char brick_char = 0x00;
BYTE human[8] = {0x00,0x2f,0xbb,0x7f,0x7f,0xbb,0x2f,0x00};
char human_char = 0x01;
BYTE zombie[8] = {0x00,0x27,0xb5,0x7d,0xff,0x39,0x4f,0x00};
char zombie_char = 0x02;
BYTE blank[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
char blank_char = 0x03;
BYTE load_bar1[8] = {0x00, 0x3c, 0x42, 0x99, 0xdd, 0xdd, 0xdd, 0xdd};
char load_bar1_char = 0x04;
BYTE load_bar2[8] = {0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,};
char load_bar2_char = 0x05;
BYTE load_bar3[8] = {0xdd, 0xdd, 0xdd, 0xdd, 0x99, 0x42, 0x3c, 0x00};
char load_bar3_char = 0x06;
BYTE hole[8] = {0x18, 0x7e, 0x7e, 0xff, 0xff, 0x7e, 0x7e, 0x18};
char hole_char = 0x07;


// Global variable to count number of times in timer2 ISR
unsigned int timer2_ms_value = 0;
int Zombie_Count = 0;
int human_count = 0;
int count = 0;
unsigned char Button1History = 0;
unsigned char Button2History = 0;
unsigned char Button3History = 0;

int flag;


// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    DBG_ON(MASK_DBG1);
    timer2_ms_value++; // Increment the millisecond counter.
    DBG_OFF(MASK_DBG1);
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.


    ///////////////////////////////
    // Button debouncing
    Button1History <<= 1;
    Button2History <<= 1;
    Button3History <<= 1;

    if(PORTG & 0x40)
        Button1History |= 0x01;

    if(PORTG & 0x80)
        Button2History |= 0x01;

    if(PORTA & 0x01)
        Button3History |= 0x01;
    ///////////////////////////////


    human_count++;
    Zombie_Count++;
    count++;
    ///////////////////////////////

}


// Initialize timer2 and set up the interrupts
void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);


}

// Return value check macro
#define CHECK_RET_VALUE(a) { \
  if (a == 0) { \
    LATGSET = 0xF << 12; \
    return(EXIT_FAILURE) ; \
  } \
}


typedef struct _position{
    int x;
    int y;
}position;

//////////////////////////////////////////////////////////////////////
// function protocals


void draw_human(position human);

int move_human(short x, short y, position* human);

void draw_zombie(position zombie);

int Tilt_Check(short x, short y);

int collision_check(position human, position zombie);

void move_zombie(position human, position* zombie);

void clear_glyph(position p1);

void draw_hole(position p1);

void draw_brick(position p1);

//////////////////////////////////////////////////////////////////////

int main() {

    char oledstring[17];
    short xcur;
    short ycur;
    short zcur;
    int score = 0;
    int countdown = 0;
    int blinky_count = 0;
    int human_state = 0;
    int tilt;
    int gencount = 0;

    int i;

    TRISGSET = 0xc0; // for BTNs 1,2
    LATGCLR = 0xc0;


    DDPCONbits.JTAGEN = 0;
    TRISASET = 0x01; // for BTN 3
    LATACLR = 0x01;


    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();


    // Initialize GPIO for debugging
    DBG_INIT();

    // Initial Timer2 and ADC
    initTimer2();


    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();

    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);

    // Send a welcome message to the OLED display
    OledClearBuffer();
    OledSetCursor(0, 0);
    //OledPutString("ADC Example");
    OledUpdate();

    int retValue = 0;

    retValue = OledDefUserChar(human_char, human);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(zombie_char, zombie);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(blank_char, blank);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(load_bar1_char, load_bar1);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(load_bar2_char, load_bar2);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(load_bar3_char, load_bar3);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(hole_char, hole);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(brick_char, brick);
    CHECK_RET_VALUE(retValue);


    //initiallizing the adxl
    if((ADXL345_Init(0) == 1)){
        ADXL345_SetRegisterValue(0x31, ADXL345_RANGE(0x5));
        ADXL345_SetPowerMode(0x1);
    }


    //values of accelerometer
    short x = 1;
    short y = 1;
    short z = 1;

    position human_position;
    position last_human_position;
    int human_cleared = 0;

    human_position.x = 2;
    human_position.y = 2;

    position zombie_position;
    position last_zombie_position;
    int zombie_cleared = 0;

    zombie_position.x = 6;
    zombie_position.y = 1;


    //used for a cute animation
    position bricks[64];
    position brick;

    //array of holes to display
    position holes_position[3];


    ///////////////////////////////////////
    // Game modes for extra credit;
    const char* mode_names[] = {"EASY", "NORMAL", "HARD", "EXTREME"};
    int mode = 1;
    ///////////////////////////////////////

    ///////////////////////////////////////
    // The finite state machine
    enum States {TITLE, OPTIONS_MODE, RELEASE12, RELEASE3, LOAD, GAME, BLINKY, FADE, LOSE};
    enum States state = TITLE;
    enum States last_state;
    ///////////////////////////////////////


    while (1) {

        //get the values from the adxl functions
        ADXL345_GetXyz( &xcur, &ycur, &zcur);

        if( (xcur != x)||(ycur!=y)||(zcur!=z)){

            x = xcur;
            y = ycur;
            z = zcur;

        }
        switch(state){

            //////////////////////////////////////////////////
            // Title/splash screen. Has a "loading" bar and
            // lasts for about 2 seconds.
            case TITLE:

                    sprintf(oledstring, "     Zombie");
                    OledSetCursor(0, 0);
                    OledPutString(oledstring);

                    sprintf(oledstring, "   APOCALYPSE");
                    OledSetCursor(0, 1);
                    OledPutString(oledstring);


                    //  This is for the loading bar.
                    //  Things aren't realy loading,
                    //  but it looks nice.
                    for(i = 0; count > 150*i; i++){

                        if(i == 0){

                            OledSetCursor(i,3);
                            #ifdef USE_OLED_DRAW_GLYPH
                                 OledDrawGlyph(load_bar1_char);
                                 //OledUpdate();
                            #else
                                 OledPutChar(blank_char);
                            #endif
                        }
                        else if(i == 15){
                            OledSetCursor(i,3);
                            #ifdef USE_OLED_DRAW_GLYPH
                                 OledDrawGlyph(load_bar3_char);
                                 //OledUpdate();
                            #else
                                 OledPutChar(blank_char);
                            #endif
                        }
                        else if( i < 15){
                            OledSetCursor(i,3);
                            #ifdef USE_OLED_DRAW_GLYPH
                                 OledDrawGlyph(load_bar2_char);
                                 //OledUpdate();
                            #else
                                 OledPutChar(blank_char);
                            #endif

                        }


                    }
                    //keeping time for when to take to options screen
                    if(count > 2200){
                        OledClearBuffer();
                        state = OPTIONS_MODE;
                        count = 0;

                        //calling the function will make sure
                        //it will not skip the next state
                        ADXL345_SingleTapDetected();
                    }


                    break;


            ///////////////////////////////////////////////////////////
            // second screen of the options menu
            case OPTIONS_MODE:

                    last_state = OPTIONS_MODE;


                    sprintf(oledstring, "Set Mode:%4s", mode_names[mode]);
                    OledSetCursor(0, 0);
                    OledPutString(oledstring);


                    sprintf(oledstring, "Btn2: Mode++");
                    OledSetCursor(0, 1);
                    OledPutString(oledstring);


                    sprintf(oledstring, "Btn1: Mode--");
                    OledSetCursor(0, 2);
                    OledPutString(oledstring);


                    sprintf(oledstring, "Tap: Accept");
                    OledSetCursor(0, 3);
                    OledPutString(oledstring);


                    ////////////////////////////////////
                    // the below logic is to modify the
                    // mode, then sends to release state

                    if(Button1History  == 0xff){
                        if(mode > 0){
                            mode--;
                        }

                        state = RELEASE12;

                    }
                    else if(Button2History == 0xff){
                        if(mode < 3){
                            mode++;
                        }

                        state = RELEASE12;
                    }
                    else if(ADXL345_SingleTapDetected() == true){
                        while(ADXL345_SingleTapDetected()==1);
                        state = RELEASE3;
                        OledClearBuffer();
                    }

                    break;


            ////////////////////////////////////////////////////////
            // this state is for waiting for the release of
            // buttons 1 and 2. This is so that the options
            // menu only will increase goal or mode once per
            // button press
            case RELEASE12:



                if((Button1History == 0x00)&&(Button2History == 0x00)){

                    // clears top line for when mode or goal is modified
                    sprintf(oledstring, "      ");
                    OledSetCursor(10, 0);
                    OledPutString(oledstring);

                    state = OPTIONS_MODE;

                }


                break;

            /////////////////////////////////////////////////////////////
            // This is for initializing all of the characters
            // 
            //
            case RELEASE3:

                //seeding the random after user input
                srand(count);

                if(Button3History == 0x00){

                    //randomly placing characters
                    human_position.x = rand()%9;
                    human_position.y = rand()%4;
                    zombie_position.x = rand()%9;
                    zombie_position.y = rand()%4;


                    //check and make sure they don't spawn in the same spot
                    while(collision_check(human_position, zombie_position)!=0){

                        zombie_position.x = rand()%9;
                        zombie_position.y = rand()%4;

                    }

                    if(mode == 3){

                        for(i = 0; i < 3; i++){

                            holes_position[i].x = rand()%9;
                            holes_position[i].y = rand()%4;

                            while(collision_check(human_position, holes_position[i])!= 0 || collision_check(zombie_position, holes_position[i])!= 0){
                                holes_position[i].x = rand()%9;
                                holes_position[i].y = rand()%4;
                            }
                        }
                    }

                    count = 0;
                    state = LOAD;

                }


                break;


            /////////////////////////////////////////////////////
            // State will put the characters on the screen and then
            // give the player a countdown for when the game will
            // start.
            case LOAD:

                //countdown is based on the count
                countdown = (4000-count)/1000;

                //draw human and zombie
                draw_human(human_position);

                draw_zombie(zombie_position);

                //draw the holes only if in extreme mode
                if(mode == 3){
                    for(i = 0; i < 3; i++){
                        draw_hole(holes_position[i]);
                    }
                }

                sprintf(oledstring, "Start");
                OledSetCursor(10, 1);
                OledPutString(oledstring);

                sprintf(oledstring, "in: %d", countdown);
                OledSetCursor(10, 2);
                OledPutString(oledstring);

                if(count > 3000){

                    //just clearing the area that was just used
                    sprintf(oledstring, "     ");
                    OledSetCursor(10, 1);
                    OledPutString(oledstring);

                    sprintf(oledstring, "     ");
                    OledSetCursor(11, 2);
                    OledPutString(oledstring);

                    count = 0;
                    state = GAME;

                    //calling the function will make sure
                    //it will not skip the next state
                    while(ADXL345_SingleTapDetected()==1);
                }

                break;


            ////////////////////////////////////////////////////////////
            // GAME state.
            //
            case GAME:

                //attempt at making the movement seem less jumpy
                if(human_cleared == 1){
                    clear_glyph(last_human_position);
                    human_cleared = 0;
                }
                if(zombie_cleared == 1){
                    clear_glyph(last_zombie_position);
                    zombie_cleared = 0;
                }

                //score is the time in seconds
                score = count/1000;


                sprintf(oledstring, "SCORE");
                OledSetCursor(10, 1);
                OledPutString(oledstring);


                sprintf(oledstring, "%d", score);
                OledSetCursor(10, 2);
                OledPutString(oledstring);

                // check to see if the ADXL has been tapped
                if(ADXL345_SingleTapDetected() == 1){
                    OledClearBuffer();
                    state = OPTIONS_MODE;
                }

                /*this is for debugging purposes
                sprintf(oledstring, "X %4d", xcur);
                OledSetCursor(10, 1);
                OledPutString(oledstring);

                sprintf(oledstring, "Y %4d", ycur);
                OledSetCursor(10, 2);
                OledPutString(oledstring);

                sprintf(oledstring, "Z %4d", zcur);
                OledSetCursor(10, 3);
                OledPutString(oledstring);
                */


                draw_human(human_position);

                draw_zombie(zombie_position);

                // states in which the player moves through the game. If
                // the player runs into a wall, the function will not read
                // the adxl values and bounce the player off the wall.
                switch(human_state){

                    //the character is moving without having bumped into wall
                    case 0:
                        tilt = Tilt_Check(x,y);
                        if(human_count > Tilt_Check(x,y)){

                            last_human_position = human_position;
                            human_state = move_human(x, y, &human_position);

                            if(collision_check(human_position, last_human_position) == 0)
                                human_cleared = 1;
                            human_count = 0;

                            //check to see if you moved into the zombie
                            if(collision_check(human_position, zombie_position) == 1){
                                state = BLINKY;
                                OledClearBuffer();
                            }
                        }

                        break;

                        //bouncing upward
                    case 1:

                        if(human_count > tilt){

                            last_human_position = human_position;

                            human_position.y--;
                            human_count = 0;
                            human_state = 0;

                            if(collision_check(human_position, last_human_position) == 0)
                                human_cleared = 1;

                        }

                        break;

                        //bouncing downward
                    case 2:

                        if(human_count > tilt){
                            last_human_position = human_position;

                            human_position.y++;
                            human_count = 0;
                            human_state = 0;
                            if(collision_check(human_position, last_human_position) == 0)
                                human_cleared = 1;
                        }


                        break;

                        //bouncing to right
                    case 3:

                        if(human_count > tilt){

                            last_human_position = human_position;

                            human_position.x++;
                            human_count = 0;
                            human_state = 0;
                            if(collision_check(human_position, last_human_position) == 0)
                                human_cleared = 1;
                        }

                        break;

                        //bouncing to left
                    case 4:

                        if(human_count > tilt){
                            last_human_position = human_position;

                            human_position.x--;
                            human_count = 0;
                            human_state = 0;
                            if(collision_check(human_position, last_human_position) == 0)
                                human_cleared = 1;
                        }

                        break;

                }
                //////////////////////////////////////////////////////////////////////

                // implementing all the restrictions for when the game is in
                // extreme difficulty mode.
                if(mode == 3){


                    for(i = 0; i < 3; i++){

                        //check to see if the player fell into a hole
                        if(collision_check(human_position, holes_position[i]) == 1){
                            state = BLINKY;
                            OledClearBuffer();
                            count = 0;
                        }
                        draw_hole(holes_position[i]);

                    }
                }

                // general restrictions on the game
                if(Zombie_Count > (2000/(mode+1))){

                    last_zombie_position = zombie_position;

                    move_zombie(human_position, &zombie_position);

                    if(collision_check(zombie_position, last_zombie_position) == 0)
                        zombie_cleared = 1;
                    Zombie_Count = 0;

                }

                if(collision_check(human_position, zombie_position) == 1){
                    state = BLINKY;
                    count = 0;

                    draw_human(human_position);

                    draw_zombie(zombie_position);

                    OledClearBuffer();
                }

                break;

            ///////////////////////////////////////////////////////
            //  This state is for making the human character blink
            //  once the game is over.
            //
            case BLINKY:

                if(blinky_count < 3){
                    if(count < 250)
                        draw_human(human_position);
                    else if(count > 250 && count < 500)
                        clear_glyph(human_position);
                    else{
                        count = 0;
                        blinky_count++;
                    }
                }
                else if(blinky_count == 3){
                    blinky_count = 0;
                    state = FADE;
                    while(ADXL345_SingleTapDetected()!=0);

                }


                break;

            /////////////////////////////////////////////////////////
            // another cute animation for at the end of the game
            case FADE:

                if(count > 50){
                    brick.x = rand()%16;
                    brick.y = rand()%4;

                    //make sure it is place in a new spot
                    for(i = 0; i < 64; i++){

                        if(collision_check(brick, bricks[i])){
                            i = 0;
                            brick.x = rand()%16;
                            brick.y = rand()%4;
                        }

                    }
                    draw_brick(brick);
                    count = 0;
                    bricks[gencount].x = brick.x;
                    bricks[gencount].y = brick.y;
                    gencount++;
                }
                if(gencount == 64){
                    state = LOSE;
                    gencount = 0;

                    //reset the array of all the placed bricks
                    for(i = 0; i < 64; i ++){
                        bricks[i].x = -1;
                    }
                    OledClearBuffer();
                }

                break;

            ////////////////////////////////////////////////////////
            // Generic game over screen
            // Will allow you to start the game over if you tap the
            // accelerometer
            case LOSE:

                sprintf(oledstring, "GAME OVER");
                OledSetCursor(0, 0);
                OledPutString(oledstring);

                sprintf(oledstring, "Score: %d", score);
                OledSetCursor(0, 1);
                OledPutString(oledstring);

                sprintf(oledstring, "Tap to replay");
                OledSetCursor(0, 3);
                OledPutString(oledstring);

                if(ADXL345_SingleTapDetected() == 1){

                    OledClearBuffer();
                    state = OPTIONS_MODE;

                    //calling the function will make sure
                    //it will not skip the next state
                    while(ADXL345_SingleTapDetected()!=0);

                }

                break;

            ///////////////////////////////////////////////////////////
            //  Default state should never be invoked and so prints
            //  an error on the screen.
            //
            default:


                sprintf(oledstring, "ERROR");
                OledSetCursor(10, 1);
                OledPutString(oledstring);

                break;
        }

    }

    return (EXIT_SUCCESS);
}

/////////////////////////////////////////////////////////////
//  This function draws the human character at whatever
//  position.
//
void draw_human(position human){

     OledSetCursor(human.x, human.y);
#ifdef USE_OLED_DRAW_GLYPH
     OledDrawGlyph(human_char);
     OledUpdate();
#else
     OledPutChar(blank_char);
#endif

}
/////////////////////////////////////////////////////////////
//  This function will take in the tilt in the x and y
//  direction and change the position of the human
//
int move_human(short x, short y, position* human){

    int bounce = 0;

    if(y > 40 && y > x){
        if(human->y < 3)
            human->y++;
        else
            bounce = 1;
    }
    else if (y < -40 && y < x){
        if(human->y >0)
            human->y--;
        else
            bounce = 2;
    }


    if(x > 40 && x > y){
        if(human->x >0)
            human->x--;
        else
            bounce = 3;
    }
    else if(x < -40 && x < y){
        if(human->x <=8)
            human->x++;
        else
            bounce = 4;
    }

    return bounce;
}
////////////////////////////////////////////////////
//  Will check if two different positions are
//  in the same position
//
int collision_check(position human, position zombie){

    int status = 0;

    if((human.x == zombie.x) && (human.y == zombie.y))
        status = 1;

    return status;
}
////////////////////////////////////////////////////
//  Takes in the amount of tilt and and returns the
//  total tilt from being on a flat plane
//
int Tilt_Check(short x, short y){

    int total;

    if(abs(x) > abs(y))
        total = abs(x);
    else
        total = abs(y);

    total*=7;

    total = 1000-total;

    return total;

}
////////////////////////////////////////////
//  Draws the zombie at whatever position
//  you choose to draw
//
void draw_zombie(position zombie){



     OledSetCursor(zombie.x, zombie.y);
#ifdef USE_OLED_DRAW_GLYPH
     OledDrawGlyph(zombie_char);
     OledUpdate();
#else
     OledPutChar(blank_char);
#endif

}

/////////////////////////////////////////////////////////////
//  This is the function that moves the zombie. It takes in
//  the the position of the human and the zombie will
//  take a step closer to the human.
//
void move_zombie(position human, position* zombie){

    int stepx;
    int stepy;
    position absValue;

    stepx = zombie->x - human.x;
    stepy = zombie->y - human.y;

    absValue.x = abs(stepx);
    absValue.y = abs(stepy);

    if(absValue.x > absValue.y){
        stepx = stepx/(abs(stepx));
        stepy = 0;
    }
    else{
        stepy = stepy/(abs(stepy));
        stepx = 0;
    }

    zombie->x-=stepx;
    zombie->y-=stepy;

    
}

/////////////////////////////////////////////////////////////////
//  Puts a blank glyph anywhere you want a section cleared
//
void clear_glyph(position p1){



     OledSetCursor(p1.x, p1.y);
#ifdef USE_OLED_DRAW_GLYPH
     OledDrawGlyph(blank_char);
     OledUpdate();
#else
     OledPutChar(blank_char);
#endif

}

////////////////////////////////////////////////////
//  This function takes in a position to draw
//  the hole for the player to avoid
//
void draw_hole(position p1){


     OledSetCursor(p1.x, p1.y);
#ifdef USE_OLED_DRAW_GLYPH
     OledDrawGlyph(hole_char);
     OledUpdate();
#else
     OledPutChar(blank_char);
#endif


}

///////////////////////////////////////////////////////////
//  same as above, draws a brick at a location specified
//
void draw_brick(position p1){

    OledSetCursor(p1.x, p1.y);
#ifdef USE_OLED_DRAW_GLYPH
     OledDrawGlyph(brick_char);
     OledUpdate();
#else
     OledPutChar(blank_char);
#endif

}
