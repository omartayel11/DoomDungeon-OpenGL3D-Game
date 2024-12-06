#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
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

GLuint tex;
void PlaySoundEffect(const char* filename);
void PlayCollisionSound(const char* filename);
void printPlayerPosition();
void updateJump();


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
	float length() { return sqrt(x * x + y * y + z * z); }
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

int score = 500;
bool ballHit = false;
bool ballHit2 = false;
float ballRadius = 0.5f;
bool gameWin = false;
bool gameLost = false;
int timer = 3000;

float lastFrameTime = 0.0f;

int playerHealth = 100;

bool reachedF2 = false;
bool reachedF3 = false;

enum GameState { PLAYING, GAME_WON, GAME_LOST, LEVEL2_START, LEVEL2_PLAYING };
GameState currentGameState = PLAYING;

Model_3DS model_coin;
Model_3DS model_tree;
Model_3DS model_key;
Model_3DS model_trap;
Model_3DS model_crate;
Model_3DS model_player;
Model_3DS model_door1;
Model_3DS model_door2;
Model_3DS model_swtrap;
Model_3DS model_gem;
Model_3DS model_map;

GLTexture tex_crate;
GLTexture tex_floor;
GLTexture tex_wall1;
GLTexture tex_barell;
GLTexture tex_wall2;
GLTexture tex_floor2;



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

		// Calculate the rotation angle for spinning
		float rotationAngle = fmod(time * 100.0f, 360.0f); // Rotation speed factor: 100.0f

		// Calculate the glow intensity using a sine wave for pulsing effect
		float glowIntensity = 0.8f + 0.2f * sin(time * 3.0f); // Varies between 0.8 and 1.0

		// Set emissive material for the glow effect
		GLfloat emissiveMaterial[] = { glowIntensity, glowIntensity * 0.85f, 0.0f, 1.0f }; // Golden glow
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissiveMaterial);

		// Push transformation matrix
		glPushMatrix();

		// Position the coin with vertical movement
		glTranslatef(x, y + verticalOffset, z);

		// Rotate the coin around its y-axis for spinning effect
		glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);

		// Scale the model
		glScalef(4.0f, 4.0f, 4.0f); // Adjust scaling as needed

		// Draw the coin model
		model_coin.Draw(); // Render the .3ds model

		// Reset emissive material to turn off glow effect for other objects
		GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);

		// Pop transformation matrix
		glPopMatrix();

		// Add a soft halo effect using blending
		glDisable(GL_LIGHTING); // Disable lighting for halo
		glEnable(GL_BLEND);     // Enable blending for transparency
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for the halo

		glPushMatrix();

		// Position the halo slightly above the coin
		glTranslatef(x, y + verticalOffset, z);

		// Use a semi-transparent golden-yellow color for the halo
		glColor4f(1.0f, 0.85f, 0.0f, 0.2f); // Semi-transparent gold
		glutSolidSphere(radius * 1.5f, 32, 32); // Slightly larger than the coin for halo effect

		glPopMatrix();

		glDisable(GL_BLEND); // Disable blending
		glEnable(GL_LIGHTING); // Re-enable lighting
	}



	void checkCollision(float playerX, float playerY, float playerZ) {
		if (collected) return; // Skip if the coin is already collected

		// Calculate the distance between the player and the coin
		float distance = sqrt(pow(playerX - x, 2) + pow(playerY + 1 - y, 2) + pow(playerZ - z, 2));

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

Coin coin18(3.8f, 7.3f, 9.2f, 0.3f);
Coin coin19(-1.2f, 7.3f, -0.1f, 0.3f);
Coin coin20(-1.7f, 7.3f, -7.7f, 0.3f);
Coin coin21(4.7f, 7.3f, -6.3f, 0.3f);
Coin coin22(3.7f, 7.3f, -12.3f, 0.3f);

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
		float amplitude = 0.3f; // Maximum height of the spikes
		float frequency = 5.0f; // Frequency of the movement
		float y = amplitude * sin(frequency * time);

		// Draw traps
		glPushMatrix();
		glTranslatef(x, y, z);
		glEnable(GL_TEXTURE_2D);  // Enable 2D texturing

		// Reset material properties (use white diffuse and ambient to avoid color influence)
		GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
		GLfloat defaultShininess = 0.0f;

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);
		glScalef(4.02f, 8.02f, 1.01f); // Adjust the scale as needed
		model_trap.Draw();
		glPopMatrix();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);

		// Draw spikes with full transparency
		glColor4f(0.5f, 0.5f, 0.5f, 0.0f); // Gray color with 0 alpha (fully transparent)
		for (const auto& spike : spikes) {
			glPushMatrix();
			glTranslatef(spike.x, y, spike.z);
			glRotatef(-90, 1.0f, 0.0f, 0.0f);
			glutSolidCone(0.1f, spikeHeight, 10, 2);
			glPopMatrix();
		}
		// Re-enable lighting after drawing the spikes
		glEnable(GL_LIGHTING);
	}



	void checkCollision(float playerX, float playerY, float playerZ) {
		// Check if player is within the trap's area
		if (playerX >= x - width / 2 && playerX <= x + width / 2 &&
			playerZ >= z - depth / 2 && playerZ <= z + depth / 2) {
			// Check if player is touching the spikes
			if (y >= 0.0f && y <= 1.0f) {
				// Check if player is above the trap
				if (playerY <= y) {
					playerHealth -= 1; // Decrease player health
					std::cout << "Player hit by trap! Health: " << playerHealth << std::endl;
					if (playerHealth <= 0) {
						std::cout << "Player is dead!" << std::endl;
						gameLost = true;
						// Handle player death (e.g., end game, respawn, etc.)
					}
				}
				else {
					std::cout << "Player jumped over the trap!" << std::endl;
				}
			}
		}
	}

};
Trap trap1(-5.6f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);
Trap trap2(-3.6f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);
Trap trap3(-1.6f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);
Trap trap4(0.4f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);
Trap trap5(2.4f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);
Trap trap6(4.4f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);
Trap trap7(5.6f, 1.0f, 3.0f, 2.0f, 0.5f, 0.6f, 0.0f);

