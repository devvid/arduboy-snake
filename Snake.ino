// Snake game for Arduino / Arduboy hardware
// Made by David Cedar / Supereasyelectronics / Devvid 2018
// Repo https://github.com/devvid/arduboy-snake

#include <Arduboy2.h>
#include <EEPROM.h>

// 128x64 / 4 = 32x16
#define WORLD_WIDTH  16
#define WORLD_HEIGHT 8
#define SCALE_FACTOR 8

// make an instance of arduboy used for many functions
Arduboy2 arduboy;

// Game state
enum state {
    TITLE,
    GAME,
    WIN,
    GAMEOVER,
    RESET
};
enum state GAMESTATE = RESET;
// Position & Button enum
enum pos {X = 0, Y = 1};
enum button {A = 0, B = 1, UP = 2, LEFT = 3, DOWN = 4, RIGHT = 5 };

// Game variables
byte foodPos[2] = {0};                                             // The position of the food. Currently we are only showing one piece
boolean pressed [6] = {false, false, false, false , false, false}; // Check if button A & B is pressed already
int delayTime = 300;                                               // The delay the game will use to control movement
unsigned long previousTime;                                        // Used with the control of the deplay
int timeDecay = 50;                                                // Use will use an exponential decay to control play speed. Speed up the game over time.
byte scoreHuman = 0;                                                    // Keep track of the player score, this is saved into EEPROM when gameover.
int snakeHumanDirection = 3;                                       // The direction of the snake. 0 top, 1 left, 2 down, 3 right
byte snakeHuman[WORLD_WIDTH*WORLD_HEIGHT][2] = {0};                // The snake body is a 2d array, rows consisting of the body parts and col the x & y cords [The long body][y & x]



// Manage vector like function using 2d array instead.
void push_front_array(int, int, byte [WORLD_WIDTH*WORLD_HEIGHT][2]);
void push_back_array(byte [WORLD_WIDTH*WORLD_HEIGHT][2]);
void push_back_array(int, int, byte [WORLD_WIDTH*WORLD_HEIGHT][2]);
byte get_array_end(byte [WORLD_WIDTH*WORLD_HEIGHT][2]);
void pop_array_element(int , int , byte [WORLD_WIDTH*WORLD_HEIGHT][2]);
void clear_array(byte [WORLD_WIDTH*WORLD_HEIGHT][2]);

// Time function: We still want to listen to our button presses in real time.
// else we wan to delay our game soo the player ... steps 
void timeCritial(){
    // Delay the system so we create the stepping effect while still listeniig to controls
    previousTime = millis();
    while((millis() - previousTime) < exp(-scoreHuman/timeDecay)*delayTime){
        // Player control D Pad
        if(arduboy.pressed(RIGHT_BUTTON) and !pressed[RIGHT]){
            snakeHumanDirection++;
            if(snakeHumanDirection == 4) snakeHumanDirection = 0;
            pressed[RIGHT] = true;
        }
        if(arduboy.pressed(LEFT_BUTTON) and !pressed[LEFT]){
            snakeHumanDirection--;
            if(snakeHumanDirection == -1) snakeHumanDirection = 3;
            pressed[LEFT] = true;
        }

        //Check if the button is being held down
        if(arduboy.notPressed(UP_BUTTON)) pressed[UP] = false;
        if(arduboy.notPressed(LEFT_BUTTON)) pressed[LEFT] = false;
        if(arduboy.notPressed(DOWN_BUTTON)) pressed[DOWN] = false;
        if(arduboy.notPressed(RIGHT_BUTTON)) pressed[RIGHT] = false;
    }    
}

void placeFood(){
    // Set the Food to a random spot on the map
    foodPos[X] = 0;
    foodPos[Y] = 0;
    while(foodPos[X] == 0 and foodPos[Y] == 0 ){
        foodPos[X] = random(1,WORLD_WIDTH-1);
        foodPos[Y] = random(1,WORLD_HEIGHT-1);
        for(int i = 0; i < WORLD_HEIGHT*WORLD_HEIGHT; i++){
            if(foodPos[X] == snakeHuman[i][X] && foodPos[Y] == snakeHuman[i][Y]){
                foodPos[X] = 0;
                foodPos[Y] = 0;
            }
        }
    }
}

