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
#define HEIGHT 320

//class Environment {
//	public:
//		DrawingWindow window;
//		float depthBuffer[WIDTH][HEIGHT] = {0.0};
//};

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

void CHECK(bool assertion, std::string failureMessage, size_t lineNumber) {
	if (!assertion){
		std::cout << "Error on line " << std::to_string(lineNumber) << std::endl;
		std::cout << failureMessage << std::endl;
		exit(EXIT_FAILURE);
	}
}


float interpolatePointDepth(CanvasPoint from, CanvasPoint to, CanvasPoint pointOnLine) {
	CHECK(from.depth > 0, std::to_string(from.depth), 97);


	float percentage = (pointOnLine.x - from.x) / (to.x - from.x);

	//float depthDiff = to.depth - from.depth;
	//return from.depth + (percentage * depthDiff);
	
	//assert(percentage >= 0);
	
	// asserts pointOnLine is on the line between from and to
	// Too sensitive to minute rounding errors?
	// assert(xPercentage == yPercentage);

	float fromDepth = 1 / from.depth;
	float toDepth = 1 / to.depth;
	float depthDiff = toDepth - fromDepth;
	if (depthDiff == 0) {
		return from.depth;
	}
	float interpolatedDepth = fromDepth + (percentage * depthDiff);
	float inverseDepth = 1 / interpolatedDepth;
	assert(inverseDepth > 0);
	return inverseDepth;
}

// LINE DRAWING
void drawLine(
		DrawingWindow &window,
		CanvasPoint from,
		CanvasPoint to,
		Colour colour
	){
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	for(size_t i = 0; i < points.size(); i++) {
		float x = points[i].x;
		float y = points[i].y;
		window.setPixelColour(round(x), round(y), colourToCode(colour));
	}
}

