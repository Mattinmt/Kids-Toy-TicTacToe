#include <Charlieplex.h>
#include <Arduino.h>

byte pins[5] = {5, 6, 9, 10, 11};
Charlieplex charlie(pins, sizeof(pins));
int i = 0;

const int green[3][3] = { // Green is the player
  {0, 8, 14},
  {2, 10, 16},
  {4, 12, 6}
};
const int red[3][3] = { // Red is the Arduino
  {1, 9, 15},
  {3, 11, 17},
  {5, 13, 7}
};
const int button = A0; // The analog pin the button matrix is connected to
const int resButtons[3][3] = { // The resistance thresholds for the buttons
  {800, 400, 200},
  {160, 140, 120},
  {90, 85, 70}
};

const int greenWin = 18; // Lights if the player wins
const int redWin = 19; // Lights if the Arduino wins
const int resetButton = 13; // Button to start a new game

const int win[8][3][3] = { // This 4D array defines all possible winning combinations
  {
    {1, 1, 1},
    {0, 0, 0},
    {0, 0, 0}
  },
  {
    {0, 0, 0},
    {1, 1, 1},
    {0, 0, 0}
  },
  {
    {0, 0, 0},
    {0, 0, 0},
    {1, 1, 1}
  },
  {
    {1, 0, 0},
    {1, 0, 0},
    {1, 0, 0}
  },
  {
    {0, 1, 0},
    {0, 1, 0},
    {0, 1, 0}
  },
  {
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1}
  },
  {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
  },
  {
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 0}
  }
};

// Global variables
int gamePlay[3][3] = { // Holds the current game
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};
int squaresLeft = 9; // The number of free squares left on the board
int played = 0; // Has the Arduino played or not

// Global constants
const int arduinoDelay = 3000; // How long the Arduino waits before playing

void setup() {
  // put your setup code here, to run once:
  // Start serial comms
  Serial.begin(9600);
  // Define buttons as inputs
  pinMode(button, INPUT);
  //Define reset button as input
  pinMode(resetButton, INPUT);
  initialise();
}

void initialise() {
  // Prepare the board for a game
  Serial.println("Initialising...");
  // Set green and red LEDs off
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      charlie.setLed(green[i][j], false);
      charlie.setLed(red[i][j], false);
      gamePlay[i][j] = 0;
    }
  }
  // Set win LEDs off
  charlie.setLed(greenWin, false);
  charlie.setLed(redWin, false);

  // Reset variables
  squaresLeft = 9;
}
void loop() {
  // put your main code here, to run repeatedly:
  // Wait for an input and call the buttonPress routine if pressed
  int upper = 10000;
  if (analogRead(button) != 0) {
    int x;
    int y;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (gamePlay[i][j] == 0) {
          if ((analogRead(button) > resButtons[i][j]) && (analogRead(button) < upper)) {
            buttonPress(i, j);
          }
        }
        upper = resButtons[i][j];
      }
    }
  }
  charlie.loop();
}

void buttonPress(int i, int j) {
  // Button pressed, light the green LED and note the square as taken
  Serial.print("Button press ");
  Serial.print(i);
  Serial.print(":");
  Serial.println(j);
  charlie.setLed(green[i][j], true);
  gamePlay[i][j] = 1; // Note the square played
  squaresLeft -= 1; // Update number of squares left
  printGame(); // Print the game to serial monitor
  checkGame(); // Check for a winner
  arduinosTurn(); // Arduino's turn
}

void arduinosTurn() {
  // Arduino takes a turn
  Serial.println("Arduino's turn");
  checkPossiblities(); // Check to see if a winning move can be played
  if (played == 0) {
    checkBlockers(); // If no winning move played, check to see if we can block a win
  }
  if (played == 0) {
    randomPlay(); // Otherwise, pick a random square
  }
  squaresLeft -= 1; // Update number of squares left
  played = 0; // Reset if played or not
  printGame(); // Print the games to serial monitor
  checkGame(); // Check for a winner
}