// This function runs once in your game.
// use it for anything that needs to be set only once in your game.
void setup() {
    // initiate arduboy instance
    arduboy.begin();

    //Seed the random number generator
	arduboy.initRandomSeed();

    // here we set the framerate to 15, we do not need to run at
    // default 60 and it saves us battery life
    arduboy.setFrameRate(60);
    arduboy.clear();
}

// our main game loop, this runs once every cycle/frame.
// this is where our game logic goes.
void loop() {
    // pause render until it's time for the next frame
    if (!(arduboy.nextFrame()))
    return;

    // first we clear our screen to black and start from top left corner
    arduboy.clear();
    arduboy.setCursor(0, 0);

    
    // Game Logic
    switch(GAMESTATE){
        case RESET:
            // Initilise Stack
            for (byte y = 0; y < WORLD_HEIGHT*WORLD_WIDTH; y++)
                for ( byte x = 0; x < 2; x++)
                    snakeHuman[y][x] = 255;

            // Set player
            snakeHuman[0][X] = random(1,WORLD_WIDTH-1);
            snakeHuman[0][Y] = random(1,WORLD_HEIGHT-1); 

            // Set the Food to a random spot on the map
            placeFood();

            // Reset score
            scoreHuman = 0;

            // Proced to the title screen.
            GAMESTATE = TITLE;
            break;

        case TITLE:
            arduboy.setCursor(0, 0);
            arduboy.setTextSize(2);
			      arduboy.print("Snake V2\n");
            arduboy.setTextSize(1);
            arduboy.print("Controls: \nLeft & Right pad\n");
            arduboy.print("Press A to play\n\n");
            //arduboy.setCursor(0, 45);
            arduboy.print("David Cedar 2018\n");
            arduboy.print("www.devvid.com/snake");
            
            if(arduboy.pressed(A_BUTTON) and !pressed[A]) {
      				pressed[A] = true;
      				GAMESTATE = GAME;
      			}
            break;

        case GAME:

            // Slow down game: This makes the whole game freeze, not so good as we lose input.
            //delay(exp(-score/50)*delayTime);
    
            // Set Direction. & Slow the game using tight loop whiles responding to button clicks
            timeCritial();

            // Body movement logic
            switch (snakeHumanDirection){
                case 0: // UP
                    if(snakeHuman[0][Y] - 1 < 0) snakeHuman[0][Y] = 8; 
                    push_front_array(snakeHuman[0][X], snakeHuman[0][Y] - 1, snakeHuman );
                    break;
                case 1: // RIGHT
                    if(snakeHuman[0][X] + 1 > 15) snakeHuman[0][X] = -1; 
                    push_front_array(snakeHuman[0][X] + 1, snakeHuman[0][Y], snakeHuman );
                    break;
                case 2: // DOWN
                    if(snakeHuman[0][Y] + 1 > 7) snakeHuman[0][Y] = -1; 
                    push_front_array(snakeHuman[0][X], snakeHuman[0][Y] + 1, snakeHuman );
                    break;
                case 3: // LEFT
                    if(snakeHuman[0][X] - 1 < 0) snakeHuman[0][X] = 16;
                    push_front_array(snakeHuman[0][X] - 1, snakeHuman[0][Y], snakeHuman );
                    break;
            }

            // Chop off tail
            pop_back_array(snakeHuman);

            // Collision check with Body
            for(byte i = 1; i < WORLD_HEIGHT*WORLD_WIDTH; i++){
                if (snakeHuman[0][X] == snakeHuman[i][X] && snakeHuman[0][Y] == snakeHuman[i][Y])
                    GAMESTATE = GAMEOVER;
            }

            // Collision detection snakeHuman v Food
            if(snakeHuman[0][X] == foodPos[X] && snakeHuman[0][Y] == foodPos[Y]) {
                placeFood();
                scoreHuman++;
                push_back_array(snakeHuman);
            }

            // Drawing! inc player, head, tail, fruit
            arduboy.setCursor(0, 0);
            // Draw fruit
            arduboy.drawCircle((foodPos[X] * SCALE_FACTOR) + SCALE_FACTOR/2, (foodPos[Y] * SCALE_FACTOR) + SCALE_FACTOR/2, SCALE_FACTOR/2, WHITE);
            // Draw snakeHuman
            for (byte i = 0; i < get_array_end(snakeHuman); i++)
                arduboy.fillRect(snakeHuman[i][X] * SCALE_FACTOR, snakeHuman[i][Y] * SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR, WHITE);
            break;

        case WIN:
            // Do you ever win?
            break;

        case GAMEOVER:
            // Read the EEPROM for save data.
            byte savedScore = EEPROM.read(0);
            if ( savedScore == 255) savedScore = 0;

            arduboy.setCursor(0, 0);
            arduboy.print("GAME OVER!");
            arduboy.print("\n\n");
            arduboy.print("Score: ");
            arduboy.print(scoreHuman);
            arduboy.print("\nHigh Score: ");
            arduboy.print(savedScore);
            arduboy.print("\n\n");
            arduboy.print("Press A to Play Again");
            
            // Save to EEPROM if new high score.
            if (scoreHuman > savedScore) EEPROM.update(0, scoreHuman);
            
            if(arduboy.pressed(A_BUTTON) and !pressed[A]) {
                pressed[A] = true;
                GAMESTATE = RESET;
            }
            break;    
    }

    // Reset the game if both A and B are pressed.
    if(arduboy.pressed(A_BUTTON) && arduboy.pressed(B_BUTTON) && !pressed[A] && !pressed[B]) GAMESTATE = RESET;
    
    //Check if the button is being held down
    if(arduboy.notPressed(A_BUTTON)) pressed[A] = false;
    if(arduboy.notPressed(B_BUTTON)) pressed[B] = false;
    if(GAMESTATE != GAME){
        // Dont both when in GAME as timeCritial() takes care of D Pad
        if(arduboy.notPressed(UP_BUTTON)) pressed[UP] = false;
        if(arduboy.notPressed(LEFT_BUTTON)) pressed[LEFT] = false;
        if(arduboy.notPressed(DOWN_BUTTON)) pressed[DOWN] = false;
        if(arduboy.notPressed(RIGHT_BUTTON)) pressed[RIGHT] = false;
        }
    // then we finaly we tell the arduboy to display what we just wrote to the display
    arduboy.display();
}

