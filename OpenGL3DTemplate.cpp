#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <glut.h>
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <cmath>
#include <sstream>

#include <vector>


#define GLUT_KEY_ESCAPE 27
#define DEG2RAD(a) (a * 0.0174532925)
#define M_PI 3.14159265358979323846


void PlaySoundEffect(const char* filename);
void PlayCollisionSound(const char* filename);
void printPlayerPosition();


class Vector3f {
public:
	float x, y, z;
	Vector3f(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) : x(_x), y(_y), z(_z) {}

	Vector3f operator+(Vector3f v) { return Vector3f(x + v.x, y + v.y, z + v.z); }
	Vector3f operator-(Vector3f v) { return Vector3f(x - v.x, y - v.y, z - v.z); }
	Vector3f operator*(float n) { return Vector3f(x * n, y * n, z * n); }
	Vector3f operator/(float n) { return Vector3f(x / n, y / n, z / n); }
	Vector3f unit() { return *this / sqrt(x * x + y * y + z * z); }
	Vector3f cross(Vector3f v) { return Vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
};

Vector3f ballPosition(2.0f, 1.1f, 1.0f);  // Ball position
float ballBounceHeight = 1.0f;  // Maximum height of the bounce
float ballBounceSpeed = 4.0f;   // Speed of the bounce
float elapsedTime = 0.0f;       // Elapsed time for controlling bounce
Vector3f ballPosition2(-2.0f, 1.1f, 1.0f);  // Ball position

float courtMinX = -5.8f; // Left boundary
float courtMaxX = 5.8f;  // Right boundary
float courtMinZ = -13.3f; // Front boundary
float courtMaxZ = 13.3f;  // Back boundary

bool isForehand = false;  // Whether forehand motion is happening
float forehandAngle = 0.0f;  // The angle of the forehand motion
float forehandSpeed = 2.0f;  // Speed at which the forehand swing occurs

int score = 0;
bool ballHit = false;
bool ballHit2 = false;
float ballRadius = 0.5f;
bool gameWin = false;
bool gameLost = false;
int timer = 3000;

int playerHealth = 100;


enum GameState { PLAYING, GAME_WON, GAME_LOST };
GameState currentGameState = PLAYING;


class Camera {
public:
	Vector3f eye, center, up;

	Camera(float eyeX = 4.0f, float eyeY = 13.0f, float eyeZ = 25.0f, float centerX = 0.0f, float centerY = 0.0f, float centerZ = 0.0f, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f) {
		eye = Vector3f(eyeX, eyeY, eyeZ);
		center = Vector3f(centerX, centerY, centerZ);
		up = Vector3f(upX, upY, upZ);
	}

	void moveX(float d) {
		Vector3f right = up.cross(center - eye).unit();
		eye = eye + right * d;
		center = center + right * d;
	}

	void setView(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ) {
		eye = Vector3f(eyeX, eyeY, eyeZ);
		center = Vector3f(centerX, centerY, centerZ);
	}

	void moveY(float d) {
		eye = eye + up.unit() * d;
		center = center + up.unit() * d;
	}

	void moveZ(float d) {
		Vector3f view = (center - eye).unit();
		eye = eye + view * d;
		center = center + view * d;
	}

	void rotateX(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + up * sin(DEG2RAD(a));
		up = view.cross(right);
		center = eye + view;
	}

	void rotateY(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + right * sin(DEG2RAD(a));
		right = view.cross(up);
		center = eye + view;
	}

	void look() {
		gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z
		);
	}
};
Camera camera;


class Coin {
public:
	float x, y, z;
	float radius;
	bool collected;

	Coin(float _x, float _y, float _z, float _radius)
		: x(_x), y(_y), z(_z), radius(_radius), collected(false) {}

	void draw(float time) {
		if (collected) return; // Skip drawing if the coin is collected

		// Calculate the vertical offset using a sine wave for smooth up and down movement
		float verticalOffset = 0.2f * sin(time);

		glPushMatrix();
		glTranslatef(x, y + verticalOffset, z); // Apply the vertical offset

		// Draw the coin (a simple cylinder for now)
		glColor3f(1.0f, 0.84f, 0.0f); // Gold color
		GLUquadric* quad = gluNewQuadric();
		gluDisk(quad, 0.0f, radius, 20, 1); // Top face
		glTranslatef(0.0f, 0.1f, 0.0f); // Move up slightly to draw the side
		gluCylinder(quad, radius, radius, 0.1f, 20, 1); // Side face
		glTranslatef(0.0f, 0.1f, 0.0f); // Move up slightly to draw the bottom face
		gluDisk(quad, 0.0f, radius, 20, 1); // Bottom face
		gluDeleteQuadric(quad);

		glPopMatrix();
	}

