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
#include <RayTriangleIntersection.h>

#include <algorithm>

#define WIDTH 320
#define HEIGHT 320

enum MaterialType { TEXTURE, COLOUR };

class Material {
	public:
		MaterialType type;
		Colour colour;
		TextureMap textureMap;

		void setColour(Colour newColour){
			colour = newColour;
			type = COLOUR;
		}

		void setTextureMap(TextureMap newTextureMap){
			textureMap = newTextureMap;
			type = TEXTURE;
		}
};

class CameraEnvironment {
	public:
		glm::vec3 position;
		glm::mat3 rotation;
		float focalLength;

		void rotateX(float theta) {
			glm::mat3 rotationX = glm::mat3(
				1.0, 0.0, 0.0, 					// first column
				0.0, cos(theta), sin(theta), 	// second column
				0.0, -sin(theta), cos(theta) 	// third column
			);
			rotation = rotation * rotationX;
		}

		void rotateY(float theta) {
			glm::mat3 rotationY = glm::mat3(
				cos(theta), 0.0, -sin(theta), 	// first column
				0.0, 1.0, 0.0, 					// second column
				sin(theta), 0.0, cos(theta) 	// third column
			);
			rotation = rotation * rotationY;
		}

		void lookAt(glm::vec3 point) {
			glm::vec3 forward = glm::normalize(position - point);
			glm::vec3 vertical = glm::vec3(0.0, 1.0, 0.0);
			glm::vec3 right = glm::normalize(glm::cross(vertical, forward));
			glm::vec3 up = glm::cross(forward, right);

			rotation = glm::mat3(
				right,
				up,
				forward
			);
		}
};

enum IntersectionType { NONE, COLLISION };

class Intersection {
	public:
		IntersectionType type;
		RayTriangleIntersection intersection;
};

float SCALING_FACTOR = 0.17;

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
			
			materialMap[name] = textureMaterial;
		}
	}

	fileStream.close();
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
	assert(interpolatedDepth >= 0);

	return interpolatedDepth;
}