void drawLineWithDepth(
		DrawingWindow &window,
		CanvasPoint from,
		CanvasPoint to,
		Colour colour,
		float depthBuffer[WIDTH][HEIGHT]
	){
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	for(size_t i = 0; i < points.size(); i++) {
		float x = points[i].x;
		float y = points[i].y;
		float depth = interpolatePointDepth(from, to, points[i]);
		int xCoord = round(x);
		int yCoord = round(y);

		if (depthBuffer[xCoord][yCoord] <= depth){
			window.setPixelColour(xCoord, yCoord, colourToCode(colour));
			depthBuffer[xCoord][yCoord] = depth;
		}
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

void drawTopTriangle(
		DrawingWindow &window, 
		CanvasPoint top, 
		CanvasPoint bottomLeftPoint, 
		CanvasPoint bottomRightPoint, 
		Colour colour,
		float depthBuffer[WIDTH][HEIGHT]
	) {
	assert(top.y <= bottomLeftPoint.y);
	assert(top.y <= bottomRightPoint.y);
	assert(bottomLeftPoint.x <= bottomRightPoint.x);

	float verticalSteps = bottomLeftPoint.y - top.y;

	float leftStepDelta = (bottomLeftPoint.x - top.x) / verticalSteps;
	float rightStepDelta = (bottomRightPoint.x - top.x) / verticalSteps;

	float currentLeftX = top.x;
	float currentRightX = top.x;

	for (float y = top.y; y <= bottomLeftPoint.y; y++) {
		CanvasPoint leftPoint = CanvasPoint(currentLeftX, y);
		leftPoint.depth = interpolatePointDepth(top, bottomLeftPoint, leftPoint);
		
		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		rightPoint.depth = interpolatePointDepth(top, bottomRightPoint, rightPoint);

		drawLineWithDepth(window, leftPoint, rightPoint, colour, depthBuffer);

		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawBottomTriangle(
		DrawingWindow &window, 
		CanvasPoint bottom, 
		CanvasPoint topLeftPoint, 
		CanvasPoint topRightPoint,
		Colour colour,
		float depthBuffer[WIDTH][HEIGHT]	
	) {

	assert(topLeftPoint.y <= bottom.y);
	assert(topRightPoint.y <= bottom.y);
	assert(topLeftPoint.x <= topRightPoint.x);

	float verticalSteps = bottom.y - topLeftPoint.y;

	float leftStepDelta = (bottom.x - topLeftPoint.x) / verticalSteps;
	float rightStepDelta = (bottom.x - topRightPoint.x) / verticalSteps;

	float currentLeftX = topLeftPoint.x;
	float currentRightX = topRightPoint.x;

	for (float y = topLeftPoint.y; y <= bottom.y; y++) {
		CanvasPoint leftPoint = CanvasPoint(currentLeftX, y);
		leftPoint.depth = interpolatePointDepth(topLeftPoint, bottom, leftPoint);

		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		rightPoint.depth = interpolatePointDepth(topRightPoint, bottom, rightPoint);

		drawLineWithDepth(window, leftPoint, rightPoint, colour, depthBuffer);

		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawFilledTriangle(
		DrawingWindow &window, 
		CanvasTriangle triangle, 
		Colour colour, 
		float depthBuffer[WIDTH][HEIGHT]
	){	
	sortVerticies(triangle.vertices);

	CanvasPoint top = triangle.vertices[0];
	CanvasPoint middle = triangle.vertices[1];
	CanvasPoint bottom = triangle.vertices[2];
	
	CanvasPoint intersectionPoint = getIntersectionPoint(triangle.vertices);
	intersectionPoint.depth = interpolatePointDepth(top, bottom, intersectionPoint);

	CanvasPoint leftPoint = middle;
	CanvasPoint rightPoint = intersectionPoint;
	if (rightPoint.x < leftPoint.x){
		std::swap(leftPoint, rightPoint);	
	}

	drawTopTriangle(window, top, leftPoint, rightPoint, colour, depthBuffer);
	drawBottomTriangle(window, bottom, leftPoint, rightPoint, colour, depthBuffer);
}

// Week 4

// MTL Parser

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

glm::vec2 vertexToImagePlane(glm::vec3 vertex, float focalLength, glm::vec3 camera) {
	assert(camera.x == 0);
	assert(camera.y == 0);
	assert(focalLength < camera.z);

	float planeScaling = 500;
	float cameraDist = camera.z - vertex.z;
	float u = (focalLength / cameraDist) * vertex.x * planeScaling + WIDTH / 2;
	// negative since y of zero at top of page
	float v = -((focalLength / cameraDist) * vertex.y * planeScaling) + HEIGHT / 2;
	return glm::vec2(u, v);
}

void drawCornellBox(DrawingWindow &window) {
	std::map<std::string, Colour> colourMap = loadMaterialsFromMTL("cornell-box.mtl");

	std::vector<ModelTriangle> triangles = loadFromOBJ("cornell-box.obj", colourMap);

	float depthBuffer[WIDTH][HEIGHT] = {0.0};

	glm::vec3 camera = glm::vec3(0.0, 0.0, 4.0);
	float focalLength = 2.0;

	Colour white = Colour(255,255,255);

	for (int i = 0; i < triangles.size(); i++){
		std::vector<CanvasPoint> verticies;
		for (int j = 0; j < 3; j++){
			glm::vec3 modelVertex = triangles[i].vertices[j];
			glm::vec2 projectedVertex = vertexToImagePlane(modelVertex, focalLength, camera);
			float depth = 1 / (focalLength - modelVertex.z);
			verticies.push_back(CanvasPoint(projectedVertex[0], projectedVertex[1], depth));
		}
		CanvasTriangle triangle = CanvasTriangle(verticies[0], verticies[1], verticies[2]);
		drawFilledTriangle(window, triangle, triangles[i].colour, depthBuffer);
		//drawStrokedTriangle(window, triangle, white);
	}
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
	
	drawCornellBox(window);
	
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
