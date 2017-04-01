/*************************************************** 
  This is a Pong port for the Arduino. The application 
  uses an Arduino Uno, Adafruit’s 128x64 OLED display, 
  2 potentiometers and an piezo buzzer.

  More info about this project can be found on my blog:
  http://michaelteeuw.nl

  Written by Michael Teeuw | Xonay Labs.
  Apache 2 license, all text above must be included 
  in any redistribution.
 ****************************************************/

/*************************************************** 
  The game now uses the U8G library, so it can
  handle my OLED display from ebay, what have some
  problems with the Adafruit library.

  Partialy re-written by Bence Darabos
  Apache 2 license, all text above must be included 
  in any redistribution.
 ****************************************************/

#include "pitches.h"
#include "U8glib.h"
// Hardware SPI version
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);  // I2C / TWI 
// Software SPI version
#define OLED_MOSI   11
#define OLED_CLK   12
#define OLED_DC    9
#define OLED_CS    8
#define OLED_RESET 10
//U8GLIB_SSD1306_ADAFRUIT_128X64 u8g(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC,OLED_RESET);

//Define Pinse
#define BEEPER 3
#define CONTROL_A A0
#define CONTROL_B A1

//Define Visuals
#define FONT_SIZE_W 4
#define FONT_SIZE_H 10
#define SCREEN_WIDTH 127  //real size minus 1, because coordinate system starts with 0
#define SCREEN_HEIGHT 63  //real size minus 1, because coordinate system starts with 0
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 10
#define PADDLE_PADDING 10
#define BALL_SIZE 3
#define SCORE_PADDING 10

#define EFFECT_SPEED 0.5
#define MIN_Y_SPEED 0.5
#define MAX_Y_SPEED 2
enum {
  two_player_mode=0,
  one_player_mode=1
} GAME_MODE = two_player_mode;

#define AI_HANDICAP 2 // 1-player AI strongness. 1 is unbeatable, higher makes AI easier to beat.

#define WIN_SCORE 15 // The winner has to reach this score AND score 2 points more than the other!

//Define Variables

int paddleLocationA = 0;
int paddleLocationB = 0;

float ballX = SCREEN_WIDTH/2;
float ballY = SCREEN_HEIGHT/2;
float ballSpeedX = 4;
float ballSpeedY = 2;

int lastPaddleLocationA = 0;
int lastPaddleLocationB = 0;

int scoreA = 0;
int scoreB = 0;
int winner = 0;

int start = 0;
int controlAstart;
int controlBstart;

int ai_count=0;

//Setup 
void setup(){
  controlAstart = analogRead(CONTROL_A);
  controlBstart = analogRead(CONTROL_B);
}

// //Splash Screen
void splash()
{
  if(winner != 0) {
    centerPrint("WINNER IS",0, 2);
    if(winner==1)  centerPrint("PLAYER 1",18,2);
    else centerPrint("PLAYER 2",18,2);
  } else {
    centerPrint("PONG",0, 2);
    centerPrint("By Allan Alcorn",18,1);
    centerPrint("MichaelTeeuw.nl",27, 1);
    centerPrint("Bence Darabos",36, 1);
  }
  centerPrint("Move paddle to start!",SCREEN_HEIGHT-11, 1);

  if (abs(controlAstart - analogRead(CONTROL_A) + controlBstart - analogRead(CONTROL_B)) > 50) {
    //show as long as the total absolute change of 
    //both potmeters is smaler than 5
    soundStart();
    delay(200);
    start = 1;
  }
}

//Loop
void loop()
{
  calculateMovement();

  u8g.firstPage();
  do {
    draw();
  u8g.setColorIndex(1);
  } while( u8g.nextPage() );
}

