#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display configuration for Heltec WiFi Kit 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Touch-capable pins for Heltec WiFi Kit 32
// Jump pins: 22, 23, 19, 18, 5, 17, 21
const int jumpPins[] = {22, 23, 19, 18, 5, 17, 21};
const int numJumpPins = 7;

// Duck pins: 36, 37, 38, 39, 34, 35 (analog input pins)
const int duckPins[] = {36, 37, 38, 39, 34, 35};
const int numDuckPins = 6;

// Game constants
#define GROUND_Y 54
#define DINO_X 10
#define DINO_WIDTH 8
#define DINO_HEIGHT 12
#define OBSTACLE_WIDTH 6
#define OBSTACLE_HEIGHT 12
#define CACTUS_HEIGHT 10

// Game state
int dinoY = GROUND_Y - DINO_HEIGHT;
int dinoVelocity = 0;
bool isJumping = false;
bool isDucking = false;

int obstacleX = SCREEN_WIDTH;
int obstacleType = 0; // 0 = cactus, 1 = bird

unsigned long score = 0;
unsigned long lastScoreUpdate = 0;
int gameSpeed = 3;
bool gameOver = false;

// Touch thresholds - LOWER values = more sensitive
int touchThreshold = 20;  // Decreased from 40 to reduce false positives
int analogThreshold = 2000;  // For analog pins

// Debounce
unsigned long lastJumpTime = 0;
unsigned long jumpDebounceDelay = 300;  // 300ms between jumps

void setup() {
  Serial.begin(115200);

  // Initialize I2C and display
  Wire.begin(OLED_SDA, OLED_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println(F("DINO RUNNER"));
  display.setCursor(10, 35);
  display.println(F("Touch to Jump!"));
  display.display();

  delay(2000);

  Serial.println("Dino Runner Started!");
  Serial.println("Jump pins: 22, 23, 19, 18, 5, 17, 21");
  Serial.println("Duck pins: 36, 37, 38, 39, 34, 35");
}

bool checkJumpTouch() {
  // Check all jump pins
  for (int i = 0; i < numJumpPins; i++) {
    int touchValue = touchRead(jumpPins[i]);
    if (touchValue < touchThreshold) {
      return true;
    }
  }
  return false;
}

bool checkDuckTouch() {
  // Check all duck pins - these are analog pins, use analogRead
  for (int i = 0; i < numDuckPins; i++) {
    int analogValue = analogRead(duckPins[i]);
    // For analog pins, detect touch when value changes significantly from baseline (~4095 when not touched)
    if (analogValue < analogThreshold) {  // Touched when value drops
      return true;
    }
  }
  return false;
}

void drawDino() {
  if (isDucking) {
    // Draw ducking dino (smaller)
    display.fillRect(DINO_X, GROUND_Y - 6, 10, 6, SSD1306_WHITE);
    display.drawPixel(DINO_X + 2, GROUND_Y - 7, SSD1306_WHITE);
    display.drawPixel(DINO_X + 3, GROUND_Y - 7, SSD1306_WHITE);
  } else {
    // Draw standing/jumping dino
    display.fillRect(DINO_X, dinoY, DINO_WIDTH, DINO_HEIGHT, SSD1306_WHITE);

    // Eye
    display.drawPixel(DINO_X + 6, dinoY + 2, SSD1306_BLACK);

    // Legs (animated)
    int legOffset = (millis() / 100) % 2;
    display.drawLine(DINO_X + 2, dinoY + DINO_HEIGHT, DINO_X + 2, dinoY + DINO_HEIGHT + 2 + legOffset, SSD1306_WHITE);
    display.drawLine(DINO_X + 6, dinoY + DINO_HEIGHT, DINO_X + 6, dinoY + DINO_HEIGHT + 2 - legOffset, SSD1306_WHITE);
  }
}

void drawObstacle() {
  if (obstacleType == 0) {
    // Cactus
    display.fillRect(obstacleX, GROUND_Y - CACTUS_HEIGHT, OBSTACLE_WIDTH, CACTUS_HEIGHT, SSD1306_WHITE);
    display.drawLine(obstacleX + 2, GROUND_Y - CACTUS_HEIGHT + 3, obstacleX + 2, GROUND_Y - CACTUS_HEIGHT, SSD1306_WHITE);
    display.drawLine(obstacleX + OBSTACLE_WIDTH - 2, GROUND_Y - CACTUS_HEIGHT + 3, obstacleX + OBSTACLE_WIDTH - 2, GROUND_Y - CACTUS_HEIGHT, SSD1306_WHITE);
  } else {
    // Flying bird
    int birdY = GROUND_Y - 20;
    display.fillRect(obstacleX, birdY, 8, 4, SSD1306_WHITE);
    // Wings (animated)
    int wingPos = (millis() / 100) % 2;
    if (wingPos == 0) {
      display.drawLine(obstacleX + 1, birdY - 1, obstacleX + 3, birdY - 2, SSD1306_WHITE);
      display.drawLine(obstacleX + 5, birdY - 1, obstacleX + 7, birdY - 2, SSD1306_WHITE);
    } else {
      display.drawLine(obstacleX + 1, birdY + 5, obstacleX + 3, birdY + 6, SSD1306_WHITE);
      display.drawLine(obstacleX + 5, birdY + 5, obstacleX + 7, birdY + 6, SSD1306_WHITE);
    }
  }
}

void drawGround() {
  display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);

  // Scrolling ground pattern
  int groundScroll = (millis() / 50) % 8;
  for (int i = 0; i < SCREEN_WIDTH; i += 8) {
    int x = i - groundScroll;
    if (x >= 0 && x < SCREEN_WIDTH) {
      display.drawPixel(x, GROUND_Y + 1, SSD1306_WHITE);
    }
  }
}

