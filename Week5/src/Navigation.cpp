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

#include <TexturePoint.h>
#include <TextureMap.h>

#include <ModelTriangle.h>


#define WIDTH 320
#define HEIGHT 320

//class Environment {
//	public:
//		DrawingWindow window;
//		float depthBuffer[WIDTH][HEIGHT] = {0.0};
//};

enum MaterialType { TEXTURE, COLOUR };

class Material {
	public:
		MaterialType type;
		Colour colour;
		TextureMap textureMap;
		std::string textureMapName;

		void setColour(Colour newColour){
			colour = newColour;
			type = COLOUR;
		}

		void setTextureMap(TextureMap newTextureMap){
			textureMap = newTextureMap;
			type = TEXTURE;
		}
};

float SCALING_FACTOR = 0.17;

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
	assert(from.depth >= 0);
	assert(to.depth >= 0);

	float depthDiff = to.depth - from.depth;
	
	float percentage = (pointOnLine.x - from.x) / (to.x - from.x);

	// Resolves issues where dividing by a number close to 0 make percentage NaN
	if (!isnormal(percentage)) {
		assert(isnormal(from.depth));
		return from.depth;
	}
	
	float interpolatedDepth = from.depth + (percentage * depthDiff);

	if (interpolatedDepth < 0){
		return from.depth;
		std::cout << "TO.DEPTH " << to.depth << std::endl;
		std::cout << "FROM.DEPTH " << from.depth << std::endl;
		std::cout << "DEPTH DIFF " << depthDiff << std::endl;
		std::cout << "PERCENTAGE " << percentage << std::endl;
		std::cout << "DEPTH " << interpolatedDepth << std::endl;
		std::cout << std::endl;
	}

	//assert(interpolatedDepth >= 0);

	return interpolatedDepth;
}

// LINE DRAWING
bool validCoord(int x, int y) {
	return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT;
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

		if (depthBuffer[xCoord][yCoord] < depth && validCoord(xCoord, yCoord)){
			window.setPixelColour(xCoord, yCoord, colourToCode(colour));
			depthBuffer[xCoord][yCoord] = depth;
		}
	}
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


// TEXTURE FUNCTIONS
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

void drawTextureLine(
		DrawingWindow &window, 
		CanvasPoint from, 
		CanvasPoint to, 
		Material material,
		float depthBuffer[WIDTH][HEIGHT]
	){
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	for(size_t i = 0; i < points.size(); i++) {
		float x = points[i].x;
		float y = points[i].y;
		
		float depth = interpolatePointDepth(from, to, points[i]);
		int xCoord = round(x);
		int yCoord = round(y);

		uint32_t colourCode;
		if (material.type == TEXTURE) {
			TexturePoint texturePoint = interpolateIntoTextureMap(from, to, points[i]);
			colourCode = getTexturePixelColour(material.textureMap, texturePoint.x, texturePoint.y);
		}
		else if (material.type == COLOUR) {
			colourCode = colourToCode(material.colour);
		}

		if (depthBuffer[xCoord][yCoord] < depth && validCoord(xCoord, yCoord)){
			window.setPixelColour(xCoord, yCoord, colourCode);
			depthBuffer[xCoord][yCoord] = depth;
		}
	}
}

void drawTopTextureTriangle(
		DrawingWindow &window, 
		CanvasPoint top, 
		CanvasPoint bottomLeftPoint, 
		CanvasPoint bottomRightPoint,
		Material material,
		float depthBuffer[WIDTH][HEIGHT]
	){

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
		leftPoint.depth = interpolatePointDepth(top, bottomLeftPoint, leftPoint);

		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		rightPoint.depth = interpolatePointDepth(top, bottomRightPoint, rightPoint);

		if (material.type == TEXTURE){
			leftPoint.texturePoint = interpolateIntoTextureMap(top, bottomLeftPoint, leftPoint);
			rightPoint.texturePoint = interpolateIntoTextureMap(top, bottomRightPoint, rightPoint);
		}

		drawTextureLine(window, leftPoint, rightPoint, material, depthBuffer);

		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

void drawBottomTextureTriangle(
		DrawingWindow &window, 
		CanvasPoint bottom, 
		CanvasPoint topLeftPoint, 
		CanvasPoint topRightPoint,
		Material material,
		float depthBuffer[WIDTH][HEIGHT]
	){

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
		leftPoint.depth = interpolatePointDepth(topLeftPoint, bottom, leftPoint);

		CanvasPoint rightPoint = CanvasPoint(currentRightX, y);
		rightPoint.depth = interpolatePointDepth(topRightPoint, bottom, rightPoint);

		if (material.type == TEXTURE) {
			leftPoint.texturePoint = interpolateIntoTextureMap(topLeftPoint, bottom, leftPoint);
			rightPoint.texturePoint = interpolateIntoTextureMap(topRightPoint, bottom, rightPoint);
		}

		drawTextureLine(window, leftPoint, rightPoint, material, depthBuffer);
		currentLeftX += leftStepDelta;
		currentRightX += rightStepDelta;
	}
}