void checkPossiblities() {
  // Check all rows, then columns, then diagonals to see if there are two reds lit and a free square to make a line of three
  Serial.println("Checking possibilities to win...");
  int poss = 0; // Used to count if possible - if it gets to 2 then its a possiblity
  int x = 0; // The X position to play
  int y = 0; // The Y position to play
  int space = 0; // Is there a free square or not to play
  // Check rows
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gamePlay[i][j] == 2) {
        poss += 1; // Square is red. Increment the possiblity counter
      }
      if (gamePlay[i][j] == 0) {
        space = 1; // Square is empty. Note this and the position
        x = i;
        y = j;
      }
      if ((poss == 2) && (space == 1)) { // 2 red squares and a free square
        Serial.print("Found an obvious row! ");
        Serial.print(x);
        Serial.println(y);
        playPoss(x, y);
      }
    }
    poss = 0;
    x = 0;
    y = 0;
    space = 0;
  }
  // Check columns - same as for rows but the "for" loops have been reversed to go to columns
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gamePlay[j][i] == 2) {
        poss += 1;
      }
      if (gamePlay[j][i] == 0) {
        space = 1;
        x = j;
        y = i;
      }
      if ((poss == 2) && (space == 1) && (played == 0)) {
        // This time also check if we've already played
        Serial.print("Found an obvious column! ");
        Serial.print(x);
        Serial.println(y);
        playPoss(x, y);
      }
    }
    // Reset variables
    poss = 0;
    x = 0;
    y = 0;
    space = 0;
  }
  // Check crosses - as for rows and columns but "for" loops changed
  // Check diagonal top left to bottom right
  for (int i = 0; i < 3; i++) {
    if (gamePlay[i][i] == 2) {
      poss += 1;
    }
    if (gamePlay[i][i] == 0) {
      space = 1;
      x = i;
      y = i;
    }
    if ((poss == 2) && (space == 1) && (played == 0)) {
      Serial.print("Found an obvious cross! ");
      Serial.print(x);
      Serial.println(y);
      playPoss(x, y);
    }
  }
  // Reset variables
  poss = 0;
  x = 0;
  y = 0;
  space = 0;
  // Check diagonal top right to bottom left
  int row = 0; // Used to count up the rows
  for (int i = 2; i >= 0; i--) { // We count DOWN the columns
    if (gamePlay[row][i] == 2) {
      poss += 1;
    }
    if (gamePlay[row][i] == 0) {
      space = 1;
      x = row;
      y = i;
    }
    if ((poss == 2) && (space == 1) && (played == 0)) {
      Serial.print("Found an obvious cross! "); Serial.print(x); Serial.println(y);
      playPoss(x, y);
    }
    row += 1; // Increment the row counter
  }
  // Reset variables
  poss = 0;
  x = 0;
  y = 0;
  space = 0;
}