// ######################################
// Manage vector like effects
// ######################################
void push_front_array(int x, int y, byte tailStack[WORLD_WIDTH*WORLD_HEIGHT][2]){
	// Push
	int stopByte = get_array_end(tailStack);
	if (stopByte == 255) return;
    if (stopByte > 0 && stopByte < 255) {
        // Move elements down one, ready to push to top
        for(int i = stopByte - 1; i > -1; i--){
            tailStack[i + 1][X] = tailStack[i][X];
            tailStack[i + 1][Y] = tailStack[i][Y];
        }
    }  
    tailStack[0][X] = x;
    tailStack[0][Y] = y;
}
void push_back_array(byte tailStack[WORLD_WIDTH*WORLD_HEIGHT][2]){
    int stopByte = get_array_end(tailStack);
    tailStack[stopByte][X] = tailStack[stopByte - 1][X];
    tailStack[stopByte][Y] = tailStack[stopByte - 1][Y];
}
void push_back_array(int x, int y, byte tailStack[WORLD_WIDTH*WORLD_HEIGHT][2]){
    int stopByte = get_array_end(tailStack);
    tailStack[stopByte][X] = x;
    tailStack[stopByte][Y] = y;
}
// Returns the actual end byte, in other words not the final real data the user wants. kinda like '\0'
byte get_array_end(byte tailStack[WORLD_WIDTH*WORLD_HEIGHT][2]){
	for (int i = 0; i < WORLD_WIDTH*WORLD_HEIGHT; i++){
		if (tailStack[i][X] == 255) return i;
	}
	return 255;
}
void pop_back_array(byte tailStack[WORLD_WIDTH*WORLD_HEIGHT][2]){
    int stopByte = get_array_end(tailStack);
    tailStack[stopByte - 1][X] = 255;
    tailStack[stopByte - 1][Y] = 255;
}
void clear_array(byte tailStack[WORLD_WIDTH*WORLD_HEIGHT][2]){
	for (int i = 0; i < WORLD_WIDTH*WORLD_HEIGHT; i++){
		tailStack[i][X] = 255;
        tailStack[i][Y] = 255;
    }
}