void drawScore() {
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH - 40, 2);
  display.print(score / 10);
}

bool checkCollision() {
  int dinoBottom = isDucking ? GROUND_Y : dinoY + DINO_HEIGHT;
  int dinoTop = isDucking ? GROUND_Y - 6 : dinoY;
  int dinoRight = DINO_X + (isDucking ? 10 : DINO_WIDTH);

  int obstacleBottom, obstacleTop;
  if (obstacleType == 0) {
    obstacleTop = GROUND_Y - CACTUS_HEIGHT;
    obstacleBottom = GROUND_Y;
  } else {
    obstacleTop = GROUND_Y - 24;
    obstacleBottom = GROUND_Y - 20;
  }

  int obstacleRight = obstacleX + OBSTACLE_WIDTH;

  // Check overlap
  if (dinoRight > obstacleX && DINO_X < obstacleRight) {
    if (dinoBottom > obstacleTop && dinoTop < obstacleBottom) {
      return true;
    }
  }

  return false;
}

void resetGame() {
  dinoY = GROUND_Y - DINO_HEIGHT;
  dinoVelocity = 0;
  isJumping = false;
  isDucking = false;
  obstacleX = SCREEN_WIDTH;
  obstacleType = random(0, 2);
  score = 0;
  gameSpeed = 3;
  gameOver = false;
}

void showGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 15);
  display.println(F("GAME"));
  display.setCursor(15, 35);
  display.println(F("OVER!"));
  display.setTextSize(1);
  display.setCursor(20, 55);
  display.print(F("Score: "));
  display.print(score / 10);
  display.display();
}

void loop() {
  if (gameOver) {
    showGameOver();

    // Wait for any touch to restart
    if (checkJumpTouch() || checkDuckTouch()) {
      delay(200);
      resetGame();
    }
    return;
  }

  // Check touch sensors
  bool jumpTouched = checkJumpTouch();
  bool duckTouched = checkDuckTouch();

  // Handle jumping with debounce
  if (jumpTouched && !isJumping && !isDucking) {
    unsigned long currentTime = millis();
    if (currentTime - lastJumpTime > jumpDebounceDelay) {
      isJumping = true;
      dinoVelocity = -8;
      lastJumpTime = currentTime;
      Serial.println("JUMP!");
    }
  }

  // Handle ducking
  isDucking = duckTouched && !isJumping;

  // Update dino position
  if (isJumping) {
    dinoY += dinoVelocity;
    dinoVelocity += 1; // Gravity

    // Land on ground
    if (dinoY >= GROUND_Y - DINO_HEIGHT) {
      dinoY = GROUND_Y - DINO_HEIGHT;
      isJumping = false;
      dinoVelocity = 0;
    }
  }

  // Move obstacle
  obstacleX -= gameSpeed;

  // Respawn obstacle
  if (obstacleX < -OBSTACLE_WIDTH) {
    obstacleX = SCREEN_WIDTH;
    obstacleType = random(0, 2);
    score += 10;

    // Increase difficulty
    if (score % 100 == 0 && gameSpeed < 8) {
      gameSpeed++;
    }
  }

  // Update score
  if (millis() - lastScoreUpdate > 100) {
    score++;
    lastScoreUpdate = millis();
  }

  // Check collision
  if (checkCollision()) {
    gameOver = true;
    Serial.print("Game Over! Score: ");
    Serial.println(score / 10);
    return;
  }

  // Draw everything
  display.clearDisplay();
  drawGround();
  drawDino();
  drawObstacle();
  drawScore();
  display.display();

  delay(30);
}
