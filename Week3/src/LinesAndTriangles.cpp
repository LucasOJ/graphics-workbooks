#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>

#include <math.h> 
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <CanvasPoint.h>
#include <Colour.h>

#define WIDTH 600
#define HEIGHT 600

// WEEK 1

std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
	std::vector<float> result;
	float delta = (to - from) / (numberOfValues - 1);
	for(size_t i=0; i<numberOfValues; i++) result.push_back(from + i * delta);
	return result;
}

// WEEK 2

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


// WEEK 3

uint32_t colourToCode(Colour colour){
	return (colour.red << 16) + (colour.green << 8) + colour.blue;
}

std::vector<CanvasPoint> getLinePoints(CanvasPoint from, CanvasPoint to) {
	std::vector<CanvasPoint> points;
	float distX = to.x - from.x;
	float distY = to.y - from.y;
	float numberOfSteps = std::max(abs(distX), abs(distY));
	float xStepSize = distX / numberOfSteps;
	float yStepSize = distY / numberOfSteps;
	for(float i = 0.0; i < numberOfSteps; i++) {
		float x = from.x + (xStepSize * i);
		float y = from.y + (yStepSize * i);
		points.push_back(CanvasPoint(x,y));
	}
	return points;
}

std::vector<CanvasPoint> yAxisInterpolate(CanvasPoint from, CanvasPoint to) {
	std::vector<CanvasPoint> points;
	float distX = to.x - from.x;
	float distY = to.y - from.y;
	float numberOfSteps = abs(distY);
	float xStepSize = distX / numberOfSteps;
	float yStepSize = distY / numberOfSteps;
	for(float i = 0.0; i < numberOfSteps; i++) {
		float x = from.x + (xStepSize * i);
		float y = from.y + (yStepSize * i);
		points.push_back(CanvasPoint(x,y));
	}
	return points;
}

void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour){
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	for(size_t i = 0; i < points.size(); i++) {
		float x = points[i].x;
		float y = points[i].y;
		window.setPixelColour(round(x), round(y), colourToCode(colour));
	}
}

void drawStrokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour){
	drawLine(window, triangle.vertices[0], triangle.vertices[1], colour);
	drawLine(window, triangle.vertices[1], triangle.vertices[2], colour);
	drawLine(window, triangle.vertices[2], triangle.vertices[0], colour);
}

void drawRandomStrokedTriangle(DrawingWindow &window) {
	std::vector<CanvasPoint> verticies;
	for(int i = 0; i < 3; i++){
		int x = rand() % window.width;
		int y = rand() % window.height;
		verticies.push_back(CanvasPoint(x, y));
	}
	CanvasTriangle triangle = CanvasTriangle(verticies[0], verticies[1], verticies[2]);
	Colour colour = Colour(rand() % 256, rand() % 256, rand() % 256);
	drawStrokedTriangle(window, triangle, colour);
}

void sortVerticies(std::array<CanvasPoint, 3UL> &verticies){
	if (verticies[0].y > verticies[1].y){
		std::swap(verticies[0], verticies[1]);
	}
	if (verticies[0].y > verticies[2].y){
		std::swap(verticies[0], verticies[2]);
	}
	if (verticies[1].y > verticies[2].y){
		std::swap(verticies[1], verticies[2]);
	}
}

// PRE-CONDITION: SORTED VERTICIES
CanvasPoint getIntersectionPoint(std::array<CanvasPoint, 3UL> &verticies){
	float yPercentage = (verticies[1].y - verticies[0].y) / (verticies[2].y - verticies[0].y);
	float xDiffVector = verticies[2].x - verticies[0].x;
	float x = verticies[0].x + (yPercentage * xDiffVector);
	return CanvasPoint(x, verticies[1].y);
}


void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour){
	sortVerticies(triangle.vertices);
	
	CanvasPoint intersectionPoint = getIntersectionPoint(triangle.vertices);
	std::cout << "DIAGNOSTICS" << std::endl;

	CanvasPoint top = triangle.vertices[0];
	CanvasPoint middle = triangle.vertices[1];
	CanvasPoint bottom = triangle.vertices[2];

	//REPLACE WITH SWAP
	CanvasPoint leftPoint = middle;
	CanvasPoint rightPoint = intersectionPoint;
	if (rightPoint.x < leftPoint.x){
		std::swap(leftPoint, rightPoint);
	}

	float rightPointX = std::max(middle.x, intersectionPoint.x);
	std::vector<CanvasPoint> leftEdge = yAxisInterpolate(top, leftPoint);
	std::vector<CanvasPoint> rightEdge = yAxisInterpolate(top, rightPoint);
	
	//drawLine(window, intersectionPoint, triangle.vertices[1],colour);
	//drawStrokedTriangle(window, triangle, Colour(255,0,255));
	

	for (size_t i = 0; i < leftEdge.size(); i++) {
		//drawLine(window, leftEdge[i], rightEdge[i], colour);
		window.setPixelColour(round(leftEdge[i].x),round(leftEdge[i].y), colourToCode(Colour(255,255,255)));
	}

	drawStrokedTriangle(window, triangle, Colour(255,0,255));
	


}

// TEMPLATE

void draw(DrawingWindow &window) {
	window.clearPixels();

	//glm::vec3 topLeft(255, 0, 0);        // red 
	//glm::vec3 topRight(0, 0, 255);       // blue 
	//glm::vec3 bottomRight(0, 255, 0);    // green 
	//glm::vec3 bottomLeft(255, 255, 0);   // yellow

	//std::vector<glm::vec3> leftColumn = interpolateThreeElementValues(topLeft, bottomLeft, window.height);
	//std::vector<glm::vec3> rightColumn = interpolateThreeElementValues(topRight, bottomRight, window.height);

	for (size_t y = 0; y < window.height; y++) {
	
		for (size_t x = 0; x < window.width; x++) {
			float alpha = 255;
			float red = 0;
			float green = 0;
			float blue = 0;
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
		else if (event.key.keysym.sym == SDLK_u){
			drawRandomStrokedTriangle(window);
		}
		else if (event.key.keysym.sym == SDLK_f){

			Colour yellow = Colour(255,255,0);
			Colour purple = Colour(255,0,255);
			CanvasPoint v1 = CanvasPoint(200, 10);
			CanvasPoint v2 = CanvasPoint(50, 500);
			CanvasPoint v3 = CanvasPoint(547, 400);
			CanvasTriangle triangle = CanvasTriangle(v1, v2, v3);
			drawFilledTriangle(window, triangle, yellow);
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		update(window);
	
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