	void checkCollision(float playerX, float playerZ) {
		if (collected) return; // Skip if the coin is already collected

		// Calculate the distance between the player and the coin
		float distance = sqrt(pow(playerX - x, 2) + pow(playerZ - z, 2));

		// Check if the distance is less than the sum of the radii (collision detection)
		if (distance < radius + 0.5f) { // Assuming player radius is 0.5f
			collected = true; // Mark the coin as collected
			score += 100; // Increase score by 100
			std::cout << "Coin collected! Score: " << score << std::endl;

			// Optionally, you can play a sound effect
			// PlaySoundEffect("coin_collect.wav");
		}
	}
};
Coin coin1(3.8f, 1.0f, 9.2f, 0.3f);
Coin coin2(-1.2f, 1.0f, -0.1f, 0.3f);
Coin coin3(-1.7f, 1.0f, -7.7f, 0.3f);
Coin coin4(4.7f, 1.0f, -6.3f, 0.3f);
Coin coin5(7.6f, 1.0f, -1.1f, 0.3f);
Coin coin6(10.3f, 1.0f, -4.4f, 0.3f);
Coin coin7(10.3f, 1.0f, -9.0f, 0.3f);
Coin coin8(18.6f, 1.0f, -6.8f, 0.3f);
Coin coin9(16.5f, 1.0f, 0.9f, 0.3f);
Coin coin10(15.3f, 1.0f, 9.1f, 0.3f);
Coin coin11(19.7f, 1.0f, 9.1f, 0.3f);
Coin coin12(3.7f, 1.0f, -12.3f, 0.3f);
Coin coin13(-1.0f, 1.0f, -17.8f, 0.3f);
Coin coin14(-3.8f, 1.0f, -21.5f, 0.3f);
Coin coin15(5.1f, 1.0f, -21.5f, 0.3f);
Coin coin16(3.9f, 1.0f, -31.4f, 0.3f);
Coin coin17(-4.6f, 1.0f, 31.6f, 0.3f);

class Trap {
public:
	float x, y, z;
	float width, depth;
	float spikeHeight;
	float moveSpeed;
	bool movingUp;
	std::vector<Vector3f> spikes;

	Trap(float _x, float _y, float _z, float _width, float _depth, float _spikeHeight, float _moveSpeed)
		: x(_x), y(_y), z(_z), width(_width), depth(_depth), spikeHeight(_spikeHeight), moveSpeed(_moveSpeed), movingUp(true) {
		generateSpikes();
	}

	void generateSpikes() {
		int numSpikesX = static_cast<int>(width * 2);
		int numSpikesZ = static_cast<int>(depth * 2);
		for (int i = 0; i < numSpikesX; ++i) {
			for (int j = 0; j < numSpikesZ; ++j) {
				float spikeX = x - width / 2 + i * (width / numSpikesX);
				float spikeZ = z - depth / 2 + j * (depth / numSpikesZ);
				spikes.push_back(Vector3f(spikeX, y, spikeZ));
			}
		}
	}

	void draw(float time) {
		// Update spike movement using a sine wave for smooth and constant speed
		float amplitude = 1.0f; // Maximum height of the spikes
		float frequency = 1.0f; // Frequency of the movement
		y = amplitude * sin(frequency * time);

		// Draw spikes
		glColor3f(0.5f, 0.5f, 0.5f); // Gray color for spikes
		for (const auto& spike : spikes) {
			glPushMatrix();
			glTranslatef(spike.x, y, spike.z);
			glRotatef(-90, 1.0f, 0.0f, 0.0f);
			glutSolidCone(0.1f, spikeHeight, 10, 2);
			glPopMatrix();
		}
	}