void calculateMovement() 
{
  if(start == 0){}
  else{
    int controlA = analogRead(CONTROL_A);
    int controlB = analogRead(CONTROL_B);

    paddleLocationA = map(controlA, 0, 1023, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
    
    if(GAME_MODE == two_player_mode){
    paddleLocationB = map(controlB, 0, 1023, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
    }
    else{//One player mode
      ai_count++;
      if(!(ai_count%AI_HANDICAP)) {
        // To the current location, add the half distance between the current location and the ideal position,
        paddleLocationB = paddleLocationB - (paddleLocationB - round(abs(ballY - PADDLE_HEIGHT / 2)))/2;
        ai_count=0;
      }
    }

    int paddleSpeedA = paddleLocationA - lastPaddleLocationA;
    int paddleSpeedB = paddleLocationB - lastPaddleLocationB;

    ballX += ballSpeedX;
    ballY += ballSpeedY;

    //bounce from top and bottom
    if (ballY >= SCREEN_HEIGHT - BALL_SIZE || ballY <= 0) {
      ballSpeedY *= -1;
      soundBounce();
    }

    //bounce from paddle A
    if (ballX >= PADDLE_PADDING && ballX <= PADDLE_PADDING+BALL_SIZE && ballSpeedX < 0) {
      if (ballY > paddleLocationA - BALL_SIZE && ballY < paddleLocationA + PADDLE_HEIGHT) {
        soundBounce();
        ballSpeedX *= -1;
      
        addEffect(paddleSpeedA);
      }
    }

    //bounce from paddle B
    if (ballX >= SCREEN_WIDTH-PADDLE_WIDTH-PADDLE_PADDING-BALL_SIZE && ballX <= SCREEN_WIDTH-PADDLE_PADDING-BALL_SIZE && ballSpeedX > 0) {
      if (ballY > paddleLocationB - BALL_SIZE && ballY < paddleLocationB + PADDLE_HEIGHT) {
        soundBounce();
        ballSpeedX *= -1;
      
        addEffect(paddleSpeedB);
      }

    }

    //score points if ball hits wall behind paddle
    if (ballX >= SCREEN_WIDTH - BALL_SIZE || ballX <= 0) {
      if (ballSpeedX > 0) {
        scoreA++;
        ballX = SCREEN_WIDTH / 4;
      }
      if (ballSpeedX < 0) {
        scoreB++;
        ballX = SCREEN_WIDTH / 4 * 3;
      }

      soundPoint();

      // The winner is the one that reached the winner score and scored 2 more points than the other.
      if( (scoreA>=WIN_SCORE || scoreB>=WIN_SCORE) && (abs(scoreA-scoreB)>=2)) {
        if(scoreA>=WIN_SCORE) winner=1; else winner=2;
        // Reset the game
        start = 0;
        ballX = SCREEN_WIDTH/2;
        ballY = SCREEN_HEIGHT/2;
        ballSpeedX = 2;
        ballSpeedY = 1;

        lastPaddleLocationA = 0;
        lastPaddleLocationB = 0;

        scoreA = 0;
        scoreB = 0;

        soundWinner();

        controlAstart = analogRead(CONTROL_A);
        controlBstart = analogRead(CONTROL_B);
      }
    }

    //set last paddle locations
    lastPaddleLocationA = paddleLocationA;
    lastPaddleLocationB = paddleLocationB;  
  }
}

void draw() 
{
  if(start == 0){
    splash();
  }
  else{
    //draw paddle A
    u8g.drawVLine(PADDLE_PADDING, paddleLocationA, PADDLE_HEIGHT);
    u8g.drawVLine(PADDLE_PADDING+1, paddleLocationA, PADDLE_HEIGHT);

    //draw paddle B
    u8g.drawVLine(SCREEN_WIDTH-PADDLE_WIDTH-PADDLE_PADDING, paddleLocationB, PADDLE_HEIGHT);
    u8g.drawVLine(SCREEN_WIDTH-PADDLE_WIDTH-PADDLE_PADDING+1, paddleLocationB, PADDLE_HEIGHT);

    //draw center line
    for (int i=0; i<SCREEN_HEIGHT; i+=4) {
      u8g.drawVLine(SCREEN_WIDTH/2, i, 2);
    }

    //draw ball
    for (int i=0; i<BALL_SIZE; i++) {
      u8g.drawVLine(ballX, ballY, BALL_SIZE);
      u8g.drawVLine(ballX+i, ballY, BALL_SIZE);
      u8g.drawVLine(ballX+i, ballY, BALL_SIZE);
    }
    //print scores

    //backwards indent score A. This is dirty, but it works ... ;)
    int scoreAWidth = FONT_SIZE_W;
    if (scoreA > 9) scoreAWidth += FONT_SIZE_W * 2;
    if (scoreA > 99) scoreAWidth += FONT_SIZE_W * 2;
    if (scoreA > 999) scoreAWidth += FONT_SIZE_W * 2;
    if (scoreA > 9999) scoreAWidth += FONT_SIZE_W * 2;
    
    u8g.setFont(u8g_font_unifont);

    u8g.setPrintPos(SCREEN_WIDTH/2 - SCORE_PADDING - scoreAWidth,FONT_SIZE_H+4);
    u8g.print(scoreA);

    u8g.setPrintPos(SCREEN_WIDTH/2 + SCORE_PADDING+1,FONT_SIZE_H+4);
    u8g.print(scoreB);
  }
} 


void addEffect(int paddleSpeed)
{
  float oldBallSpeedY = ballSpeedY;

  //add effect to ball when paddle is moving while bouncing.
  //for every pixel of paddle movement, add or substact EFFECT_SPEED to ballspeed.
  for (int effect = 0; effect < abs(paddleSpeed); effect++) {
    if (paddleSpeed > 0) {
      ballSpeedY += EFFECT_SPEED;
    } else {
      ballSpeedY -= EFFECT_SPEED;
    }
  }

  //limit to minimum speed
  if (ballSpeedY < MIN_Y_SPEED && ballSpeedY > -MIN_Y_SPEED) {
    if (ballSpeedY > 0) ballSpeedY = MIN_Y_SPEED;
    if (ballSpeedY < 0) ballSpeedY = -MIN_Y_SPEED;
    if (ballSpeedY == 0) ballSpeedY = oldBallSpeedY;
  }

  //limit to maximum speed
  if (ballSpeedY > MAX_Y_SPEED) ballSpeedY = MAX_Y_SPEED;
  if (ballSpeedY < -MAX_Y_SPEED) ballSpeedY = -MAX_Y_SPEED;
}

void soundStart() 
{
  tone(BEEPER, NOTE_B3);
  delay(100);
  tone(BEEPER, NOTE_B4);
  delay(100);
  tone(BEEPER, NOTE_B5);
  delay(100);
  noTone(BEEPER);
}

void soundWinner() 
{
  int melody[] = {
    NOTE_G4, NOTE_A4, NOTE_B4,NOTE_C4,0,NOTE_A4,NOTE_C4};

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {
    8, 8, 8, 8,4,8,4 };

  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    tone(BEEPER, melody[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration;
    delay(pauseBetweenNotes);
    noTone(BEEPER);
  }
}

void soundBounce() 
{
  tone(BEEPER, NOTE_B4, 50);
}

void soundPoint() 
{
  tone(BEEPER, NOTE_D3, 150);
}

void centerPrint(char *text, int y, int font)
{
  if(font==1){
    u8g.setFont(u8g_font_6x10);
    u8g.setPrintPos(SCREEN_WIDTH/2-((strlen(text))*6)/2, y + FONT_SIZE_H);
  }
  else{
    u8g.setFont(u8g_font_unifont);
    u8g.setPrintPos(SCREEN_WIDTH/2-((strlen(text))*8)/2, y + FONT_SIZE_H);
  }
  u8g.print(text);
}