Trap trap8(-5.6f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap9(-3.6f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap10(-1.6f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap11(0.4f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap12(2.4f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap13(4.4f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap14(5.6f, 1.0f, -10.0f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap15(6.2f, 1.0f, -0.9f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap16(9.3f, 1.0f, 4.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap17(11.4f, 1.0f, 4.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap18(13.5f, 1.0f, 4.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap19(15.6f, 1.0f, 4.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap20(17.7f, 1.0f, 4.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap21(19.8f, 1.0f, 4.2f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap22(14.8f, 1.0f, -2.5f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap23(14.8f, 1.0f, -4.6f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap24(14.8f, 1.0f, -6.7f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap25(14.8f, 1.0f, -8.8f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap26(14.8f, 1.0f, -10.1f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap27(14.8f, 1.0f, -2.5f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap28(16.9f, 1.0f, -2.5f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap29(19.0f, 1.0f, -2.5f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap30(20.7f, 1.0f, -2.5f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap31(-0.8f, 1.0f, -15.7f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap32(-5.7f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap33(-3.6f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap34(-1.5f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap35(0.6f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap36(2.7f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap37(4.8f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap38(5.7f, 1.0f, -24.9f, 2.0f, 0.5f, 0.6f, 0.01f);

Trap trap39(-5.7f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap40(-3.6f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap41(-1.5f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap42(0.6f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap43(2.7f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap44(4.8f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);
Trap trap45(5.7f, 1.0f, -29.2f, 2.0f, 0.5f, 0.6f, 0.01f);

class AncientKey {
public:
	float x, y, z;
	float radius;
	bool collected;

	AncientKey(float _x, float _y, float _z, float _radius)
		: x(_x), y(_y), z(_z), radius(_radius), collected(false) {}

	void draw(float time) {
		if (collected) return; // Skip drawing if the key is collected

		// Calculate the vertical offset using a sine wave for smooth up and down movement
		float verticalOffset = 0.2f * sin(time);

		glPushMatrix();
		glTranslatef(x, y + verticalOffset + 0.2, z); // Apply the vertical offset

		glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset the color to white

		// Reset material properties (use white diffuse and ambient to avoid color influence)
		GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
		GLfloat defaultShininess = 0.0f;

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

		// Draw the coin model
		glScalef(1.0f, 1.0f, 1.0f); // Scale the model as needed
		model_key.Draw();

		glPopMatrix();
		glDisable(GL_TEXTURE_2D);
	}

	void checkCollision(float playerX, float playerZ) {
		if (collected) return; // Skip if the key is already collected

		// Calculate the distance between the player and the key
		float distance = sqrt(pow(playerX - x, 2) + pow(playerZ - z, 2));

		// Check if the distance is less than the sum of the radii (collision detection)
		if (distance < radius + 0.5f) { // Assuming player radius is 0.5f
			collected = true; // Mark the key as collected
			std::cout << "Ancient Key collected!" << std::endl;

		}
	}
};

AncientKey key1(-2.6f, 1.0f, -36.1f, 0.3f);

class MapObject {
public:
	float x, y, z;
	float radius;
	bool collected;

	MapObject(float _x, float _y, float _z, float _radius)
		: x(_x), y(_y), z(_z), radius(_radius), collected(false) {}

	void draw(float time) {
		if (collected) return; // Skip drawing if the coin is collected

		// Calculate the vertical offset using a sine wave for smooth up and down movement
		float verticalOffset = 0.2f * sin(time);

		// Calculate the glow intensity using a sine wave
		float glowIntensity = 0.75f + 0.25f * sin(time * 2.0f); // Varies between 0.75 and 1.0

		glPushMatrix();
		glTranslatef(x, y + verticalOffset, z); // Apply the vertical offset

		// Reset OpenGL state before drawing
		glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
		glColor4f(glowIntensity, glowIntensity, glowIntensity, 1.0f); // Apply the glow effect

		// Reset material properties (use white diffuse and ambient to avoid color influence)
		GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
		GLfloat defaultShininess = 0.0f;

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

		// Draw the coin model
		glScalef(0.60f, 0.60f, 0.30f); // Scale the model as needed
		model_map.Draw(); // Render the .3ds model

		glPopMatrix();

		// Reset OpenGL state if needed
		glDisable(GL_TEXTURE_2D); // Disable 2D texturing if not needed elsewhere
	}




	void checkCollision(float playerX, float playerY, float playerZ) {
		if (collected) return; // Skip if the coin is already collected

		// Calculate the distance between the player and the coin
		float distance = sqrt(pow(playerX - x, 2) + pow(playerY + 1 - y, 2) + pow(playerZ - z, 2));

		// Check if the distance is less than the sum of the radii (collision detection)
		if (distance < radius + 0.5f) { // Assuming player radius is 0.5f
			collected = true; // Mark the coin as collected
			std::cout << "map collected! Score: " << score << std::endl;

			// Optionally, you can play a sound effect
			// PlaySoundEffect("coin_collect.wav");
		}
	}
};
MapObject map1(-4.4f, 1.0f, -11.6f, 0.3f);


class Gem {
public:
	float x, y, z;
	float radius;
	bool collected;

	Gem(float _x, float _y, float _z, float _radius)
		: x(_x), y(_y), z(_z), radius(_radius), collected(false) {}

	void draw(float time) {
		if (collected) return; // Skip drawing if the gem is collected

		// Calculate the vertical offset using a sine wave for smooth up and down movement
		float verticalOffset = 0.2f * sin(time);

		// Calculate the glow intensity using a sine wave
		float glowIntensity = 0.85f + 0.25f * sin(time * 2.0f); // Varies between 0.75 and 1.0

		glPushMatrix();
		glTranslatef(x, y + verticalOffset + 1, z); // Apply the vertical offset

		// Reset OpenGL state before drawing
		glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
		glColor4f(glowIntensity, glowIntensity, glowIntensity, 1.0f); // Apply the glow effect

		// Reset material properties (use white diffuse and ambient to avoid color influence)
		GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
		GLfloat defaultShininess = 0.0f;

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

		// Draw the gem model
		glScalef(0.5f, 0.5f, 0.5f); // Scale the model as needed
		model_gem.Draw(); // Render the .3ds model (assuming you have a gem model)

		glPopMatrix();

		// Reset OpenGL state if needed
		glDisable(GL_TEXTURE_2D); // Disable 2D texturing if not needed elsewhere
	}

	void checkCollision(float playerX, float playerY, float playerZ) {
		if (collected) return; // Skip if the gem is already collected

		// Calculate the distance between the player and the gem
		float distance = sqrt(pow(playerX - x, 2) + pow(playerY - y, 2) + pow(playerZ - z, 2));

		// Check if the distance is less than the sum of the radii (collision detection)
		if (distance < radius + 0.5f) { // Assuming player radius is 0.5f
			collected = true; // Mark the gem as collected
			//score += 200; // Increase score by 200
			std::cout << "Gem collected! Score: " << score << std::endl;

			// Optionally, you can play a sound effect
			// PlaySoundEffect("gem_collect.wav");
		}
	}
};
Gem gem1(-3.0f, 6.5f, 10.0f, 0.5f); // Example gem object with specified position and radius


class Crate {
public:
	float x, y, z;
	float width, height, depth;

	Crate(float _x, float _y, float _z, float _width, float _height, float _depth)
		: x(_x), y(_y), z(_z), width(_width), height(_height), depth(_depth) {}

	void draw() {
		glPushMatrix();
		glTranslatef(x, y, z);

		// Bind the texture
		glEnable(GL_TEXTURE_2D);
		tex_crate.Use();

		// Set color to white to avoid any color tint
		glColor3f(1.0f, 1.0f, 1.0f);

		// Draw the textured crate
		glBegin(GL_QUADS);

		// Front face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, depth / 2);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, depth / 2);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, depth / 2);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, depth / 2);

		// Back face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, -depth / 2);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, -depth / 2);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, -depth / 2);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, -depth / 2);

		// Left face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, -depth / 2);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-width / 2, -height / 2, depth / 2);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-width / 2, height / 2, depth / 2);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, -depth / 2);

		// Right face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, -depth / 2);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, depth / 2);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, depth / 2);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, height / 2, -depth / 2);

		// Top face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, height / 2, -depth / 2);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, height / 2, -depth / 2);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, depth / 2);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, depth / 2);

		// Bottom face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, -depth / 2);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, -depth / 2);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, -height / 2, depth / 2);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, -height / 2, depth / 2);

		glEnd();

		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
	}


	bool checkCollision(float playerX, float playerZ, float playerRadius) const {
		float halfWidth = width / 2.0f;
		float halfDepth = depth / 2.0f;

		// Check if the player is within the bounds of the crate
		if (playerX + playerRadius > x - halfWidth && playerX - playerRadius < x + halfWidth &&
			playerZ + playerRadius > z - halfDepth && playerZ - playerRadius < z + halfDepth) {
			return true;
		}
		return false;
	}
};


class Barrel {
public:
	float x, y, z;
	float radius, height;

	Barrel(float _x, float _y, float _z, float _radius, float _height)
		: x(_x), y(_y), z(_z), radius(_radius), height(_height) {}

	void draw() {
		glPushMatrix();
		glTranslatef(x, y, z);
		glColor3f(1.0f, 1.0f, 1.0f); // Set color to white to avoid tinting the texture

		// Set material properties to increase brightness
		GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
		GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
		GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		GLfloat mat_shininess[] = { 50.0f };

		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

		// Enable texturing
		glEnable(GL_TEXTURE_2D);
		tex_barell.Use(); // Bind the tex_barell texture

		GLUquadric* quad = gluNewQuadric();
		gluQuadricTexture(quad, GL_TRUE); // Enable texture coordinates for the quadric

		// Draw the cylinder
		gluCylinder(quad, radius, radius, height, 20, 1);

		// Draw the top base
		glPushMatrix();
		glTranslatef(0.0f, 0.0f, height);
		gluDisk(quad, 0.0f, radius, 20, 1);
		glPopMatrix();

		// Draw the bottom base
		gluDisk(quad, 0.0f, radius, 20, 1);

		// Disable texturing
		glDisable(GL_TEXTURE_2D);

		glPopMatrix();
	}



	bool checkCollision(float playerX, float playerZ, float playerRadius) const {
		// Calculate the distance between the player and the barrel
		float distance = sqrt(pow(playerX - x, 2) + pow(playerZ - z, 2));

		// Check if the distance is less than the sum of the radii (collision detection)
		if (distance < radius + playerRadius) {
			return true;
		}
		return false;
	}
};

// Example instantiation and usage
Crate crate1(-3.0f, 0.0f, 6.5f, 1.0f, 1.0f, 1.0f);
Barrel barrel1(1.5f, 0.5f, 6.5f, 0.5f, 1.0f);


// x = from -5.2 to -2.9, z = 9
Crate crate2(-5.2f, 0.0f, 9.0f, 1.0f, 1.0f, 1.0f);
Crate crate3(-4.2f, 0.0f, 9.0f, 1.0f, 1.0f, 1.0f);
Crate crate4(-3.2f, 0.0f, 9.0f, 1.0f, 1.0f, 1.0f);

// x = -2.9, z = from 8.5 to 4.5
Barrel barrel2(-2.9f, 0.5f, 8.5f, 0.5f, 1.0f);
Barrel barrel3(-2.9f, 0.5f, 7.5f, 0.5f, 1.0f);
Barrel barrel4(-2.9f, 0.5f, 6.5f, 0.5f, 1.0f);
Barrel barrel5(-2.9f, 0.5f, 5.5f, 0.5f, 1.0f);
Barrel barrel6(-2.9f, 0.5f, 4.5f, 0.5f, 1.0f);

// x = 4, z = from 6.8 to 4.4
Crate crate5(4.0f, 0.0f, 6.8f, 1.0f, 1.0f, 1.0f);
Crate crate6(4.0f, 0.0f, 5.8f, 1.0f, 1.0f, 1.0f);
Crate crate7(4.0f, 0.0f, 4.8f, 1.0f, 1.0f, 1.0f);

// x = from 5.3 to -0.4, z = 12.6
Barrel barrel7(5.3f, 0.5f, 12.6f, 0.5f, 1.0f);
Barrel barrel8(4.3f, 0.5f, 12.6f, 0.5f, 1.0f);
Barrel barrel9(3.3f, 0.5f, 12.6f, 0.5f, 1.0f);
Barrel barrel10(2.3f, 0.5f, 12.6f, 0.5f, 1.0f);
Barrel barrel11(1.3f, 0.5f, 12.6f, 0.5f, 1.0f);
Barrel barrel12(0.3f, 0.5f, 12.6f, 0.5f, 1.0f);
Barrel barrel13(-0.4f, 0.5f, 12.6f, 0.5f, 1.0f);

// x = from -0.4 to -5.4, z = -1.8
Crate crate8(-0.4f, 0.0f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate9(-1.4f, 0.0f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate10(-2.4f, 0.0f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate11(-3.4f, 0.0f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate12(-4.4f, 0.0f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate13(-5.4f, 0.0f, -1.8f, 1.0f, 1.0f, 1.0f);

// x = from -0.6 to -5.4, z = -6.3
Barrel barrel14(-0.6f, 0.5f, -6.3f, 0.5f, 1.0f);
Barrel barrel15(-1.6f, 0.5f, -6.3f, 0.5f, 1.0f);
Barrel barrel16(-2.6f, 0.5f, -6.3f, 0.5f, 1.0f);
Barrel barrel17(-3.6f, 0.5f, -6.3f, 0.5f, 1.0f);
Barrel barrel18(-4.6f, 0.5f, -6.3f, 0.5f, 1.0f);
Barrel barrel19(-5.4f, 0.5f, -6.3f, 0.5f, 1.0f);

// x = from 2.8 to 5.5, z = -4.6
Crate crate14(2.8f, 0.0f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate15(3.8f, 0.0f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate16(4.8f, 0.0f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate17(5.5f, 0.0f, -4.6f, 1.0f, 1.0f, 1.0f);

// x = 9.3, z = 1.1
Crate crate18(9.3f, 0.0f, 1.1f, 1.0f, 1.0f, 1.0f);

// x = from 9.7 to 11.9, z = -6.8
Barrel barrel20(9.7f, 0.5f, -6.8f, 0.5f, 1.0f);
Barrel barrel21(10.7f, 0.5f, -6.8f, 0.5f, 1.0f);
Barrel barrel22(11.9f, 0.5f, -6.8f, 0.5f, 1.0f);

// x = 20, z = -9.5
Crate crate19(20.0f, 0.0f, -9.5f, 1.0f, 1.0f, 1.0f);

// x = 16.5, z = from -7.1 to -9.8
Barrel barrel23(16.5f, 0.5f, -7.1f, 0.5f, 1.0f);
Barrel barrel24(16.5f, 0.5f, -8.1f, 0.5f, 1.0f);
Barrel barrel25(16.5f, 0.5f, -9.1f, 0.5f, 1.0f);
Barrel barrel26(16.5f, 0.5f, -9.8f, 0.5f, 1.0f);

// x = 13.4, z = 2.1
Crate crate20(13.4f, 0.0f, 2.1f, 1.0f, 1.0f, 1.0f);

// x = 19.6, z = from 1.9 to 0.4
Barrel barrel27(19.6f, 0.5f, 1.9f, 0.5f, 1.0f);
Barrel barrel28(19.6f, 0.5f, 0.9f, 0.5f, 1.0f);
Barrel barrel29(19.6f, 0.5f, 0.4f, 0.5f, 1.0f);

// x = 17.3, z = from 7.8 to 9.6
Crate crate21(17.3f, 0.0f, 7.8f, 1.0f, 1.0f, 1.0f);
Crate crate22(17.3f, 0.0f, 8.8f, 1.0f, 1.0f, 1.0f);
Crate crate23(17.3f, 0.0f, 9.6f, 1.0f, 1.0f, 1.0f);

// x = 11.1, z = from 7.3 to 9.7
Barrel barrel30(11.1f, 0.5f, 7.3f, 0.5f, 1.0f);
Barrel barrel31(11.1f, 0.5f, 8.3f, 0.5f, 1.0f);
Barrel barrel32(11.1f, 0.5f, 9.3f, 0.5f, 1.0f);
Barrel barrel33(11.1f, 0.5f, 9.7f, 0.5f, 1.0f);

// x = 0.9, z = -13.2
Crate crate24(0.9f, 0.0f, -13.2f, 1.0f, 1.0f, 1.0f);

// x = 2.6, z = from -20.2 to -22.5
Barrel barrel34(2.6f, 0.5f, -20.2f, 0.5f, 1.0f);
Barrel barrel35(2.6f, 0.5f, -21.2f, 0.5f, 1.0f);
Barrel barrel36(2.6f, 0.5f, -22.2f, 0.5f, 1.0f);
Barrel barrel37(2.6f, 0.5f, -22.5f, 0.5f, 1.0f);

Crate crate25(-3.0f, 0.0f, 6.5f, 1.0f, 1.0f, 1.0f);
Barrel barrel38(1.5f, 0.5f, 6.5f, 0.5f, 1.0f);

Crate crate26(-3.3f, 0.0f, -23.2f, 1.0f, 1.0f, 1.0f);
Barrel barrel39(-4.0f, 0.5f, -23.2f, 0.5f, 1.0f);
Crate crate27(-5.4f, 0.0f, -23.2f, 1.0f, 1.0f, 1.0f);

Crate crate28(-4.9f, 0.0f, -32.6f, 1.0f, 1.0f, 1.0f);
Barrel barrel40(-3.5f, 0.5f, -32.6f, 0.5f, 1.0f);
Crate crate29(-2.0f, 0.0f, -32.6f, 1.0f, 1.0f, 1.0f);

Barrel barrel41(5.1f, 0.5f, -39.1f, 0.5f, 1.0f);

//----------------------------------------------

Crate crate30(-3.0f, 6.6f, 6.5f, 1.0f, 1.0f, 1.0f);
Crate crate31(-5.2f, 6.6f, 9.0f, 1.0f, 1.0f, 1.0f);
Crate crate32(-4.2f, 6.6f, 9.0f, 1.0f, 1.0f, 1.0f);
Crate crate33(-3.2f, 6.6f, 9.0f, 1.0f, 1.0f, 1.0f);
Crate crate34(4.0f, 6.6f, 6.8f, 1.0f, 1.0f, 1.0f);
Crate crate35(4.0f, 6.6f, 5.8f, 1.0f, 1.0f, 1.0f);
Crate crate36(4.0f, 6.6f, 4.8f, 1.0f, 1.0f, 1.0f);
Crate crate37(-0.4f, 6.6f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate38(-1.4f, 6.6f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate39(-2.4f, 6.6f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate40(-3.4f, 6.6f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate41(-4.4f, 6.6f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate42(-5.4f, 6.6f, -1.8f, 1.0f, 1.0f, 1.0f);
Crate crate43(2.8f, 6.6f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate44(3.8f, 6.6f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate45(4.8f, 6.6f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate46(5.5f, 6.6f, -4.6f, 1.0f, 1.0f, 1.0f);
Crate crate47(9.3f, 6.6f, 1.1f, 1.0f, 1.0f, 1.0f);
Crate crate48(20.0f, 6.6f, -9.5f, 1.0f, 1.0f, 1.0f);
Crate crate49(13.4f, 6.6f, 2.1f, 1.0f, 1.0f, 1.0f);
Crate crate50(17.3f, 6.6f, 7.8f, 1.0f, 1.0f, 1.0f);
Crate crate51(17.3f, 6.6f, 8.8f, 1.0f, 1.0f, 1.0f);
Crate crate52(17.3f, 6.6f, 9.6f, 1.0f, 1.0f, 1.0f);
Crate crate53(0.9f, 6.6f, -13.2f, 1.0f, 1.0f, 1.0f);
Crate crate54(-3.0f, 6.6f, 6.5f, 1.0f, 1.0f, 1.0f);
Crate crate55(-3.3f, 6.6f, -23.2f, 1.0f, 1.0f, 1.0f);
Crate crate56(-5.4f, 6.6f, -23.2f, 1.0f, 1.0f, 1.0f);
Crate crate57(-4.9f, 6.6f, -32.6f, 1.0f, 1.0f, 1.0f);
Crate crate58(-2.0f, 6.6f, -32.6f, 1.0f, 1.0f, 1.0f);

class SwingingTrap {
public:
	float x, y, z; // Position of the pivot point
	float length; // Length of the pendulum
	float angle; // Current angle of the pendulum
	float angularSpeed; // Speed of the swinging motion
	float radius; // Radius of the trap for collision detection
	bool hit; // Whether the trap has hit the player

	SwingingTrap(float _x, float _y, float _z, float _length, float _radius)
		: x(_x), y(_y), z(_z), length(_length), angle(0.0f), angularSpeed(1.0f), radius(_radius), hit(false) {}

	void update(float deltaTime) {
		// Update the angle using a simple harmonic motion formula
		angle = sin(glutGet(GLUT_ELAPSED_TIME) * 0.001f * angularSpeed) * 45.0f; // Swing between -45 and 45 degrees
	}

	void draw() {
		glPushMatrix();
		glTranslatef(x, y, z);
		glRotatef(angle, 0.0f, 0.0f, 1.0f); // Rotate around the Z-axis


		// Draw the pendulum arm
		glBegin(GL_LINES);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, -length, 0.0f);
		glEnd();

		// Draw the trap at the end of the pendulum
		glTranslatef(0.0f, -length, 0.0f);
		glScalef(0.15f, 0.15f, 0.15f); // Scale the model as needed
		model_swtrap.Draw();
		//glutSolidSphere(radius, 20, 20); // Draw the trap as a sphere

		glPopMatrix();
	}

	void checkCollision(float playerX, float playerY, float playerZ) {
		//if (hit) return; // Skip if the trap has already hit the player

		// Calculate the position of the trap
		float trapX = x + length * sin(DEG2RAD(angle));
		float trapY = y - length * cos(DEG2RAD(angle));

		// Calculate the distance between the player and the trap
		float distance = sqrt(pow(playerX - trapX, 2) + pow(playerY - trapY, 2) + pow(playerZ - z, 2));

		// Check if the distance is less than the sum of the radii (collision detection)
		if (distance < radius + 1.5f) { // Assuming player radius is 0.5f
			hit = true; // Mark the trap as hit
			playerHealth -= 1; // Decrement player health by 10
			std::cout << "Player hit by trap! Health: " << playerHealth << std::endl;

			// Optionally, you can play a sound effect
			// PlayCollisionSound("trap_hit.wav");
		}
	}
};

// Example instance of the SwingingTrap class
SwingingTrap swingingTrap1(1.0f, 4.0f, 6.8f, 3.0f, 0.5f);
SwingingTrap swingingTrap2(2.8f, 4.0f, 0.8f, 3.0f, 0.5f);
SwingingTrap swingingTrap3(1.0f, 11.0f, 6.8f, 3.0f, 0.5f);
SwingingTrap swingingTrap4(2.8f, 11.0f, 0.8f, 3.0f, 0.5f);

class Door {
public:
	Vector3f position;
	float width, height;
	float openAngle;
	bool isOpen;
	Vector3f cubePosition;
	float cubeSize;

	Door(float x, float y, float z, float w, float h, float cubeSize)
		: position(x, y, z), width(w), height(h), openAngle(0.0f), isOpen(false), cubeSize(cubeSize) {
		cubePosition = Vector3f(x, y, z + w); // Position the cube behind the door
	}

	void draw() {
		// Draw the door
		if (currentGameState == PLAYING) {
			glPushMatrix();
			glTranslatef(position.x - 0.2, position.y, position.z);
			glRotatef(openAngle + 90, 0.0f, 1.0f, 0.0f);
			glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset the color to white

			// Reset material properties (use white diffuse and ambient to avoid color influence)
			GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
			GLfloat defaultShininess = 0.0f;

			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

			// Draw the coin model
			glScalef(1.0f, 1.0f, 1.0f); // Scale the model as needed
			model_door2.Draw();
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);

			// Draw the cube behind the door
			glPushMatrix();
			glTranslatef(cubePosition.x, cubePosition.y, cubePosition.z - 3);
			glColor3f(1.0f, 1.0f, 1.0f); // Blue color for the cube
			glutSolidCube(cubeSize);
			glPopMatrix();
		}
		if (currentGameState == LEVEL2_PLAYING) {
			glPushMatrix();
			glTranslatef(0.0, 15.4, 3.9);
			glRotatef(openAngle + 90, 0.0f, 1.0f, 0.0f);
			glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset the color to white

			// Reset material properties (use white diffuse and ambient to avoid color influence)
			GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
			GLfloat defaultShininess = 0.0f;

			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

			// Draw the coin model
			glScalef(10.02, 10.02, 10.02);
			model_door1.Draw();
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);

			// Draw the cube behind the door
			glPushMatrix();
			glTranslatef(cubePosition.x, cubePosition.y, cubePosition.z - 3);
			glColor3f(1.0f, 1.0f, 1.0f); // Blue color for the cube
			glutSolidCube(cubeSize);
			glPopMatrix();
		}


	}

	void openDoor() {
		if (key1.collected && currentGameState == PLAYING) {
			if (openAngle < 90.0f) {
				openAngle += 0.5f; // Adjust the speed of opening as needed
				if (openAngle >= 90.0f) {
					openAngle = 90.0f;
					isOpen = true;
				}
			}
		}
		if (gem1.collected && currentGameState == LEVEL2_PLAYING) {
			if (openAngle < 90.0f) {
				openAngle += 0.5f; // Adjust the speed of opening as needed
				if (openAngle >= 90.0f) {
					openAngle = 90.0f;
					isOpen = true;
				}
			}
		}

	}

	bool checkCollision(Vector3f playerPosition) {
		if (isOpen) {
			float distance = (playerPosition - cubePosition).length();
			if (distance < cubeSize / 2.0f) {
				// Handle collision with the cube behind the open door
				std::cout << "Player collided with the cube behind the open door!" << std::endl;
				// Add your collision handling logic here
				return true;
			}
		}
		return false;
	}
};

class Door2 {
public:
	Vector3f position;
	float width, height;
	float openAngle;
	bool isOpen;
	Vector3f cubePosition;
	float cubeSize;

	Door2(float x, float y, float z, float w, float h, float cubeSize)
		: position(x, y, z), width(w), height(h), openAngle(0.0f), isOpen(false), cubeSize(cubeSize) {
		cubePosition = Vector3f(x, y, z + w); // Position the cube behind the door
	}

	void draw() {
		// Draw the door
		if (currentGameState == PLAYING) {
			glPushMatrix();
			glTranslatef(position.x - 0.2, position.y, position.z);
			glRotatef(openAngle + 90, 0.0f, 1.0f, 0.0f);
			glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset the color to white

			// Reset material properties (use white diffuse and ambient to avoid color influence)
			GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
			GLfloat defaultShininess = 0.0f;

			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

			// Draw the coin model
			glScalef(1.0f, 1.0f, 1.0f); // Scale the model as needed
			model_door2.Draw();
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);

			// Draw the cube behind the door
			glPushMatrix();
			glTranslatef(cubePosition.x, cubePosition.y, cubePosition.z - 3);
			glColor3f(1.0f, 1.0f, 1.0f); // Blue color for the cube
			glutSolidCube(cubeSize);
			glPopMatrix();
		}
		if (currentGameState == LEVEL2_PLAYING) {
			glPushMatrix();
			glTranslatef(position.x - 0.2, position.y, position.z);
			glRotatef(openAngle + 90, 0.0f, 1.0f, 0.0f);
			glEnable(GL_TEXTURE_2D);  // Enable 2D texturing
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset the color to white

			// Reset material properties (use white diffuse and ambient to avoid color influence)
			GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat defaultSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Specular off for simplicity
			GLfloat defaultShininess = 0.0f;

			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultSpecular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, defaultShininess);

			// Draw the coin model
			glScalef(10.02, 10.02, 10.02);
			model_door1.Draw();
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);

			// Draw the cube behind the door
			glPushMatrix();
			glTranslatef(cubePosition.x, cubePosition.y, cubePosition.z - 6);
			glColor3f(1.0f, 1.0f, 1.0f); // Blue color for the cube
			glutSolidCube(cubeSize);
			glPopMatrix();
		}


	}

	void openDoor() {

		if (gem1.collected && currentGameState == LEVEL2_PLAYING) {
			if (openAngle < 90.0f) {
				openAngle += 0.5f; // Adjust the speed of opening as needed
				if (openAngle >= 90.0f) {
					openAngle = 90.0f;
					isOpen = true;
				}
			}
		}

	}

	bool checkCollision(Vector3f playerPosition) {
		if (isOpen) {
			float distance = (playerPosition - cubePosition).length();
			if (distance < cubeSize / 2.0f) {
				// Handle collision with the cube behind the open door
				std::cout << "Player collided with the cube behind the open door!" << std::endl;
				// Add your collision handling logic here
				return true;
			}
		}
		return false;
	}
};

// Example usage
Door door(0.8f, 0.0f, -39.1f, 2.0f, 4.0f, 1.5f);
Door2 door2(0.0f, 15.4f, 3.9f, 2.0f, 4.0f, 1.5f);

void updateDoor(int value) {

	door.openDoor();
	glutPostRedisplay();
	glutTimerFunc(16, updateDoor, 0); // Update every 16ms (~60 FPS)
}
void updateDoor2(int value) {

	door2.openDoor();
	glutPostRedisplay();
	glutTimerFunc(16, updateDoor2, 0); // Update every 16ms (~60 FPS)
}


float playerX = -3.0f;  // Player's initial X position
float playerZ = 10.0f;  // Player's initial Z position
float playerY = -0.3f;  // Player's initial Y position (for jumping)
float playerAngle = 0.0f;  // Player's facing direction in degrees
bool isJumping = false;
float jumpStartTime = 0.0f;
float jumpDuration = 1.0f; // Duration of the jump in seconds
float jumpHeight = 10.0f;
float legAngle = 0.0f;  // Angle for leg movement (for walking effect)
bool legMovingForward = true;  // Direction of leg movement (for walking)

void drawPlayer() {
	glPushMatrix();
	glTranslatef(playerX, playerY + 0.2, playerZ); // Player's position
	glRotatef(playerAngle + 180, 0.0f, 1.0f, 0.0f); // Rotate player to face the camera's view direction

	// Update spotlight position (above the player)
	GLfloat playerLightPosition[] = { playerX, playerY + 40.0f, playerZ, 40.0f };
	glLightfv(GL_LIGHT1, GL_POSITION, playerLightPosition);

	// Update spotlight direction (parallel to the XZ plane)
	GLfloat playerLightDirection[] = { sin(DEG2RAD(playerAngle+180)), 0.0f, cos(DEG2RAD(playerAngle)) };
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, playerLightDirection);

	// Draw the player model
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	GLfloat defaultDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat defaultAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defaultDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defaultAmbient);

	glScalef(1.5f, 1.5f, 1.5f);
	model_player.Draw();

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
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
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	if (currentGameState == PLAYING) {
		tex_floor.Use();
	}
	else {
		tex_wall2.Use();
	}

	glPushMatrix();
	glScaled(width, height, thickness);
	glColor3f(1.0f, 1.0f, 1.0f); // Set material properties to increase brightness
	// Set material properties to increase brightness
	GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// Draw the wall with texture coordinates
	glBegin(GL_QUADS);
	// Front face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Back face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	// Top face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Bottom face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	// Right face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	// Left face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glEnd();

	glPopMatrix();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
}


float toRadians(float degrees) {
	return degrees * (3.14159f / 180.0f);
}

void drawCourt() {
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	tex_floor.Use(); // Bind the tex_floor texture
	if (currentGameState == PLAYING) {
		tex_floor.Use();
	}
	else {
		tex_floor2.Use();
	}
	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(0.0f, -0.15f, 0.0f);  // Move cube slightly below the court surface
	glColor3f(0.8f, 0.8f, 1.0f);   // Lighter brownish color for the ground
	glScaled(12.0f, 0.2f, 26.8f);     // Scale the cube to match the court size and add thickness

	// Set material properties to increase brightness
	GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// Draw the ground cube with texture coordinates
	glBegin(GL_QUADS);
	// Front face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Back face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	// Top face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Bottom face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	// Right face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	// Left face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glEnd();

	glPopMatrix();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
}



void drawRoom() {
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	tex_floor.Use(); // Bind the tex_floor texture

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(15.0f, -0.15f, 0.0f);  // Move cube slightly below the court surface
	glColor3f(1.0f, 1.0f, 1.0f);   // Lighter brownish color for the ground
	glScaled(12.0f, 0.2f, 20.8f);     // Scale the cube to match the court size and add thickness

	// Set material properties to increase brightness
	GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// Draw the ground cube with texture coordinates
	glBegin(GL_QUADS);
	// Front face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Back face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	// Top face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Bottom face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	// Right face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	// Left face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glEnd();

	glPopMatrix();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
}


void drawRoom2() {
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	tex_floor.Use(); // Bind the tex_floor texture

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(0.0f, -0.15f, -30.0f);  // Move cube slightly below the court surface
	glColor3f(1.0f, 1.0f, 1.0f);   // Lighter brownish color for the ground
	glScaled(12.0f, 0.2f, 20.8f);     // Scale the cube to match the court size and add thickness

	// Set material properties to increase brightness
	GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// Draw the ground cube with texture coordinates
	glBegin(GL_QUADS);
	// Front face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Back face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	// Top face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Bottom face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	// Right face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	// Left face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glEnd();

	glPopMatrix();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
}

void drawCoridor1() {
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	tex_floor.Use(); // Bind the tex_floor texture

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(7.5f, -0.15f, 0.0f);  // Move cube slightly below the court surface
	glColor3f(1.0f, 1.0f, 1.0f);   // Lighter brownish color for the ground
	glScaled(3.0f, 0.2f, 3.8f);     // Scale the cube to match the court size and add thickness

	// Set material properties to increase brightness
	GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// Draw the ground cube with texture coordinates
	glBegin(GL_QUADS);
	// Front face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Back face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	// Top face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Bottom face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	// Right face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	// Left face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glEnd();

	glPopMatrix();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
}

void drawCoridor2() {
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	tex_floor.Use(); // Bind the tex_floor texture

	// Add a cube beneath the court to give the impression of thickness (ground)
	glPushMatrix();
	glTranslatef(0.0f, -0.15f, -16.0f);  // Move cube slightly below the court surface
	glColor3f(1.0f, 1.0f, 1.0f);   // Lighter color for the ground
	glScaled(3.0f, 0.2f, 7.0f);     // Scale the cube to match the court size and add thickness

	// Set material properties to increase brightness
	GLfloat mat_ambient[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	GLfloat mat_specular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// Draw the ground cube with texture coordinates
	glBegin(GL_QUADS);
	// Front face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Back face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	// Top face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	// Bottom face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	// Right face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, 0.5f, -0.5f);
	// Left face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
	glEnd();

	glPopMatrix();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
}

void drawEnv1() {

	// Draw boundary walls around the court
	glColor3f(1.0f, 0.0f, 0.0f);

	//----------------------------

	// Back wall
	glPushMatrix();
	glTranslatef(-3.7f, 1.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(4.5, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.7f, 1.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(4.5, 10.0, 0.2);
	glPopMatrix();

	// Left wall
	glPushMatrix();
	glTranslatef(-6.0f, 1.0f, 0.0f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(27.0, 10.0, 0.2);
	glPopMatrix();

	// Right wall
	glPushMatrix();
	glTranslatef(6.0f, 1.0f, -7.5f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(11.5, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(6.0f, 1.0f, 7.5f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(11.5, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 1.0f, 13.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 10.0, 0.2);
	glPopMatrix();

	//------------------------

	glPushMatrix();
	glTranslatef(21.0f, 1.0f, 0.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(21.0, 10.0, 0.2);
	glPopMatrix();

	// Back wall
	glPushMatrix();
	glTranslatef(15.0f, 1.0f, -10.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(15.0f, 1.0f, 10.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 10.0, 0.2);
	glPopMatrix();

	//--------------------
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -27.0f);
	glPushMatrix();
	glTranslatef(0.0f, 1.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 10.0, 0.2);
	glPopMatrix();

	// Left wall
	glPushMatrix();
	glTranslatef(-6.0f, 1.0f, -3.0f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(21.0, 10.0, 0.2);
	glPopMatrix();

	// Right wall
	glPushMatrix();
	glTranslatef(6.0f, 1.0f, -3.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(21.0, 10.0, 0.2);
	glPopMatrix();
	glPopMatrix();

	//----------------------

	glPushMatrix();
	glTranslatef(-3.8f, 1.0f, -19.6f);  // Adjusted to match the court boundary
	drawWall(4.5, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.8f, 1.0f, -19.6f);  // Adjusted to match the court boundary
	drawWall(4.5, 10.0, 0.2);
	glPopMatrix();

	//_-------------------------

	glPushMatrix();
	glTranslatef(9.0f, 1.0f, -6.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(8.6, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(9.0f, 1.0f, 6.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(8.6, 10.0, 0.2);
	glPopMatrix();

	//------------------------------

	glPushMatrix();
	glTranslatef(7.6f, 1.0f, 1.8f);  // Adjusted to match the court boundary
	drawWall(3.0, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(7.6f, 1.0f, -1.8f);  // Adjusted to match the court boundary
	drawWall(3.0, 10.0, 0.2);
	glPopMatrix();

	//-------------------------------------------

	glPushMatrix();
	glTranslatef(-1.5f, 1.0f, -16.5f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(6.0, 10.0, 0.2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1.5f, 1.0f, -16.5f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(6.0, 10.0, 0.2);
	glPopMatrix();


}

void drawEnv2() {

	drawCourt();

	glColor3f(1.0f, 0.0f, 0.0f);
	// Back wall
	glPushMatrix();
	glTranslatef(0.0f, 3.0f, -13.4f);  // Adjusted to match the court boundary
	drawWall(12.0, 6.0, 0.1);
	glPopMatrix();

	// Left wall
	glPushMatrix();
	glTranslatef(-6.0f, 3.0f, 0.0f);   // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(27.0, 6.0, 0.1);
	glPopMatrix();

	// Right wall
	glPushMatrix();
	glTranslatef(6.0f, 3.0f, 0.0f);    // Adjusted to match the court boundary
	glRotatef(90, 0, 1, 0);
	drawWall(27.0, 6.0, 0.1);
	glPopMatrix();

}

void resetPlayerPosition() {
	if (reachedF2) {
		//playerX = -3.0f;
		//playerZ = 10.0f;
		//playerAngle = 0.0f;
		playerY = 6.2f;
	}
	if (reachedF3) {
		//playerX = -3.0f;
		//playerZ = 10.0f;
		//playerAngle = 0.0f;
		playerY = 12.4f;

	}

}

void setupLights() {
	// Global ambient light: set to darkness
	GLfloat globalAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

	// Spotlight setup for the player (GL_LIGHT1)
	GLfloat playerLightDiffuse[] = { 9.0f, 9.0f, 9.0f, 1.0f }; // High intensity
	GLfloat playerLightSpecular[] = { 9.0f, 9.0f, 9.0f, 1.0f };
	GLfloat playerLightAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // No ambient light for the spotlight
	glLightfv(GL_LIGHT1, GL_DIFFUSE, playerLightDiffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, playerLightSpecular);
	glLightfv(GL_LIGHT1, GL_AMBIENT, playerLightAmbient);

	// Spotlight parameters
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 30.0f);   // Narrow cone angle
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 500.0f); // Focused beam for a bright center

	glEnable(GL_LIGHT1);

	// PLAYING environment light (GL_LIGHT0)
	if (currentGameState == PLAYING) {
		GLfloat playingLightDiffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // Moderate intensity
		GLfloat playingLightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Slight ambient component
		glLightfv(GL_LIGHT0, GL_DIFFUSE, playingLightDiffuse);
		glLightfv(GL_LIGHT0, GL_AMBIENT, playingLightAmbient);
		glEnable(GL_LIGHT0);
	}
	else {
		glDisable(GL_LIGHT0); // Disable PLAYING light in other states
	}

	// LEVEL2_PLAYING environment light (GL_LIGHT2)
	if (currentGameState == LEVEL2_PLAYING) {
		static bool highIntensity = true; // Alternating state

		GLfloat level2LightDiffuse[4];
		if (highIntensity) {
			level2LightDiffuse[0] = 0.5f; // Reddish light with moderate intensity
			level2LightDiffuse[1] = 0.1f;
			level2LightDiffuse[2] = 0.1f;
		}
		else {
			level2LightDiffuse[0] = 0.2f; // Dimmer reddish light
			level2LightDiffuse[1] = 0.05f;
			level2LightDiffuse[2] = 0.05f;
		}
		level2LightDiffuse[3] = 1.0f; // Alpha

		GLfloat level2LightAmbient[] = { 0.1f, 0.05f, 0.05f, 1.0f }; // Minimal ambient red
		glLightfv(GL_LIGHT2, GL_DIFFUSE, level2LightDiffuse);
		glLightfv(GL_LIGHT2, GL_AMBIENT, level2LightAmbient);

		highIntensity = !highIntensity; // Toggle intensity state for next frame
		glEnable(GL_LIGHT2);
	}
	else {
		glDisable(GL_LIGHT2); // Disable LEVEL2 light in other states
	}
}



void toggleLevel2Light(int value) {
	if (currentGameState == LEVEL2_PLAYING) {
		setupLights(); // Re-setup lights to update alternating intensity
	}
	glutTimerFunc(500, toggleLevel2Light, 0); // Toggle every 500ms
}



void setupCamera() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, 1.0, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera.look();
}


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
float zz = 3.0f;

void updateCameraPosition() {
	if (followPlayer) {
		// Set the camera's position to the player's head position
		camera.setView(playerX, playerY + 3.8f, playerZ, playerX - sin(DEG2RAD(playerAngle)), playerY + 3.4f, playerZ - cos(DEG2RAD(playerAngle)));
	}
}

bool followPlayer2 = false; // Flag to indicate if the camera should follow the player
void updateCameraPosition2() {
	if (followPlayer2) {
		// Set the camera's position to the player's head position

		camera.setView(playerX, playerY + 5.2f, playerZ +zz , playerX - sin(DEG2RAD(playerAngle)), playerY+3.1f, playerZ - cos(DEG2RAD(playerAngle)));
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

void drawCoinCollectibles() {
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
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
	if (currentGameState == LEVEL2_PLAYING) {
		coin18.draw(time);
		coin19.draw(time);
		coin20.draw(time);
		coin21.draw(time);
		coin22.draw(time);
	}
}

void checkCoinCollision() {
	coin1.checkCollision(playerX, playerY, playerZ);
	coin2.checkCollision(playerX, playerY, playerZ);
	coin3.checkCollision(playerX, playerY, playerZ);
	coin4.checkCollision(playerX, playerY, playerZ);
	coin5.checkCollision(playerX, playerY, playerZ);
	coin6.checkCollision(playerX, playerY, playerZ);
	coin7.checkCollision(playerX, playerY, playerZ);
	coin8.checkCollision(playerX, playerY, playerZ);
	coin9.checkCollision(playerX, playerY, playerZ);
	coin10.checkCollision(playerX, playerY, playerZ);
	coin11.checkCollision(playerX, playerY, playerZ);
	coin12.checkCollision(playerX, playerY, playerZ);
	coin13.checkCollision(playerX, playerY, playerZ);
	coin14.checkCollision(playerX, playerY, playerZ);
	coin15.checkCollision(playerX, playerY, playerZ);
	coin16.checkCollision(playerX, playerY, playerZ);
	coin17.checkCollision(playerX, playerY, playerZ);
	if (currentGameState == LEVEL2_PLAYING) {
		coin18.checkCollision(playerX, playerY, playerZ);
		coin19.checkCollision(playerX, playerY, playerZ);
		coin20.checkCollision(playerX, playerY, playerZ);
		coin21.checkCollision(playerX, playerY, playerZ);
		coin22.checkCollision(playerX, playerY, playerZ);
	}
}

void drawTraps() {
	float timeTrap = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
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
	//trap15.draw(timeTrap);
	trap16.draw(timeTrap);
	trap17.draw(timeTrap);
	trap18.draw(timeTrap);
	trap19.draw(timeTrap);
	trap20.draw(timeTrap);
	trap21.draw(timeTrap);
	trap22.draw(timeTrap);
	trap23.draw(timeTrap);
	trap24.draw(timeTrap);
	trap25.draw(timeTrap);
	trap26.draw(timeTrap);
	trap27.draw(timeTrap);
	trap28.draw(timeTrap);
	trap29.draw(timeTrap);
	trap30.draw(timeTrap);
	trap31.draw(timeTrap);
	trap32.draw(timeTrap);
	trap33.draw(timeTrap);
	trap34.draw(timeTrap);
	trap35.draw(timeTrap);
	trap36.draw(timeTrap);
	trap37.draw(timeTrap);
	trap38.draw(timeTrap);
	trap39.draw(timeTrap);
	trap40.draw(timeTrap);
	trap41.draw(timeTrap);
	trap42.draw(timeTrap);
	trap43.draw(timeTrap);
	trap44.draw(timeTrap);
	trap45.draw(timeTrap);
}

void checkTrapsCollision() {
	trap1.checkCollision(playerX, playerY, playerZ);
	trap2.checkCollision(playerX, playerY, playerZ);
	trap3.checkCollision(playerX, playerY, playerZ);
	trap4.checkCollision(playerX, playerY, playerZ);
	trap5.checkCollision(playerX, playerY, playerZ);
	trap6.checkCollision(playerX, playerY, playerZ);
	trap7.checkCollision(playerX, playerY, playerZ);
	trap8.checkCollision(playerX, playerY, playerZ);
	trap9.checkCollision(playerX, playerY, playerZ);
	trap10.checkCollision(playerX, playerY, playerZ);
	trap11.checkCollision(playerX, playerY, playerZ);
	trap12.checkCollision(playerX, playerY, playerZ);
	trap13.checkCollision(playerX, playerY, playerZ);
	trap14.checkCollision(playerX, playerY, playerZ);
	trap15.checkCollision(playerX, playerY, playerZ);
	trap16.checkCollision(playerX, playerY, playerZ);
	trap17.checkCollision(playerX, playerY, playerZ);
	trap18.checkCollision(playerX, playerY, playerZ);
	trap19.checkCollision(playerX, playerY, playerZ);
	trap20.checkCollision(playerX, playerY, playerZ);
	trap21.checkCollision(playerX, playerY, playerZ);
	trap22.checkCollision(playerX, playerY, playerZ);
	trap23.checkCollision(playerX, playerY, playerZ);
	trap24.checkCollision(playerX, playerY, playerZ);
	trap25.checkCollision(playerX, playerY, playerZ);
	trap26.checkCollision(playerX, playerY, playerZ);
	trap27.checkCollision(playerX, playerY, playerZ);
	trap28.checkCollision(playerX, playerY, playerZ);
	trap29.checkCollision(playerX, playerY, playerZ);
	trap30.checkCollision(playerX, playerY, playerZ);
	trap31.checkCollision(playerX, playerY, playerZ);
	trap32.checkCollision(playerX, playerY, playerZ);
	trap33.checkCollision(playerX, playerY, playerZ);
	trap34.checkCollision(playerX, playerY, playerZ);
	trap35.checkCollision(playerX, playerY, playerZ);
	trap36.checkCollision(playerX, playerY, playerZ);
	trap37.checkCollision(playerX, playerY, playerZ);
	trap38.checkCollision(playerX, playerY, playerZ);
	trap39.checkCollision(playerX, playerY, playerZ);
	trap40.checkCollision(playerX, playerY, playerZ);
	trap41.checkCollision(playerX, playerY, playerZ);
	trap42.checkCollision(playerX, playerY, playerZ);
	trap43.checkCollision(playerX, playerY, playerZ);
	trap44.checkCollision(playerX, playerY, playerZ);
	trap45.checkCollision(playerX, playerY, playerZ);

}

void checkKey() {
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	if (score >= 500) {
		key1.draw(time);
		key1.checkCollision(playerX, playerZ);
	}
	if (key1.collected) {
		//currentGameState = GAME_WON;
	}
}

void checkMap() {
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	if (score == 500) {
		map1.draw(time);
		map1.checkCollision(playerX, playerY, playerZ);
	}
}

void checkGem() {
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	if (score == 1000) {
		gem1.draw(time);
		gem1.checkCollision(playerX, playerY, playerZ);
	}
}


void drawCrates() {
	crate1.draw();
	crate2.draw();
	crate3.draw();
	crate4.draw();
	crate5.draw();
	crate6.draw();
	crate7.draw();
	crate8.draw();
	crate9.draw();
	crate10.draw();
	crate11.draw();
	crate12.draw();
	crate13.draw();
	crate14.draw();
	crate15.draw();
	crate16.draw();
	crate17.draw();
	crate18.draw();
	crate19.draw();
	crate20.draw();
	crate21.draw();
	crate22.draw();
	crate23.draw();
	crate24.draw();
	crate25.draw();
	crate26.draw();
	crate27.draw();
	crate28.draw();
	crate29.draw();
	if (currentGameState == LEVEL2_PLAYING) {
		crate30.draw();
		crate31.draw();
		crate32.draw();
		crate33.draw();
		crate34.draw();
		crate35.draw();
		crate36.draw();
		crate37.draw();
		crate38.draw();
		crate39.draw();
		crate40.draw();
		crate41.draw();
		crate42.draw();
		crate43.draw();
		crate44.draw();
		crate45.draw();
		crate46.draw();
		crate47.draw();
		crate48.draw();
		crate49.draw();
		crate50.draw();
		crate51.draw();
		crate52.draw();
		crate53.draw();
		crate54.draw();
		crate55.draw();
		crate56.draw();
		crate57.draw();
		crate58.draw();
	}


}

void drawBarrels() {
	barrel1.draw();
	barrel2.draw();
	barrel3.draw();
	barrel4.draw();
	barrel5.draw();
	barrel6.draw();
	barrel7.draw();
	barrel8.draw();
	barrel9.draw();
	barrel10.draw();
	barrel11.draw();
	barrel12.draw();
	barrel13.draw();
	barrel14.draw();
	barrel15.draw();
	barrel16.draw();
	barrel17.draw();
	barrel18.draw();
	barrel19.draw();
	barrel20.draw();
	barrel21.draw();
	barrel22.draw();
	barrel23.draw();
	barrel24.draw();
	barrel25.draw();
	barrel26.draw();
	barrel27.draw();
	barrel28.draw();
	barrel29.draw();
	barrel30.draw();
	barrel31.draw();
	barrel32.draw();
	barrel33.draw();
	barrel34.draw();
	barrel35.draw();
	barrel36.draw();
	barrel37.draw();
	barrel38.draw();
	barrel39.draw();
	barrel40.draw();
	barrel41.draw();

}

bool checkAllCratesBarrelsCollisions(float newPlayerX, float newPlayerZ) {
	std::vector<Crate> crates = { crate1, crate2, crate3, crate4, crate5, crate6, crate7, crate8, crate9, crate10, crate11, crate12, crate13, crate14, crate15, crate16, crate17, crate18, crate19, crate20, crate21, crate22, crate23, crate24, crate25, crate26, crate27, crate28, crate29 };
	std::vector<Barrel> barrels = { barrel1, barrel2, barrel3, barrel4, barrel5, barrel6, barrel7, barrel8, barrel9, barrel10, barrel11, barrel12, barrel13, barrel14, barrel15, barrel16, barrel17, barrel18, barrel19, barrel20, barrel21, barrel22, barrel23, barrel24, barrel25, barrel26, barrel27, barrel28, barrel29, barrel30, barrel31, barrel32, barrel33, barrel34, barrel35, barrel36, barrel37, barrel38, barrel39, barrel40, barrel41 };

	for (const auto& crate : crates) {
		if (crate.checkCollision(newPlayerX, newPlayerZ, 0.5f)) {
			return true;
		}
	}

	for (const auto& barrel : barrels) {
		if (barrel.checkCollision(newPlayerX, newPlayerZ, 0.5f)) {
			return true;
		}
	}

	return false;
}

float startX = playerX;
float startZ = playerZ;

void updateJump() {
	if (!isJumping) return;

	float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	float elapsedTime = currentTime - jumpStartTime;

	// Check if the jump duration has been exceeded
	if (elapsedTime > jumpDuration) {
		isJumping = false;
		playerY = -0.3f; // Reset to ground level
		return;
	}

	// Calculate the normalized time (0 to 1) within the jump duration
	float t = elapsedTime / jumpDuration;

	// Parabolic equation for jump (h = a * t * (1 - t)), scaled by jumpHeight
	playerY = -0.3f + jumpHeight * (4.0f * t * (1.0f - t));
	float totalMovement = 5.5f;

	float movement = totalMovement * t;
	float newPlayerX = playerX;
	float newPlayerZ = playerZ;

	if (playerAngle == 0.0f) {
		newPlayerZ = startZ - movement; // Move -Z
	}
	else if (playerAngle == 180.0f) {
		newPlayerZ = startZ + movement; // Move +Z
	}
	else if (playerAngle == -90.0f) {
		newPlayerX = startX + movement; // Move +X
	}
	else if (playerAngle == 90.0f) {
		newPlayerX = startX - movement; // Move -X
	}

	// Check if the new position is within the allowed boundaries
	if (((newPlayerX >= -5.6f && newPlayerX <= 5.6f && newPlayerZ >= -13.3f && newPlayerZ <= 13.3f) || // Main room
		(newPlayerX >= 5.6f && newPlayerX <= 9.2f && newPlayerZ >= -1.5f && newPlayerZ <= 1.5f) || // Corridor 1
		(newPlayerX >= 9.2f && newPlayerX <= 20.8f && newPlayerZ >= -10.2f && newPlayerZ <= 10.2f) || // Room 1
		(newPlayerX >= -1.3f && newPlayerX <= 1.3f && newPlayerZ >= -19.7f && newPlayerZ <= -13.2f) || // Corridor 2
		(newPlayerX >= -5.7f && newPlayerX <= 5.7f && newPlayerZ >= -40.3f && newPlayerZ <= -19.7f)) && // Room 2
		!checkAllCratesBarrelsCollisions(newPlayerX, newPlayerZ) && currentGameState == PLAYING) {

		playerX = newPlayerX;
		playerZ = newPlayerZ;
	}
	updateCameraPosition();	
	updateCameraPosition2();

	// Redraw the scene
	glutPostRedisplay();
}


void Display() {
	int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
	int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//currentGameState = LEVEL2_START;

	switch (currentGameState) {
	case PLAYING:
		setupCamera();
		setupLights();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawCourt();
		drawRoom();
		drawRoom2();
		drawCoridor1();
		drawCoridor2();
		drawPlayer();
		drawEnv1();
		drawCrates();
		drawBarrels();

		drawCoinCollectibles();
		checkCoinCollision();
		drawTraps();
		checkTrapsCollision();

		updateJump();

		/*glPushMatrix();
		glTranslatef(1.1, 1, 10);
		glScalef(10.1, 10.1, 10.1);
		model_printer.Draw();
		glPopMatrix();*/

		/*glPushMatrix();
		glTranslatef(2, 1, 10);
		//glScalef(0.02, 0.02, 0.02);
		model_crate.Draw();
		glPopMatrix();*/

		glPushMatrix();
		//glTranslatef(2, 1, 10);
		//glScalef(0.02, 0.02, 0.02);
		door.draw();
		glPopMatrix();

		checkKey();

		char scoreText[20];
		sprintf(scoreText, "Score: %d", score);  // Format score as string
		renderText(0.8f, 0.9f, scoreText);  // Adjust the position of the score
		displayTimer();
		//door.isOpen = true;
		renderText("Score: " + intToString(score), windowWidth - 100, windowHeight - 20);
		renderText("Health: " + intToString(playerHealth), windowWidth - 250, windowHeight - 20);
		if (score < 1000) {
			renderText("You need to collect  " + intToString(1000 - score) + " to find key", windowWidth - 750, windowHeight - 20);
		}
		else {
			renderText("You have collected enough coins to find the key", windowWidth - 750, windowHeight - 20);
		}

		if (gameLost) {
			currentGameState = GAME_LOST;
		}
		//key1.collected = true;
		if (door.isOpen) {
			Vector3f playerpos = Vector3f(playerX, playerY, playerZ);
			if (playerX >= 0.4f && playerX <= 2.2f && playerZ >= -40.0f && playerZ <= -37.9f) {
				currentGameState = LEVEL2_START;
			}
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

	case LEVEL2_START:

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0f, 0.0f, 0.0f);
		renderText("LEVEL 1 COMPLETE", windowWidth / 2 - 50, windowHeight / 2);
		renderText("Score: " + std::to_string(score), windowWidth / 2 - 50, windowHeight / 2 - 20);
		renderText("Click to Start level 2", windowWidth / 2 - 50, windowHeight / 2 - 40);
		score = 0;
		break;

	case LEVEL2_PLAYING:
		setupCamera();
		setupLights();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawEnv2();

		glPushMatrix();
		glTranslatef(0.0f, 6.2f, 0.0f);
		drawEnv2();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(0.0f, 12.4f, 0.0f);
		drawEnv2();
		glPopMatrix();

		drawPlayer();
		resetPlayerPosition();
		drawCrates();
		//drawBarrels();
		drawCoinCollectibles();
		checkCoinCollision();
		checkMap();
		checkGem();
		//drawTraps();
		//checkTrapsCollision();

		/*glPushMatrix();
		glTranslatef(0.0, 15.4, 3.9);
		glScalef(10.02, 10.02, 10.02);
		glRotatef(90.0f, 0, 1, 0);
		model_door1.Draw();
		glPopMatrix();*/

		glPushMatrix();
		//glTranslatef(2, 1, 10);
		//glScalef(0.02, 0.02, 0.02);
		door2.draw();
		glPopMatrix();

		float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Convert milliseconds to seconds
		float deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;

		swingingTrap1.update(deltaTime);
		swingingTrap1.draw();
		swingingTrap1.checkCollision(playerX, playerY, playerZ);

		swingingTrap2.update(deltaTime);
		swingingTrap2.draw();
		swingingTrap2.checkCollision(playerX, playerY, playerZ);

		swingingTrap3.update(deltaTime);
		swingingTrap3.draw();
		swingingTrap3.checkCollision(playerX, playerY, playerZ);

		swingingTrap4.update(deltaTime);
		swingingTrap4.draw();
		swingingTrap4.checkCollision(playerX, playerY, playerZ);

		renderText("Score: " + intToString(score), windowWidth - 100, windowHeight - 20);
		renderText("Health: " + intToString(playerHealth), windowWidth - 250, windowHeight - 20);
		if (score < 500) {
			reachedF2 = false;
			reachedF3 = false;
			renderText("You need to collect " + intToString(500 - score) + " to reach floor 2", windowWidth - 750, windowHeight - 20);
		}
		if (score >= 500 && score < 1000 && map1.collected) {
			reachedF2 = true;
			reachedF3 = false;
			renderText("You are on floor 2", windowWidth - 750, windowHeight - 20);
		}
		if (score == 1000 && gem1.collected) {
			reachedF2 = false;
			reachedF3 = true;
			renderText("You are on final floor", windowWidth - 750, windowHeight - 20);
		}
		if (door2.isOpen) {
			Vector3f playerpos = Vector3f(playerX, playerY, playerZ);
			if (playerX >= -3.7f && playerX <= 1.6f && playerZ >= 1.5f && playerZ <= 5.5f) {
				currentGameState = LEVEL2_START;
			}
		}

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
			key1.collected = false;
		}
		if (currentGameState == LEVEL2_START) {
			currentGameState = LEVEL2_PLAYING;
			score = 0;
			playerHealth = 100;
			gameLost = false;
			resetCoins();
			playerX = -3.0f;
			playerZ = 10.0f;
			key1.collected = false;
		}
		if (currentGameState == PLAYING && !isJumping) {
			isJumping = true;
			jumpStartTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
			std::cout << "i am jumping" << std::endl;
			startX = playerX;
			startZ = playerZ;
		}
	}
}


bool checkAllCratesBarrelsCollisions2(float newPlayerX, float newPlayerZ) {
	std::vector<Crate> crates = { crate1, crate2, crate3, crate4, crate5, crate6, crate7, crate8, crate9, crate10, crate11, crate12, crate13, crate14, crate15, crate16, crate17, crate18, crate19, crate20, crate21, crate22, crate23, crate24, crate25, crate26, crate27, crate28, crate29 };
	for (const auto& crate : crates) {
		if (crate.checkCollision(newPlayerX, newPlayerZ, 0.5f)) {
			return true;
		}
	}


	return false;
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
		zz = 3.0f;
		break;

	case 'j':  // Move backward and face backward (180 degrees)
		playerAngle = 180.0f;  // Face backward
		newPlayerZ += stepSize;  // Move backward in the Z direction
		zz = -3.0f;
		break;

	case 'h':  // Move left and face left (90 degrees)
		playerAngle = 90.0f;  // Face left
		newPlayerX -= stepSize;  // Move left in the X direction
		zz = 3.0f;
		break;

	case 'k':  // Move right and face right (-90 degrees)
		playerAngle = -90.0f;  // Face right
		newPlayerX += stepSize;  // Move right in the X direction
		zz = 3.0f;
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
	if (((newPlayerX >= -5.6f && newPlayerX <= 5.6f && newPlayerZ >= -13.3f && newPlayerZ <= 13.3f) || // Main room
		//(newPlayerX >= 5.6f && newPlayerX <= 9.2f && newPlayerZ >= -1.5f && newPlayerZ <= 1.5f) || // Corridor 1
		(newPlayerX >= 9.2f && newPlayerX <= 20.8f && newPlayerZ >= -10.2f && newPlayerZ <= 10.2f) || // Room 1
		(newPlayerX >= -1.3f && newPlayerX <= 1.3f && newPlayerZ >= -19.7f && newPlayerZ <= -13.2f) || // Corridor 2
		(newPlayerX >= -5.7f && newPlayerX <= 5.7f && newPlayerZ >= -40.3f && newPlayerZ <= -19.7f)) && // Room 2
		!checkAllCratesBarrelsCollisions(newPlayerX, newPlayerZ) && currentGameState == PLAYING) {

		playerX = newPlayerX;
		playerZ = newPlayerZ;
		startX = playerX;
		startZ = playerZ;
		updateCameraPosition();
		updateCameraPosition2();
	}
	else {
		std::cout << "Collision detected! Player cannot move in this direction." << std::endl;
	}
	if (newPlayerX >= courtMinX && newPlayerX <= courtMaxX && newPlayerZ >= courtMinZ && newPlayerZ <= courtMaxZ && !checkAllCratesBarrelsCollisions2(newPlayerX, newPlayerZ) && currentGameState == LEVEL2_PLAYING) {
		// If the new position is valid, update the player's position
		playerX = newPlayerX;
		playerZ = newPlayerZ;
		updateCameraPosition();
		updateCameraPosition2();
		//PlayCollisionSound(".../../../Downloads/Bruh (Sound Effect) 1 second video!.wav");
	}
	else {
		std::cout << "Collision detected! Player cannot move in this direction." << std::endl;
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
	std::cout << "Player Position - X: " << playerX << ", Y: " << playerY << ", Z: " << playerZ << std::endl;
	std::cout << "Player Angle : " << playerAngle << std::endl;
	std::cout << "door level 2 : " << door2.isOpen << std::endl;

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

void LoadAssets()
{

	model_coin.Load("Models/house/uploads_files_233898_50ct.3ds");
	model_swtrap.Load("Models/key/broom 3ds.3DS");
	model_key.Load("Models/key/Key9.3DS");
	model_trap.Load("Models/trap/t.3ds");
	model_crate.Load("Models/house/house.3ds");
	model_player.Load("Models/player/pl.3ds");
	model_door1.Load("Models/door/door1.3ds");
	model_door2.Load("Models/door/d.3ds");
	model_gem.Load("Models/gem/gem.3ds");
	model_map.Load("Models/map/map.3ds");

	tex_crate.Load("Textures/crate.bmp");
	tex_floor.Load("Textures/floor.bmp");
	tex_wall1.Load("Textures/walls11.bmp");
	tex_barell.Load("Textures/woodcrate.bmp");
	tex_wall1.Load("Textures/walls11.bmp");
	tex_wall2.Load("Textures/walls2.bmp");
	tex_floor2.Load("Textures/floor2.bmp");

	loadBMP(&tex, "Textures/sky.bmp", true);
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
	//glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);


	LoadAssets();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);
	// Start the timer
	glutTimerFunc(0, Timer, 0);
	glutTimerFunc(1000, timerCallback, 0);
	glutTimerFunc(16, updateDoor, 0); // Update every 16ms (~60 FPS)
	glutTimerFunc(16, updateDoor2, 0); // Update every 16ms (~60 FPS)
	glutTimerFunc(500, toggleLevel2Light, 0); // Start alternating light for LEVEL2


	glutMainLoop();
	return 0;
}