void checkBlockers() {
  // As for checkPossibilites() but this time checking the players squares for places to block a line of three
  Serial.println("Checking possibilities to block...");
  int poss = 0;
  int x = 0;
  int y = 0;
  int space = 0;
  // Check rows
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gamePlay[i][j] == 1) {
        poss += 1;
      }
      if (gamePlay[i][j] == 0) {
        space = 1;
        x = i;
        y = j;
      }
      if ((poss == 2) && (space == 1) && (played == 0)) {
        Serial.print("Found an blocker row! "); Serial.print(x); Serial.println(y);
        playPoss(x, y);
      }
    }
    poss = 0;
    x = 0;
    y = 0;
    space = 0;
  }// Check columns
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gamePlay[j][i] == 1) {
        poss += 1;
      }
      if (gamePlay[j][i] == 0) {
        space = 1;
        x = j;
        y = i;
      }
      if ((poss == 2) && (space == 1) && (played == 0)) {
        Serial.print("Found an blocker column! ");
        Serial.print(x);
        Serial.println(y);
        playPoss(x, y);
      }
    }
    poss = 0;
    x = 0;
    y = 0;
    space = 0;
  }// Check crosses
  for (int i = 0; i < 3; i++) {
    if (gamePlay[i][i] == 1) {
      poss += 1;
    }
    if (gamePlay[i][i] == 0) {
      space = 1;
      x = i;
      y = i;
    }
    if ((poss == 2) && (space == 1) && (played == 0)) {
      Serial.print("Found an blocker cross! ");
      Serial.print(x);
      Serial.println(y);
      playPoss(x, y);
    }
  }
  poss = 0;
  x = 0;
  y = 0;
  space = 0;
  int row = 0;
  for (int i = 2; i >= 0; i--) {
    if (gamePlay[row][i] == 1) {
      poss += 1;
    }
    if (gamePlay[row][i] == 0) {
      space = 1;
      x = row;
      y = i;
    }
    if ((poss == 2) && (space == 1) && (played == 0)) {
      Serial.print("Found an blocker cross! ");
      Serial.print(x);
      Serial.println(y);
      playPoss(x, y);
    }
    row += 1;
  }
  poss = 0;
  x = 0;
  y = 0;
  space = 0;
}

void randomPlay() {
  // No win or block to play... Let's just pick a square at random
  Serial.println("Choosing randomly...");
  int choice = random(1, squaresLeft); // We pick a number from 0 to the number of squares left on the board
  Serial.print("Arduino chooses ");
  Serial.println(choice);
  int pos = 1; // Stores the free square we're currently on
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (gamePlay[i][j] == 0) { // Check to see if square empty
        if (pos == choice) { // Play the empty square that corresponds to the random number
          playPoss(i, j);
        }
        pos += 1; // Increment the free square counter
      }
    }
  }
}

void playPoss(int x, int y) {
  // Simulate thought and then play the chosen square
  int delayStop = millis() + arduinoDelay;
  while (millis() < delayStop) {
    charlie.loop();
  }
  charlie.setLed(red[x][y], true);
  gamePlay[x][y] = 2; // Update the game play array
  played = 1; // Note that we've played
}

void checkGame() {
  // Check if the player has won
  //Serial.println("Checking for a winner");
  int player = 1;
  int winner = 0;
  for (int i = 0; i < 8; i++) {
    // We cycle through all winning combinations in the 4D array and check if they correspond to the current game
    //Check game
    int winCheck = 0;
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        if ((win[i][j][k] == 1) && (gamePlay[j][k] == player)) {
          winCheck += 1;
        }
      }
    }
    if (winCheck == 3) {
      winner = 1;
      Serial.print("Player won game ");
      Serial.println(i);
      endGame(1);
    }
  }

  // Do the same for to check if the Arduino has won
  player = 1;
  winner = 0;
  for (int i = 0; i < 8; i++) {
    int winCheck = 0;
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        if ((win[i][j][k] == 1) && (gamePlay[j][k] == player)) {
          winCheck += 1;
        }
      }
    }
    if (winCheck == 3) {
      winner = 1;
      Serial.print("Arduino won game ");
      Serial.println(i);
      endGame(2);
    }
  }
}

void printGame() {
  // Prints the game to the serial monitor
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      Serial.print(gamePlay[i][j]);
      Serial.print(" ");
    }
    Serial.println("");
  }
  Serial.print(squaresLeft);
  Serial.println(" squares left");
}

void endGame(int winner) {
  // Is called when a winner is found
  switch (winner) {
    case 0:
      Serial.println("It's a draw");
      charlie.setLed(greenWin, true);
      charlie.setLed(redWin, true);
      break;
    case 1:
      Serial.println("Player wins");
      charlie.setLed(greenWin, true);
      break;
    case 2:
      Serial.println("Arduino wins");
      charlie.setLed(redWin, true);
      break;
  }
  while (digitalRead(resetButton) == LOW) {
    charlie.loop();
  }
  initialise();
}