bool validCoord(int x, int y) {
	return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT;
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

// RASTERISING FUNCTIONS

void drawTextureLine(
		DrawingWindow &window, 
		CanvasPoint from, 
		CanvasPoint to, 
		Material material,
		float depthBuffer[WIDTH][HEIGHT]
	){
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	for(size_t i = 0; i < points.size(); i++) {
		int x = round(points[i].x);
		int y = round(points[i].y);
		
		float depth = interpolatePointDepth(from, to, points[i]);

		uint32_t colourCode;
		if (material.type == TEXTURE) {
			TexturePoint texturePoint = interpolateIntoTextureMap(from, to, points[i]);
			colourCode = getTexturePixelColour(material.textureMap, texturePoint.x, texturePoint.y);
		}
		else if (material.type == COLOUR) {
			colourCode = colourToCode(material.colour);
		}

		if (validCoord(x, y)) {
			if (depthBuffer[x][y] < depth){
				window.setPixelColour(x, y, colourCode);
				depthBuffer[x][y] = depth;
			}
		};
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

CanvasPoint vertexToImagePlane(glm::vec3 vertex, CameraEnvironment cameraEnv) {
	float planeScaling = 500;
	glm::vec3 absoluteDist = cameraEnv.position - vertex;
	absoluteDist.z *= -1;
	glm::vec3 orientedDist = cameraEnv.rotation * absoluteDist;

	// Checks that vertex in front of image plane
	assert(abs(orientedDist.z) > cameraEnv.focalLength);

	float u = (cameraEnv.focalLength / orientedDist.z) * orientedDist.x * planeScaling + WIDTH / 2;
	
	// negative since y of zero at top of page
	float v = -((cameraEnv.focalLength / orientedDist.z) * orientedDist.y * planeScaling) + HEIGHT / 2;

	float depth = 1 / (abs(orientedDist.z));
	
	return CanvasPoint(u, v, depth);
}

void drawCornellBox(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		float depthBuffer[WIDTH][HEIGHT],
		CameraEnvironment &cameraEnv
	) {
	for (int i = 0; i < triangles.size(); i++){
		std::vector<CanvasPoint> verticies;
		for (int j = 0; j < 3; j++){
			glm::vec3 modelVertex = triangles[i].vertices[j];
			CanvasPoint point = vertexToImagePlane(modelVertex, cameraEnv);

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

glm::vec3 rotateX(glm::vec3 vector, float theta){
	glm::mat3 rotation = glm::mat3(
		1.0, 0.0, 0.0, 					// first column
		0.0, cos(theta), sin(theta), 	// second column
		0.0, -sin(theta), cos(theta) 	// third column
	);
	return rotation * vector;
}


glm::vec3 rotateY(glm::vec3 vector, float theta){
	glm::mat3 rotation = glm::mat3(
		cos(theta), 0.0, -sin(theta), 	// first column
		0.0, 1.0, 0.0, 					// second column
		sin(theta), 0.0, cos(theta) 	// third column
	);
	return rotation * vector;
}

void rasterise(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		float depthBuffer[WIDTH][HEIGHT],
		CameraEnvironment &cameraEnv
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
		depthBuffer,
		cameraEnv
	);
}


// RAY TRACING

bool isValidSolution(glm::vec3 solution) {
	float u = solution.y;
	float v = solution.z;
	return (u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (u + v) <= 1.0;
}

RayTriangleIntersection buildRayTriangleIntersection(
		glm::vec3 ray,
		glm::vec3 cameraPosition,
		glm::vec3 rayDirection,
		ModelTriangle triangle,
		size_t triangleIndex) {
	
	glm::vec3 normalisedRayDirection = glm::normalize(rayDirection);
	glm::vec3 intersectionPoint = cameraPosition + ray[0] * normalisedRayDirection;

	return RayTriangleIntersection(
		intersectionPoint,
		ray[0],
		triangle,
		triangleIndex
	);
}

bool intersectionComparator(RayTriangleIntersection a, RayTriangleIntersection b) {
	return a.distanceFromCamera < b.distanceFromCamera;
}

Intersection getClosestIntersection(
		glm::vec3 cameraPosition, 
		glm::vec3 rayDirection, 
		std::vector<ModelTriangle> triangles){
	
	Intersection intersection;
	std::vector<RayTriangleIntersection> intersections;


	for (size_t i = 0; i < triangles.size(); i++){
		ModelTriangle triangle = triangles[i];
		glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
		glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
		glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
		glm::mat3 DEMatrix(-rayDirection, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

		if (isValidSolution(possibleSolution)) {
			RayTriangleIntersection rayTriangleIntersection = buildRayTriangleIntersection(
				possibleSolution,
				cameraPosition,
				rayDirection,
				triangle,
				i
			);
			intersections.push_back(rayTriangleIntersection);
		};
	}

	if (intersections.empty()) {
		intersection.type = NONE;
	} else {
		intersection.type = COLLISION;
		intersection.intersection = *std::min_element(
			intersections.begin(),
			intersections.end(),
			intersectionComparator
		);
	}
	return intersection;
}

void rayTraceCornellBox(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		CameraEnvironment &cameraEnv){
	float RAY_SCALING = 0.003;
	for (int x = 0; x < WIDTH; x++){
		for (int y = 0; y < HEIGHT; y++){
			//glm::vec3 imagePlanePoint = glm::vec3(i, j, )
			float u = (float(x) - (WIDTH / 2)) * RAY_SCALING;
			float v = -1 * (float(y) - (WIDTH / 2)) * RAY_SCALING;

			//ADUST FOR ORIENTATION AND POSTION

			glm::vec3 imagePlanePoint = glm::vec3(u, v, cameraEnv.focalLength);
			glm::vec3 rayDirection = imagePlanePoint - cameraEnv.position;

			Intersection intersection = getClosestIntersection(cameraEnv.position, rayDirection, triangles);
			if (intersection.type == COLLISION){
				Colour colour = intersection.intersection.intersectedTriangle.colour;
				window.setPixelColour(x, y, colourToCode(colour));
			}
			
		}
	}
};

void rayTrace(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		CameraEnvironment &cameraEnv){

	window.clearPixels();

	rayTraceCornellBox(window, triangles, materials, cameraEnv);
}


// EVENT LOOPS

void update(DrawingWindow &window, CameraEnvironment &cameraEnv) {

}

void handleEvent(SDL_Event event, DrawingWindow &window, CameraEnvironment &cameraEnv) {
	float TRANSLATION_STEP = 0.05;
	float ROTATION_STEP = M_PI * 0.01;
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) cameraEnv.position += glm::vec3(-TRANSLATION_STEP, 0.0, 0.0);
		else if (event.key.keysym.sym == SDLK_RIGHT) cameraEnv.position += glm::vec3(TRANSLATION_STEP, 0.0, 0.0);
		else if (event.key.keysym.sym == SDLK_UP) cameraEnv.position += glm::vec3(0.0, TRANSLATION_STEP, 0.0);
		else if (event.key.keysym.sym == SDLK_DOWN) cameraEnv.position += glm::vec3(0.0, -TRANSLATION_STEP, 0.0);
		else if (event.key.keysym.sym == SDLK_a) cameraEnv.position = rotateY(cameraEnv.position, -ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_d) cameraEnv.position = rotateY(cameraEnv.position, ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_w) cameraEnv.position = rotateX(cameraEnv.position, -ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_s) cameraEnv.position = rotateX(cameraEnv.position, ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_x) cameraEnv.rotateX(ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_c) cameraEnv.rotateX(-ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_u) cameraEnv.rotateY(ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_y) cameraEnv.rotateY(-ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_l) cameraEnv.lookAt(glm::vec3(0.0, 0.0, 0.0));

	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	std::map<std::string, Material> materialMap = loadMaterialsFromMTL("cornell-box.mtl");
	std::vector<Material> materials;
	std::vector<ModelTriangle> triangles = loadFromOBJ("cornell-box.obj", materialMap, materials);

	float depthBuffer[WIDTH][HEIGHT];

	CameraEnvironment cameraEnv;
	cameraEnv.position = glm::vec3(0.0, 0.0, 6.0);
	cameraEnv.focalLength = 2;
	cameraEnv.rotation = glm::mat3(
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0
	);

	rayTrace(window, triangles, materials, cameraEnv);
	
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, cameraEnv);
		
		update(window, cameraEnv);
		//rasterise(
		//	window,
		//	triangles,
		//	materials,
		//	depthBuffer,
		//	cameraEnv
		//);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}