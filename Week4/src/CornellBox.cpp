#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <map>

#include <math.h> 
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <CanvasPoint.h>
#include <Colour.h>

#include <ModelTriangle.h>

#define WIDTH 320
#define HEIGHT 240

float SCALING_FACTOR = 0.17;

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

// UTILS

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

// LINE DRAWING

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

// FILLED TRIANGLES
// ADD ASSERTIONS

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

// Week 4

std::map<std::string, Colour> loadMaterialsFromMTL(std::string filename) {
	std::ifstream fileStream = std::ifstream(filename);
	std::map<std::string, Colour> colourMap;
	std::string line;
	std::string colourName;
	while(std::getline(fileStream, line)){
		std::vector<std::string> substrs = split(line, ' ');
		
		if (substrs[0] == "newmtl"){
			colourName = substrs[1];
		}

		if (substrs[0] == "Kd"){
			std::vector<int> colourComponents;
			for (int i = 1; i <= 3; i++) {
				int component = round(std::stof(substrs[i]) * 255);
				colourComponents.push_back(component);
			}
			Colour colour = Colour(
				colourName,
				colourComponents[0], 
				colourComponents[1], 
				colourComponents[2]
			);
			colourMap.insert({ colourName, colour });
		}
	}
	return colourMap;
}

// OBJ Parser

float formatVertexComponent(std::string str) {
	return std::stof(str) * SCALING_FACTOR;
}

glm::vec3 getVertex(std::string line, std::vector<glm::vec3> verticies, int vertexNum) {
	std::string vertexString = split(line, ' ')[vertexNum];
	int vertexIndex = std::stoi(split(vertexString, '/')[0]);
	return verticies[vertexIndex - 1];
}

std::vector<ModelTriangle> loadFromOBJ(
		std::string filename,
		std::map<std::string, Colour> colourMap
	) {
	std::vector<ModelTriangle> modelTriangles;
	std::ifstream fileStream = std::ifstream(filename);
	std::string line;
	std::vector<glm::vec3> verticies;
	Colour colour;
	while(std::getline(fileStream, line)){
		std::vector<std::string> substrs = split(line, ' ');

		if (substrs[0] == "usemtl") {
			colour = colourMap[substrs[1]];
		}

		if (substrs[0] == "v") {
			glm::vec3 point = glm::vec3(
				formatVertexComponent(substrs[1]), 
				formatVertexComponent(substrs[2]), 
				formatVertexComponent(substrs[3])
			);
			verticies.push_back(point);
		}

		if (substrs[0] == "f") {
			ModelTriangle triangle = ModelTriangle(
				getVertex(line, verticies, 1),
				getVertex(line, verticies, 2),
				getVertex(line, verticies, 3),
				colour
			);
			modelTriangles.push_back(triangle);
		}
	}

	fileStream.close();
	return modelTriangles;
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
	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	
	std::map<std::string, Colour> colourMap = loadMaterialsFromMTL("cornell-box.mtl");

	std::vector<ModelTriangle> triangles = loadFromOBJ("cornell-box.obj", colourMap);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		update(window);
	
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

//for (int i = 0; i < verticies.size(); i++){
//	std::cout << glm::to_string(verticies[i]) << std::endl;
//}
//std::cout << std::endl;