// TODO: COMPRESS TO TAKE MATERIAL NOT COLOUR

void drawTextureMapTriangle(
		DrawingWindow &window, 
		CanvasTriangle triangle, 
		Material material,
		float depthBuffer[WIDTH][HEIGHT]
	){

	sortVerticies(triangle.vertices);
	CanvasPoint top = triangle.vertices[0];
	CanvasPoint middle = triangle.vertices[1];
	CanvasPoint bottom = triangle.vertices[2];

	CanvasPoint intersectionPoint = getIntersectionPoint(triangle.vertices);
	intersectionPoint.depth = interpolatePointDepth(top, bottom, intersectionPoint);

	if (material.type == TEXTURE) {
		intersectionPoint.texturePoint = interpolateIntoTextureMap(top, bottom, intersectionPoint);
	}

	CanvasPoint leftPoint = middle;
	CanvasPoint rightPoint = intersectionPoint;
	if (rightPoint.x < leftPoint.x){
		std::swap(leftPoint, rightPoint);
	}

	drawTopTextureTriangle(window, top, leftPoint, rightPoint, material, depthBuffer);
	drawBottomTextureTriangle(window, bottom, leftPoint, rightPoint, material, depthBuffer);
}


// Week 4

// MTL Parser

std::map<std::string, Material> loadMaterialsFromMTL(std::string filename) {
	std::ifstream fileStream = std::ifstream(filename);
	std::map<std::string, Material> materialMap;
	std::string line;
	std::string name;
	std::string textureMapFilename;
	while(std::getline(fileStream, line)){
		std::vector<std::string> substrs = split(line, ' ');
		
		if (substrs[0] == "newmtl"){
			name = substrs[1];
		}

		if (substrs[0] == "Kd"){
			std::vector<int> colourComponents;
			for (int i = 1; i <= 3; i++) {
				int component = round(std::stof(substrs[i]) * 255);
				colourComponents.push_back(component);
			}
			Colour colour = Colour(
				name,
				colourComponents[0], 
				colourComponents[1], 
				colourComponents[2]
			);
			Material colourMaterial;
			colourMaterial.setColour(colour);
			materialMap[name] = colourMaterial;
		}

		if (substrs[0] == "map_Kd"){
			TextureMap textureMap = TextureMap(substrs[1]);
			Material textureMaterial;
			
			textureMaterial.setTextureMap(textureMap);
			textureMaterial.textureMapName = substrs[1];
			materialMap[name] = textureMaterial;
		}
	}
	return materialMap;
}

// OBJ Parser

float formatVertexComponent(std::string str) {
	return std::stof(str) * SCALING_FACTOR;
}

int objIndexToVertexIndex(std::string objIndex) {
	return std::stoi(objIndex) - 1;
}

std::vector<ModelTriangle> loadFromOBJ(
		std::string filename,
		std::map<std::string, Material> materialMap,
		std::vector<Material> &materials
	) {
	std::vector<ModelTriangle> modelTriangles;
	std::ifstream fileStream = std::ifstream(filename);
	std::string line;
	std::vector<glm::vec3> verticies;
	std::vector<TexturePoint> texturePoints;
	bool materialSet;
	Material material;
	while(std::getline(fileStream, line)){
		materialSet = false;
		std::vector<std::string> substrs = split(line, ' ');

		if (substrs[0] == "usemtl") {
			material = materialMap[substrs[1]];
		}

		if (substrs[0] == "v") {
			glm::vec3 point = glm::vec3(
				formatVertexComponent(substrs[1]), 
				formatVertexComponent(substrs[2]), 
				formatVertexComponent(substrs[3])
			);
			verticies.push_back(point);
		}

		if (substrs[0] == "vt") {
			float x = fmod(std::stof(substrs[1]), 1) * material.textureMap.width;
			float y = fmod(std::stof(substrs[2]), 1) * material.textureMap.height;
			float flippedY = material.textureMap.height - y;
			TexturePoint texturePoint = TexturePoint(x, flippedY);

			texturePoints.push_back(texturePoint);
		}

		if (substrs[0] == "f") {
			ModelTriangle triangle = ModelTriangle();
			std::vector<std::string> lineComponents = split(line, ' ');

			for (size_t i = 1; i < lineComponents.size(); i++){
				size_t index = i - 1;
				
				// SETS MODEL POINTS
				std::vector<std::string> vertexIndexes = split(lineComponents[i], '/');
				int modelVertexIndex = objIndexToVertexIndex(vertexIndexes[0]);
				triangle.vertices[index] = verticies[modelVertexIndex];

				// USING COLOUR
				if (vertexIndexes[1] == ""){
					triangle.colour = material.colour;
					if (!materialSet) materials.push_back(material);
				} 
				
				// USING TEXTURE MAP
				else {
					int texturePointIndex = objIndexToVertexIndex(vertexIndexes[1]);
					triangle.texturePoints[index] = texturePoints[texturePointIndex];
					if (!materialSet) materials.push_back(material);
				}
				materialSet = true;
			} 
			modelTriangles.push_back(triangle);
		}

	}

	fileStream.close();
	return modelTriangles;
}