	void checkCollision(float playerX, float playerZ) {
		// Check if player is within the trap's area
		if (playerX >= x - width / 2 && playerX <= x + width / 2 &&
			playerZ >= z - depth / 2 && playerZ <= z + depth / 2) {
			// Check if player is touching the spikes
			if (y >= 0.0f && y <= 1.0f) {
				playerHealth -= 1; // Decrease player health
				std::cout << "Player hit by trap! Health: " << playerHealth << std::endl;
				if (playerHealth <= 0) {
					std::cout << "Player is dead!" << std::endl;
					gameLost = true;
					// Handle player death (e.g., end game, respawn, etc.)
				}
			}
		}
	}
};
Trap trap1(-5.6f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap2(-3.6f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap3(-1.6f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap4(0.4f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap5(2.4f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap6(4.4f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap7(5.6f, 1.0f, 3.0f, 2.0f, 2.0f, 0.6f, 0.01f);

Trap trap8(-5.6f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap9(-3.6f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap10(-1.6f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap11(0.4f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap12(2.4f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap13(4.4f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);
Trap trap14(5.6f, 1.0f, -10.0f, 2.0f, 2.0f, 0.6f, 0.01f);

float playerX = -3.0f;  // Player's initial X position
float playerZ = 10.0f;  // Player's initial Z position
float playerAngle = 0.0f;  // Player's facing direction in degrees
float legAngle = 0.0f;  // Angle for leg movement (for walking effect)
bool legMovingForward = true;  // Direction of leg movement (for walking)

// Draw the player at its current position and orientation
void drawPlayer() {
	//glDisable(GL_LIGHTING);

	glPushMatrix();
	glTranslatef(playerX, -0.3f, playerZ); // Player's position
	glRotatef(playerAngle, 0.0f, 1.0f, 0.0f); // Rotate to face movement direction

	// Head
	glColor3f(1.0f, 0.8f, 0.6f); // Skin tone
	glPushMatrix();
	glTranslatef(0.0f, 1.8f, 0.0f);
	glutSolidSphere(0.2f, 20, 20); // Head
	glPopMatrix();

	// Eyes (two small spheres)
	glColor3f(0.0f, 0.0f, 0.0f); // Black color for the eyes
	// Right Eye
	glPushMatrix();
	glTranslatef(0.1f, 1.9f, -0.16f); // Position on the right side of the face
	glutSolidSphere(0.05f, 10, 10); // Right eye
	glPopMatrix();
	// Left Eye
	glPushMatrix();
	glTranslatef(-0.1f, 1.9f, -0.16f); // Position on the left side of the face
	glutSolidSphere(0.05f, 10, 10); // Left eye
	glPopMatrix();

	// Nose (small cone)
	glColor3f(1.0f, 0.8f, 0.6f); // Skin tone
	glPushMatrix();
	glTranslatef(0.0f, 1.7f, 0.2f); // Nose position
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Rotate cone to point outwards
	glutSolidCone(0.05f, 0.1f, 10, 10); // Nose
	glPopMatrix();

	// Mouth (simple box to approximate a mouth)
	glColor3f(0.8f, 0.0f, 0.0f); // Red color for the mouth
	glPushMatrix();
	glTranslatef(0.0f, 1.8f, -0.19f); // Position the mouth below the nose
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Rotate to horizontal
	glScalef(1.2f, 1.05f, 1.05f); // Scale to make a wide mouth shape
	glutSolidCube(0.05f); // Mouth as a small box (cube)
	glPopMatrix();

	// Body
	glColor3f(0.0f, 0.5f, 1.0f); // Blue shirt
	glPushMatrix();
	glTranslatef(0.0f, 1.2f, 0.0f);
	glScalef(0.3f, 0.6f, 0.2f);
	glutSolidCube(1.0f); // Torso
	glPopMatrix();

	// Arms (fixed in position)
	glColor3f(1.0f, 0.8f, 0.6f); // Skin tone

	// Right Arm (Forehand motion)
	glPushMatrix();
	glTranslatef(0.25f, 1.3f, 0.0f);  // Position of right arm

	glRotatef(30, 0, 0, 1);  // Rotate slightly for forehand motion

	if (isForehand) {
		// Apply forehand motion (rotation over time)
		glRotatef(forehandAngle, 1.0f, 0.0f, 0.0f);
		if (forehandAngle > -60.0f) {
			forehandAngle -= forehandSpeed;
		}
		else {
			isForehand = false;  // Stop after swing completes
			forehandAngle = 0.0f; // Reset angle for next swing
		}
	}
	glScalef(0.1f, 0.5f, 0.1f); // Arm size
	glutSolidCube(1.0f); // Right arm
	glPopMatrix();  // End arm transformation

	// Left Arm (no forehand motion)
	glPushMatrix();
	glTranslatef(-0.25f, 1.3f, 0.0f);
	glRotatef(-30, 0, 0, 1);
	glScalef(0.1f, 0.5f, 0.1f);
	glutSolidCube(1.0f); // Left arm
	glPopMatrix();

	// Right Leg with walking motion
	glColor3f(0.0f, 0.5f, 1.0f); // Shorts color
	glPushMatrix();
	glTranslatef(0.1f, 0.6f, 0.0f);
	glRotatef(legAngle, 1, 0, 0); // Forward-backward leg swing
	glScalef(0.15f, 0.5f, 0.15f);
	glutSolidCube(1.0f); // Right leg
	glPopMatrix();

	// Left Leg with walking motion
	glPushMatrix();
	glTranslatef(-0.1f, 0.6f, 0.0f);
	glRotatef(-legAngle, 1, 0, 0); // Forward-backward leg swing opposite to right leg
	glScalef(0.15f, 0.5f, 0.15f);
	glutSolidCube(1.0f); // Left leg
	glPopMatrix();

	// Tennis racket in right hand (move with right arm)
	glColor3f(0.4f, 0.2f, 0.0f); // Brown handle
	glPushMatrix();
	glTranslatef(0.35f, 1.2f, 0.0f);  // Position racket at the hand
	glRotatef(-30, 0, 0, 1);  // Tilt racket
	glRotatef(-20, 0, 1, 0);  // Additional rotation

	// Apply forehand rotation to the racket, just like the arm
	if (isForehand) {
		glRotatef(forehandAngle, 1.0f, 0.0f, 0.0f); // Rotate racket with arm
	}

	// Handle of the racket
	glPushMatrix();
	glScalef(0.05f, 0.4f, 0.05f);
	glutSolidCube(1.0f);
	glPopMatrix();

	// Racket head
	glColor3f(0.9f, 0.9f, 0.9f); // Light gray
	glTranslatef(0.0f, 0.3f, 0.0f);
	glutSolidTorus(0.02f, 0.15f, 10, 10); // Racket head (with forehand rotation)
	glPopMatrix();  // End racket transformation

	glPopMatrix();  // End player transformation
	//glEnable(GL_LIGHTING);  // Restore lighting
}

void renderText(float x, float y, const char* text) {
	glDisable(GL_LIGHTING);
	glColor3f(1.0f, 1.0f, 1.0f); // White color for text
	glRasterPos2f(x, y); // Position of the text on the screen
	for (int i = 0; text[i] != '\0'; i++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]); // Render each character
	}
	glEnable(GL_LIGHTING);
}

void renderTextTimer(float x, float y, const char* text, void* font) {
	glRasterPos2f(x, y);  // Set the position for the text
	for (const char* c = text; *c != '\0'; c++) {
		glutBitmapCharacter(font, *c);  // Render each character
	}
}

void displayTimer() {
	glColor3f(1.0f, 1.0f, 1.0f);  // Set text color (white)
	char timerText[16];
	sprintf(timerText, "Time: %d", timer);  // Convert timer value to string
	renderTextTimer(-2.95f, 0.9f, timerText, GLUT_BITMAP_HELVETICA_18);  // Position and render
}

void timerCallback(int value) {
	if (timer > 0) {
		timer--;  // Decrease timer by 1 second
		glutTimerFunc(1000, timerCallback, 0);  // Call this function again in 1 second
	}
	else {
		gameLost = true;  // Set gameLost to true when timer reaches 0
	}

	glutPostRedisplay();  // Redraw the screen to update the timer display
}


void drawWall(double width, double height, double thickness) {
	glPushMatrix();
	glScaled(width, height, thickness);
	glutSolidCube(1.0);
	glPopMatrix();

}

float toRadians(float degrees) {
	return degrees * (3.14159f / 180.0f);
}

void drawCourt() {

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(0.0f, -0.15f, 0.0f);  // Move cube slightly below the court surface
	glColor3f(0.54f, 0.27f, 0.07f);   // Brownish color for the ground
	glScaled(12.0f, 0.2f, 26.8f);     // Scale the cube to match the court size and add thickness
	glutSolidCube(1.0f);  // Draw the ground cube
	glPopMatrix();


}

void drawRoom() {

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(15.0f, -0.15f, 0.0f);  // Move cube slightly below the court surface
	glColor3f(0.54f, 0.27f, 0.07f);   // Brownish color for the ground
	glScaled(12.0f, 0.2f, 20.8f);     // Scale the cube to match the court size and add thickness
	glutSolidCube(1.0f);  // Draw the ground cube
	glPopMatrix();

}

void drawRoom2() {


	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(0.0f, -0.15f, -30.0f);  // Move cube slightly below the court surface
	glColor3f(0.54f, 0.27f, 0.07f);   // Brownish color for the ground
	glScaled(12.0f, 0.2f, 20.8f);     // Scale the cube to match the court size and add thickness
	glutSolidCube(1.0f);  // Draw the ground cube
	glPopMatrix();

}

void drawCoridor1() {

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(7.5f, -0.15f, 0.0f);  // Move cube slightly below the court surface
	glColor3f(0.54f, 0.27f, 0.07f);   // Brownish color for the ground
	glScaled(3.0f, 0.2f, 3.8f);     // Scale the cube to match the court size and add thickness
	glutSolidCube(1.0f);  // Draw the ground cube
	glPopMatrix();


}

void drawCoridor2() {

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(0.0f, -0.15f, -14.0f);  // Move cube slightly below the court surface
	glColor3f(0.54f, 0.27f, 0.07f);   // Brownish color for the ground
	glScaled(3.0f, 0.2f, 11.2f);     // Scale the cube to match the court size and add thickness
	glutSolidCube(1.0f);  // Draw the ground cube
	glPopMatrix();

}

void drawEnv1() {

	// Draw boundary walls around the court
	glColor3f(1.0f, 0.0f, 0.0f);

	//----------------------------

	// Back wall
	glPushMatrix();
	glTranslatef(-3.7f, 1.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(4.5, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.7f, 1.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(4.5, 2.0, 0.2);
	glPopMatrix();

	// Left wall
	glPushMatrix();
	glTranslatef(-6.0f, 1.0f, 0.0f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(27.0, 2.0, 0.2);
	glPopMatrix();

	// Right wall
	glPushMatrix();
	glTranslatef(6.0f, 1.0f, -7.5f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(11.5, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(6.0f, 1.0f, 7.5f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(11.5, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 1.0f, 13.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 2.0, 0.2);
	glPopMatrix();

	//------------------------

	glPushMatrix();
	glTranslatef(21.0f, 1.0f, 0.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(21.0, 2.0, 0.2);
	glPopMatrix();

	// Back wall
	glPushMatrix();
	glTranslatef(15.0f, 1.0f, -10.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(15.0f, 1.0f, 10.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 2.0, 0.2);
	glPopMatrix();

	//--------------------
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -27.0f);
	glPushMatrix();
	glTranslatef(0.0f, 1.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 2.0, 0.2);
	glPopMatrix();

	// Left wall
	glPushMatrix();
	glTranslatef(-6.0f, 1.0f, -3.0f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(21.0, 2.0, 0.2);
	glPopMatrix();

	// Right wall
	glPushMatrix();
	glTranslatef(6.0f, 1.0f, -3.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(21.0, 2.0, 0.2);
	glPopMatrix();
	glPopMatrix();

	//----------------------

	glPushMatrix();
	glTranslatef(-3.8f, 1.0f, -19.6f);  // Adjusted to match the court boundary
	drawWall(4.5, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.8f, 1.0f, -19.6f);  // Adjusted to match the court boundary
	drawWall(4.5, 2.0, 0.2);
	glPopMatrix();

	//_-------------------------

	glPushMatrix();
	glTranslatef(9.0f, 1.0f, -6.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(8.6, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(9.0f, 1.0f, 6.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(8.6, 2.0, 0.2);
	glPopMatrix();

	//------------------------------

	glPushMatrix();
	glTranslatef(7.6f, 1.0f, 1.8f);  // Adjusted to match the court boundary
	drawWall(3.0, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(7.6f, 1.0f, -1.8f);  // Adjusted to match the court boundary
	drawWall(3.0, 2.0, 0.2);
	glPopMatrix();

	//-------------------------------------------

	glPushMatrix();
	glTranslatef(-1.5f, 1.0f, -16.5f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(6.0, 2.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1.5f, 1.0f, -16.5f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(6.0, 2.0, 0.2);
	glPopMatrix();


}

void setupLights() {
	GLfloat ambient[] = { 0.7f, 0.7f, 0.7, 1.0f };
	GLfloat diffuse[] = { 0.6f, 0.6f, 0.6, 1.0f };
	GLfloat specular[] = { 1.0f, 1.0f, 1.0, 1.0f };
	GLfloat shininess[] = { 50 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

	GLfloat lightIntensity[] = { 0.7f, 0.7f, 1, 1.0f };
	GLfloat lightPosition[] = { -7.0f, 6.0f, 3.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightIntensity);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightIntensity);
}

void setupCamera() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, 1.0, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera.look();
}

void checkGameOver() {
	if (gameLost) {
		if (ballHit && ballHit2) {

			currentGameState = GAME_WON;
			glutPostRedisplay();  // Request a redraw to show the new scene
		}
		else {

			currentGameState = GAME_LOST;
			glutPostRedisplay();  // Request a redraw to show the new scene

		}

	}

}

/*void checkGameWin() {
	if (ballHit && ballHit2) {
		currentGameState = GAME_WON;
		glutPostRedisplay();  // Request a redraw to show the new scene
	}
}*/

GLfloat wallColor[3] = { 1.0f, 0.0f, 0.0f };  // Initial wall color (red)
void updateWallColor(int value) {
	// Generate random RGB values for the wall color
	wallColor[0] = static_cast<float>(rand() % 100) / 100.0f;  // Red component
	wallColor[1] = static_cast<float>(rand() % 100) / 100.0f;  // Green component
	wallColor[2] = static_cast<float>(rand() % 100) / 100.0f;  // Blue component

	// Request to redisplay the scene
	glutPostRedisplay();

	// Set the timer to call this function again after 2000 ms
	glutTimerFunc(2000, updateWallColor, 0);
}

bool followPlayer = false; // Flag to indicate if the camera should follow the player

void updateCameraPosition() {
	if (followPlayer) {
		// Set the camera's position to the player's head position
		camera.setView(playerX, 1.8f, playerZ, playerX - sin(DEG2RAD(playerAngle)), 1.8f, playerZ - cos(DEG2RAD(playerAngle)));
	}
}

bool followPlayer2 = false; // Flag to indicate if the camera should follow the player

void updateCameraPosition2() {
	if (followPlayer2) {
		// Set the camera's position to the player's head position
		camera.setView(playerX, 3.8f, playerZ+3, playerX - sin(DEG2RAD(playerAngle)), 1.8f, playerZ - cos(DEG2RAD(playerAngle)));
	}
}

void resetCoins() {
	coin1.collected = false;
	coin2.collected = false;
	coin3.collected = false;
	coin4.collected = false;
	coin5.collected = false;
	coin6.collected = false;
	coin7.collected = false;
	coin8.collected = false;
	coin9.collected = false;
	coin10.collected = false;
	coin11.collected = false;
	coin12.collected = false;
	coin13.collected = false;
	coin14.collected = false;
	coin15.collected = false;
	coin16.collected = false;
	coin17.collected = false;
}

/*void Display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (currentGameState) {
	case PLAYING:
		setupCamera();
		setupLights();

		//PlaySoundEffect("../../../Downloads/Wii Music - Gaming Background Music (HD).wav");

		// Draw game elements
		drawCourt();




		drawPlayer();

		// Display score
		char scoreText[20];
		sprintf(scoreText, "Score: %d", score);  // Format score as string
		renderText(0.8f, 0.9f, scoreText);  // Adjust the position of the score

		// Display timer
		displayTimer();

		// Draw boundary walls around the court
		glColor3f(1.0f, 0.0f, 0.0f);

		// Back wall
		glPushMatrix();
		glColor3fv(wallColor);  // Use the global wall color
		glTranslatef(0.0f, 1.0f, -13.4f);  // Adjusted to match the court boundary
		drawWall(12.0, 2.0, 0.1);
		glPopMatrix();

		// Left wall
		glPushMatrix();
		glColor3fv(wallColor);  // Use the global wall color
		glTranslatef(-6.0f, 1.0f, 0.0f);   // Adjusted to match the court boundary
		glRotatef(90, 0, 1, 0);
		drawWall(27.0, 2.0, 0.1);
		glPopMatrix();

		// Right wall
		glPushMatrix();
		glColor3fv(wallColor);  // Use the global wall color
		glTranslatef(6.0f, 1.0f, 0.0f);    // Adjusted to match the court boundary
		glRotatef(90, 0, 1, 0);
		drawWall(27.0, 2.0, 0.1);
		glPopMatrix();


		// Check game status
		checkGameOver();
		//checkGameWin();
		break;

	case GAME_LOST:
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background
		glColor3f(1.0f, 0.0f, 0.0f);           // Red text
		renderTextTimer(-0.4f, 0.0f, "YOU LOST!", GLUT_BITMAP_HELVETICA_18);
		break;

	case GAME_WON:
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background
		glColor3f(0.0f, 1.0f, 0.0f);           // Green text
		renderTextTimer(-0.6f, 0.0f, "CONGRATULATIONS, YOU WON!", GLUT_BITMAP_HELVETICA_18);
		break;
	}

	glFlush();
}*/

// Function to convert an integer to a string
std::string intToString(int value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

void renderText(const std::string& text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
	int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	gluOrtho2D(0, windowWidth, 0, windowHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
	glRasterPos2f(x, y);
	for (char c : text) {
		glutBitmapCharacter(font, c);
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}


void Display() {
	int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
	int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	float timeTrap = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

	switch (currentGameState) {
	case PLAYING:
		setupCamera();
		setupLights();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw tennis court floor and lines
		drawCourt();
		drawRoom();
		drawRoom2();
		drawCoridor1();
		drawCoridor2();
		drawPlayer();
		drawEnv1();

		coin1.draw(time);
		coin2.draw(time);
		coin3.draw(time);
		coin4.draw(time);
		coin5.draw(time);
		coin6.draw(time);
		coin7.draw(time);
		coin8.draw(time);
		coin9.draw(time);
		coin10.draw(time);
		coin11.draw(time);
		coin12.draw(time);
		coin13.draw(time);
		coin14.draw(time);
		coin15.draw(time);
		coin16.draw(time);
		coin17.draw(time);
		coin1.checkCollision(playerX, playerZ);
		coin2.checkCollision(playerX, playerZ);
		coin3.checkCollision(playerX, playerZ);
		coin4.checkCollision(playerX, playerZ);
		coin5.checkCollision(playerX, playerZ);
		coin6.checkCollision(playerX, playerZ);
		coin7.checkCollision(playerX, playerZ);
		coin8.checkCollision(playerX, playerZ);
		coin9.checkCollision(playerX, playerZ);
		coin10.checkCollision(playerX, playerZ);
		coin11.checkCollision(playerX, playerZ);
		coin12.checkCollision(playerX, playerZ);
		coin13.checkCollision(playerX, playerZ);
		coin14.checkCollision(playerX, playerZ);
		coin15.checkCollision(playerX, playerZ);
		coin16.checkCollision(playerX, playerZ);
		coin17.checkCollision(playerX, playerZ);

		trap1.draw(timeTrap);
		trap2.draw(timeTrap);
		trap3.draw(timeTrap);
		trap4.draw(timeTrap);
		trap5.draw(timeTrap);
		trap6.draw(timeTrap);
		trap7.draw(timeTrap);
		trap8.draw(timeTrap);
		trap9.draw(timeTrap);
		trap10.draw(timeTrap);
		trap11.draw(timeTrap);
		trap12.draw(timeTrap);
		trap13.draw(timeTrap);
		trap14.draw(timeTrap);
		trap1.checkCollision(playerX, playerZ);
		trap2.checkCollision(playerX, playerZ);
		trap3.checkCollision(playerX, playerZ);
		trap4.checkCollision(playerX, playerZ);
		trap5.checkCollision(playerX, playerZ);
		trap6.checkCollision(playerX, playerZ);
		trap7.checkCollision(playerX, playerZ);
		trap8.checkCollision(playerX, playerZ);
		trap9.checkCollision(playerX, playerZ);
		trap10.checkCollision(playerX, playerZ);
		trap11.checkCollision(playerX, playerZ);
		trap12.checkCollision(playerX, playerZ);
		trap13.checkCollision(playerX, playerZ);
		trap14.checkCollision(playerX, playerZ);
		

		char scoreText[20];
		sprintf(scoreText, "Score: %d", score);  // Format score as string
		renderText(0.8f, 0.9f, scoreText);  // Adjust the position of the score
		displayTimer();

		
		renderText("Score: " + intToString(score), windowWidth - 100, windowHeight - 20);
		renderText("Health: " + intToString(playerHealth), windowWidth - 250, windowHeight - 20);
		if (gameLost) {
			currentGameState = GAME_LOST;
		}
		break;


	case GAME_LOST:

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0f, 0.0f, 0.0f);
		renderText("Game Over", windowWidth / 2 - 50, windowHeight / 2);
		renderText("Score: " + std::to_string(score), windowWidth / 2 - 50, windowHeight / 2 - 20);
		renderText("Click to Restart", windowWidth / 2 - 50, windowHeight / 2 - 40);
		break;
	}

	glFlush();
}

void mouseClick(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (currentGameState == GAME_LOST) {
			currentGameState = PLAYING;
			score = 0;
			playerHealth = 100;
			gameLost = false;
			resetCoins();
			playerX = -3.0f;
			playerZ = 10.0f;
		}
	}
}

void Keyboard(unsigned char key, int x, int y) {
	float d = 0.2;
	float stepSize = 0.1f;  // Movement step size
	float rotationAngle = 90.0f;  // Rotate 90 degrees to face the direction

	float newPlayerX = playerX;
	float newPlayerZ = playerZ;

	switch (key) {
	case 'w':
		camera.moveY(d);
		break;
	case 's':
		camera.moveY(-d);
		break;
	case 'a':
		camera.moveX(d);
		break;
	case 'd':
		camera.moveX(-d);
		break;
	case 'q':
		camera.moveZ(d);
		break;
	case 'e':
		camera.moveZ(-d);
		break;
	case '1':  // Front view
		camera.setView(0.0f, 2.5f, 18.0f, 0.0f, 0.0f, 0.0f);
		break;
	case '2':  // Top view
		camera.setView(0.0f, 40.0f, 0.1f, 0.0f, 0.0f, 0.0f);
		break;
	case '3':  // Side view
		camera.setView(30.0f, 14.5f, 0.0f, -9.0f, 0.0f, 0.0f);
		break;
	case '4':  // Side view
		camera.setView(4.0f, 13.0f, 25.0f, 0.0f, 0.0f, 0.0f);
		break;
	case 'u':  // Move forward and face forward (0 degrees)
		playerAngle = 0.0f;  // Face forward
		newPlayerZ -= stepSize;  // Move forward in the Z direction
		break;

	case 'j':  // Move backward and face backward (180 degrees)
		playerAngle = 180.0f;  // Face backward
		newPlayerZ += stepSize;  // Move backward in the Z direction
		break;

	case 'h':  // Move left and face left (90 degrees)
		playerAngle = 90.0f;  // Face left
		newPlayerX -= stepSize;  // Move left in the X direction
		break;

	case 'k':  // Move right and face right (-90 degrees)
		playerAngle = -90.0f;  // Face right
		newPlayerX += stepSize;  // Move right in the X direction
		break;
	case 'p':
		followPlayer = !followPlayer; // Toggle the followPlayer flag
		break;
	case 'l':
		followPlayer2 = !followPlayer2; // Toggle the followPlayer flag
		break;

	case GLUT_KEY_ESCAPE:
		exit(EXIT_SUCCESS);
	}
	if ((newPlayerX >= -5.6f && newPlayerX <= 5.6f && newPlayerZ >= -13.3f && newPlayerZ <= 13.3f) || // Main room
		(newPlayerX >= 5.6f && newPlayerX <= 9.2f && newPlayerZ >= -1.5f && newPlayerZ <= 1.5f) || // Corridor 1
		(newPlayerX >= 9.2f && newPlayerX <= 20.8f && newPlayerZ >= -10.2f && newPlayerZ <= 10.2f) || // Room 1
		(newPlayerX >= -1.3f && newPlayerX <= 1.3f && newPlayerZ >= -19.7f && newPlayerZ <= -13.2f) || // Corridor 2
		(newPlayerX >= -5.7f && newPlayerX <= 5.7f && newPlayerZ >= -40.3f && newPlayerZ <= -19.7f)) { // Room 2
		// If the new position is valid, update the player's position
		playerX = newPlayerX;
		playerZ = newPlayerZ;
		//PlayCollisionSound(".../../../Downloads/Bruh (Sound Effect) 1 second video!.wav");
		updateCameraPosition();
		updateCameraPosition2();

	}
	printPlayerPosition();

	if (legMovingForward) {
		legAngle += 5.0f;
		if (legAngle >= 20.0f) legMovingForward = false;
	}
	else {
		legAngle -= 5.0f;
		if (legAngle <= -20.0f) legMovingForward = true;
	}

	glutPostRedisplay();
}

void printPlayerPosition() {
	std::cout << "Player Position - X: " << playerX << ", Z: " << playerZ << std::endl;
}

void Special(int key, int x, int y) {
	float a = 1.7;

	switch (key) {
	case GLUT_KEY_UP:
		camera.rotateX(a);
		break;
	case GLUT_KEY_DOWN:
		camera.rotateX(-a);
		break;
	case GLUT_KEY_LEFT:
		camera.rotateY(a);
		break;
	case GLUT_KEY_RIGHT:
		camera.rotateY(-a);
		break;
	}

	glutPostRedisplay();
}



void Timer(int value) {
	// Update ball position
	glutPostRedisplay();      // Request a redraw
	glutTimerFunc(16, Timer, 0);  // Call Timer again after 16 ms (approx. 60 FPS)
}

void PlaySoundEffect(const char* filename) {
	PlaySoundA(filename, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NOSTOP);
}

void PlayCollisionSound(const char* filename) {
	// Play a collision sound asynchronously (no looping, only one play)
	PlaySoundA(filename, NULL, SND_FILENAME | SND_ASYNC);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Tennis Court");
	//PlaySoundEffect("../../../Downloads/Wii Music - Gaming Background Music (HD).wav");

	glutDisplayFunc(Display);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(Special);
	glutMouseFunc(mouseClick);

	glEnable(GL_DEPTH_TEST);
	//glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Sky blue background

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	// Start the timer
	glutTimerFunc(0, Timer, 0);
	glutTimerFunc(1000, timerCallback, 0);

	glutMainLoop();
	return 0;
}