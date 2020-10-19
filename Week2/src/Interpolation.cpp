#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <math.h> 
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define WIDTH 320
#define HEIGHT 240

std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
	std::vector<float> result;
	float delta = (to - from) / (numberOfValues - 1);
	for(size_t i=0; i<numberOfValues; i++) result.push_back(from + i * delta);
	return result;
}

std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues){
	std::vector<float> channel1 = interpolateSingleFloats(from[0], to[0], numberOfValues);
	std::vector<float> channel2 = interpolateSingleFloats(from[1], to[1], numberOfValues);
	std::vector<float> channel3 = interpolateSingleFloats(from[2], to[2], numberOfValues);
	std::vector<glm::vec3> result;
	for(size_t i=0; i<numberOfValues; i++) {
		glm::vec3 value(channel1[i], channel2[i], channel3[i]);
		result.push_back(value);
	}
	return result;
}

void draw(DrawingWindow &window) {
	window.clearPixels();

	glm::vec3 topLeft(255, 0, 0);        // red 
	glm::vec3 topRight(0, 0, 255);       // blue 
	glm::vec3 bottomRight(0, 255, 0);    // green 
	glm::vec3 bottomLeft(255, 255, 0);   // yellow

	std::vector<glm::vec3> leftColumn = interpolateThreeElementValues(topLeft, bottomLeft, window.height);
	std::vector<glm::vec3> rightColumn = interpolateThreeElementValues(topRight, bottomRight, window.height);

	for (size_t y = 0; y < window.height; y++) {
		std::vector<glm::vec3> row = interpolateThreeElementValues(leftColumn[y], rightColumn[y], window.width);
		for (size_t x = 0; x < window.width; x++) {
			float alpha = 255;
			float red = floor(row[x][0]);
			float green = floor(row[x][1]);
			float blue = floor(row[x][2]);
			uint32_t colour = (int(alpha) << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

void update(DrawingWindow &window) {
	// Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		update(window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
