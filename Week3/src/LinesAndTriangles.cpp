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

#include <TexturePoint.h>
#include <TextureMap.h>

#define WIDTH 320
#define HEIGHT 240

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

	// MAY NEED TO REMOVE
	// Leads to zero delta if left
	if (from.x == to.x && from.y == to.y){
		points.push_back(from);
		return points;
	}

	float distX = to.x - from.x;
	float distY = to.y - from.y;

	// THINK ABOUT NUMBER OF STEPS VS NUMBER OF PIXELS
	// <= vs <

	//Ceil to prevent skipping fractional remainders
	float numberOfSteps = ceil(std::max(abs(distX), abs(distY)));
	float xStepSize = distX / numberOfSteps;
	float yStepSize = distY / numberOfSteps;
	for(float i = 0.0; i <= numberOfSteps; i++) {
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

CanvasTriangle getRandomTriangle(DrawingWindow &window) {
	std::vector<CanvasPoint> verticies;
	for(int i = 0; i < 3; i++){
		int x = rand() % window.width;
		int y = rand() % window.height;
		verticies.push_back(CanvasPoint(x, y));
	}
	CanvasTriangle triangle = CanvasTriangle(verticies[0], verticies[1], verticies[2]);
	return triangle;
}

Colour getRandomColour() {
	return Colour(rand() % 256, rand() % 256, rand() % 256);
}

void drawRandomStrokedTriangle(DrawingWindow &window) {
	CanvasTriangle triangle = getRandomTriangle(window);
	Colour colour = getRandomColour();
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

CanvasPoint getIntersectionPoint(std::array<CanvasPoint, 3UL> &verticies){

	assert(verticies[0].y <= verticies[1].y);
	assert(verticies[1].y <= verticies[2].y);

	float yPercentage = (verticies[1].y - verticies[0].y) / (verticies[2].y - verticies[0].y);
	float xDiffVector = verticies[2].x - verticies[0].x;
	float x = verticies[0].x + (yPercentage * xDiffVector);
	return CanvasPoint(x, verticies[1].y);
}

// FILLED TRIANGLES
void drawTopTriangle(DrawingWindow &window, CanvasPoint top, CanvasPoint leftPoint, CanvasPoint rightPoint, Colour colour) {

	assert(top.y <= leftPoint.y);
	assert(top.y <= rightPoint.y);
	assert(leftPoint.x <= rightPoint.x);

	float verticalSteps = leftPoint.y - top.y;

	float leftStepDelta = (leftPoint.x - top.x) / verticalSteps;
	float rightStepDelta = (rightPoint.x - top.x) / verticalSteps;

	float currentLeftX = top.x;
	float currentRightX = top.x;

	for (float y = top.y; y <= leftPoint.y; y++) {
		CanvasPoint leftPoint = CanvasPoint(currentLeftX, y);
		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		drawLine(window, leftPoint, rightPoint, colour);

		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawBottomTriangle(DrawingWindow &window, CanvasPoint bottom, CanvasPoint leftPoint, CanvasPoint rightPoint, Colour colour) {

	assert(leftPoint.y <= bottom.y);
	assert(rightPoint.y <= bottom.y);
	assert(leftPoint.x <= rightPoint.x);

	float verticalSteps = bottom.y - leftPoint.y;

	float leftStepDelta = (bottom.x - leftPoint.x) / verticalSteps;
	float rightStepDelta = (bottom.x - rightPoint.x) / verticalSteps;

	float currentLeftX = leftPoint.x;
	float currentRightX = rightPoint.x;

	for (float y = leftPoint.y; y <= bottom.y; y++) {
		CanvasPoint leftPoint = CanvasPoint(currentLeftX, y);
		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		drawLine(window, leftPoint, rightPoint, colour);

		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour){
	sortVerticies(triangle.vertices);
	
	CanvasPoint intersectionPoint = getIntersectionPoint(triangle.vertices);

	CanvasPoint top = triangle.vertices[0];
	CanvasPoint middle = triangle.vertices[1];
	CanvasPoint bottom = triangle.vertices[2];

	CanvasPoint leftPoint = middle;
	CanvasPoint rightPoint = intersectionPoint;
	if (rightPoint.x < leftPoint.x){
		std::swap(leftPoint, rightPoint);
	}

	drawTopTriangle(window, top, leftPoint, rightPoint, colour);
	drawBottomTriangle(window, bottom, leftPoint, rightPoint, colour);
}

void drawRandomFilledTriangle(DrawingWindow &window) {
	CanvasTriangle triangle = getRandomTriangle(window);
	Colour colour = getRandomColour();
	drawFilledTriangle(window, triangle, colour);
	drawStrokedTriangle(window,triangle, Colour(255,255,255));
}

// TEXTURE MAP

uint32_t getTexturePixelColour(TextureMap textureMap, float x, float y){
	uint32_t index = round(y) * textureMap.width + round(x);
	return textureMap.pixels[index];
} 

TexturePoint interpolateIntoTextureMap(CanvasPoint from, CanvasPoint to, CanvasPoint pointOnLine) {
	float xPercentage = (pointOnLine.x - from.x) / (to.x - from.x);
	float yPercentage = (pointOnLine.x - from.x) / (to.x - from.x);
	
	// asserts pointOnLine is on the line between from and to
	// Too sensitive to minute rounding errors?
	// assert(xPercentage == yPercentage);

	float texturePointXDiff = to.texturePoint.x - from.texturePoint.x;
	float texturePointYDiff = to.texturePoint.y - from.texturePoint.y;
	float interpolatedPointX = from.texturePoint.x + (xPercentage * texturePointXDiff);
	float interpolatedPointY = from.texturePoint.y + (yPercentage * texturePointYDiff);

	return TexturePoint(interpolatedPointX, interpolatedPointY);
}

void drawTextureLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, TextureMap textureMap) {
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	for(size_t i = 0; i < points.size(); i++) {
		float x = points[i].x;
		float y = points[i].y;
		TexturePoint texturePoint = interpolateIntoTextureMap(from, to, points[i]);
		uint32_t colourCode = getTexturePixelColour(textureMap, texturePoint.x, texturePoint.y);
		window.setPixelColour(round(x), round(y), colourCode);
	}
}

void drawTopTextureTriangle(
	DrawingWindow &window, 
	CanvasPoint top, 
	CanvasPoint bottomLeftPoint, 
	CanvasPoint bottomRightPoint,
	TextureMap textureMap) {

	assert(top.y <= bottomLeftPoint.y);
	assert(top.y <= bottomRightPoint.y);
	assert(bottomLeftPoint.x <= bottomRightPoint.x);
	assert(bottomLeftPoint.y == bottomRightPoint.y);

	float verticalSteps = bottomLeftPoint.y - top.y;

	float leftStepDelta = (bottomLeftPoint.x - top.x) / verticalSteps;
	float rightStepDelta = (bottomRightPoint.x - top.x) / verticalSteps;

	float currentLeftX = top.x;
	float currentRightX = top.x;

	for (float y = top.y; y <= bottomLeftPoint.y; y++) {
		CanvasPoint leftPoint = CanvasPoint(currentLeftX, y);
		leftPoint.texturePoint = interpolateIntoTextureMap(top, bottomLeftPoint, leftPoint);

		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		rightPoint.texturePoint = interpolateIntoTextureMap(top, bottomRightPoint, rightPoint);

		drawTextureLine(window, leftPoint, rightPoint, textureMap);

		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawBottomTextureTriangle(
	DrawingWindow &window, 
	CanvasPoint bottom, 
	CanvasPoint topLeftPoint, 
	CanvasPoint topRightPoint,
	TextureMap textureMap) {

	assert(topLeftPoint.y <= bottom.y);
	assert(topRightPoint.y <= bottom.y);
	assert(topLeftPoint.x <= topRightPoint.x);
	assert(topLeftPoint.y == topRightPoint.y);

	float verticalSteps = bottom.y - topLeftPoint.y;

	float leftStepDelta = (bottom.x - topLeftPoint.x) / verticalSteps;
	float rightStepDelta = (bottom.x - topRightPoint.x) / verticalSteps;

	float currentLeftX = topLeftPoint.x;
	float currentRightX = topRightPoint.x;

	for (float y = topLeftPoint.y; y <= bottom.y; y++) {
		CanvasPoint leftPoint = CanvasPoint(currentLeftX, y);
		leftPoint.texturePoint = interpolateIntoTextureMap(topLeftPoint, bottom, leftPoint);
		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		rightPoint.texturePoint = interpolateIntoTextureMap(topRightPoint, bottom, rightPoint);
		drawTextureLine(window, leftPoint, rightPoint, textureMap);
		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawTextureMapTriangle(DrawingWindow &window){

	TextureMap textureMap = TextureMap("texture.ppm");

	CanvasPoint v1 = CanvasPoint(160, 10);
	v1.texturePoint = TexturePoint(195,5);
	CanvasPoint v2 = CanvasPoint(300, 230);
	v2.texturePoint = TexturePoint(395, 380);
	CanvasPoint v3 = CanvasPoint(10, 150);
	v3.texturePoint = TexturePoint(65, 330);

	CanvasTriangle triangle = CanvasTriangle(v1, v2, v3);

	sortVerticies(triangle.vertices);
	CanvasPoint top = triangle.vertices[0];
	CanvasPoint middle = triangle.vertices[1];
	CanvasPoint bottom = triangle.vertices[2];

	CanvasPoint intersectionPoint = getIntersectionPoint(triangle.vertices);
	intersectionPoint.texturePoint = interpolateIntoTextureMap(top, bottom, intersectionPoint);

	CanvasPoint leftPoint = middle;
	CanvasPoint rightPoint = intersectionPoint;
	if (rightPoint.x < leftPoint.x){
		std::swap(leftPoint, rightPoint);
	}

	drawTopTextureTriangle(window, top, leftPoint, rightPoint, textureMap);
	drawBottomTextureTriangle(window, bottom, leftPoint, rightPoint, textureMap);
	drawStrokedTriangle(window, triangle, Colour(255,255,255));
}

// TEMPLATE

void draw(DrawingWindow &window) {
	window.clearPixels();

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
			drawRandomFilledTriangle(window);
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	
	drawTextureMapTriangle(window);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		update(window);
	
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