// Cornell box

glm::vec2 vertexToImagePlane(glm::vec3 vertex, float focalLength, glm::vec3 camera) {
	// doesn't work in 3d as is
	//assert(focalLength < camera.z);

	float planeScaling = 500;
	glm::vec3 dist = vertex - camera;

	// negative as zDist given by camera.z - vertex.x??
	dist.z *= -1;

	float u = (focalLength / dist.z) * dist.x * planeScaling + WIDTH / 2;
	// negative since y of zero at top of page
	float v = -((focalLength / dist.z) * dist.y * planeScaling) + HEIGHT / 2;
	return glm::vec2(u, v);
}

void drawCornellBox(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		TextureMap textureMap,
		float depthBuffer[WIDTH][HEIGHT],
		glm::vec3 camera,
		float focalLength
	) {
	for (int i = 0; i < triangles.size(); i++){
		std::vector<CanvasPoint> verticies;
		for (int j = 0; j < 3; j++){
			glm::vec3 modelVertex = triangles[i].vertices[j];
			glm::vec2 projectedVertex = vertexToImagePlane(modelVertex, focalLength, camera);
			float depth = 1 / (focalLength - modelVertex.z);
			CanvasPoint point = CanvasPoint(projectedVertex[0], projectedVertex[1], depth);

			if (materials[i].type == TEXTURE){
				point.texturePoint = triangles[i].texturePoints[j];
			}

			verticies.push_back(point);
		}
		CanvasTriangle triangle = CanvasTriangle(verticies[0], verticies[1], verticies[2]);
		drawTextureMapTriangle(window, triangle, materials[i], depthBuffer);
	}
}

// Transformation

glm::vec3 rotateX(glm::vec3 camera, float theta){
	glm::mat3 rotation = glm::mat3(
		1.0, 0.0, 0.0, // first column
		0.0, cos(theta), sin(theta), // second column
		0.0, -sin(theta), cos(theta) // third column
	);
	return rotation * camera;
}


glm::vec3 rotateY(glm::vec3 camera, float theta){
	glm::mat3 rotation = glm::mat3(
		cos(theta), 0.0, -sin(theta), // first column
		0.0, 1.0, 0.0, // second column
		sin(theta), 0.0, cos(theta) // third column
	);
	return rotation * camera;
}

// EVENT LOOPS

void draw(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		TextureMap textureMap,
		float depthBuffer[WIDTH][HEIGHT],
		glm::vec3 camera,
		float focalLength
	) {
	window.clearPixels();

	for (int i = 0; i < WIDTH; i++) {
		for (int j = 0; j < HEIGHT; j++) {
			depthBuffer[i][j] = 0.0;
		}
	}

	drawCornellBox(
		window,
		triangles,
		materials,
		textureMap,
		depthBuffer,
		camera,
		focalLength
	);
	//CanvasTriangle triag = CanvasTriangle(
	//	CanvasPoint(0.0,0.0,0.1),
	//	CanvasPoint(WIDTH - 1, 0.0, 0.1),
	//	CanvasPoint(0.0, HEIGHT - 1, 0.1)
	//);
	//
	//drawFilledTriangle(window, triag, Colour(255,0,0), depthBuffer);
}

void update(DrawingWindow &window) {
	// Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event, DrawingWindow &window, glm::vec3 &camera) {
	float TRANSLATION_STEP = 0.05;
	float ROTATION_STEP = M_PI * 0.01;
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) camera += glm::vec3(-TRANSLATION_STEP, 0.0, 0.0);
		else if (event.key.keysym.sym == SDLK_RIGHT) camera += glm::vec3(TRANSLATION_STEP, 0.0, 0.0);
		else if (event.key.keysym.sym == SDLK_UP) camera += glm::vec3(0.0, TRANSLATION_STEP, 0.0);
		else if (event.key.keysym.sym == SDLK_DOWN) camera += glm::vec3(0.0, -TRANSLATION_STEP, 0.0);
		else if (event.key.keysym.sym == SDLK_a) camera = rotateY(camera, -ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_d) camera = rotateY(camera, ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_w) camera = rotateX(camera, -ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_s) camera = rotateX(camera, ROTATION_STEP);
	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	std::map<std::string, Material> materialMap = loadMaterialsFromMTL("textured-cornell-box.mtl");
	std::vector<Material> materials;
	std::vector<ModelTriangle> triangles = loadFromOBJ("textured-cornell-box.obj", materialMap, materials);
	TextureMap textureMap = TextureMap("texture.ppm");

	float depthBuffer[WIDTH][HEIGHT];

	glm::vec3 camera = glm::vec3(0.0, 0.0, 8.0);
	float focalLength = 2.0;
	
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camera);
		

	update(window);
		draw(
			window,
			triangles,
			materials,
			textureMap,
			depthBuffer,
			camera,
			focalLength
		);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}

//for (int i = 0; i < verticies.size(); i++){
//	std::cout << glm::to_string(verticies[i]) << std::endl;
//}
//std::cout << std::endl;
