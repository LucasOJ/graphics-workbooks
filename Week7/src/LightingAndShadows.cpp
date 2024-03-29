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

#define WIDTH 700
#define HEIGHT 700

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

enum RenderingMethod { RASTERISE, RAY_TRACE, WIREFRAME };

float SCALING_FACTOR = 0.17;

float PLANE_SCALING = 700.0;

float SHADOW_FADE = 0.5;
float BRIGHTNESS_SCALING = 1.0 / (M_PI);

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
			glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
			glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];
			triangle.normal = glm::normalize(glm::cross(edge1, edge2));
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
	glm::vec3 absoluteDist = cameraEnv.position - vertex;
	absoluteDist.z *= -1;
	glm::vec3 orientedDist = cameraEnv.rotation * absoluteDist;

	// Checks that vertex in front of image plane
	// assert(abs(orientedDist.z) > cameraEnv.focalLength);

	float u = (cameraEnv.focalLength / orientedDist.z) * orientedDist.x * PLANE_SCALING + WIDTH / 2;
	
	// negative since y of zero at top of page
	float v = -((cameraEnv.focalLength / orientedDist.z) * orientedDist.y * PLANE_SCALING) + HEIGHT / 2;

	float depth = 1 / (abs(orientedDist.z));
	
	return CanvasPoint(u, v, depth);
}

void drawRasterisedModel(
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

	drawRasterisedModel(
		window,
		triangles,
		materials,
		depthBuffer,
		cameraEnv
	);
}

// WIRE-FRAMING

void drawColourLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour) {
	std::vector<CanvasPoint> points = getLinePoints(from, to);
	int colourCode = colourToCode(colour);
	for(size_t i = 0; i < points.size(); i++) {
		int x = round(points[i].x);
		int y = round(points[i].y);
		
		if (validCoord(x, y)) {
			window.setPixelColour(x, y, colourCode);
		};
	}
}

void drawStrokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Material material){
	if (material.type == TEXTURE){
		material.colour = Colour(255,255,255);
	}
	drawColourLine(window, triangle.vertices[0], triangle.vertices[1], material.colour);
	drawColourLine(window, triangle.vertices[1], triangle.vertices[2], material.colour);
	drawColourLine(window, triangle.vertices[2], triangle.vertices[0], material.colour);
}

void drawWireframeModel(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		CameraEnvironment &cameraEnv
	) {
	window.clearPixels();
	for (int i = 0; i < triangles.size(); i++){
		std::vector<CanvasPoint> verticies;
		for (int j = 0; j < 3; j++){
			glm::vec3 modelVertex = triangles[i].vertices[j];
			CanvasPoint point = vertexToImagePlane(modelVertex, cameraEnv);
			verticies.push_back(point);
		}
		CanvasTriangle triangle = CanvasTriangle(verticies[0], verticies[1], verticies[2]);
		drawStrokedTriangle(window, triangle, materials[i]);
	}
}

// RAY TRACING

bool isValidSolution(glm::vec3 solution) {
	float t = solution.x;
	float u = solution.y;
	float v = solution.z;

	return (u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (u + v) <= 1.0 && t >= 0;
}

bool intersectionComparator(RayTriangleIntersection a, RayTriangleIntersection b) {
	return a.distanceFromCamera < b.distanceFromCamera;
}

RayTriangleIntersection getClosestIntersection(std::vector<RayTriangleIntersection> intersections) {
	assert(!intersections.empty());
	return *std::min_element(
		intersections.begin(),
		intersections.end(),
		intersectionComparator
	);
} 

std::vector<RayTriangleIntersection> getIntersections(
		glm::vec3 cameraPosition, 
		glm::vec3 rayDirection, 
		std::vector<ModelTriangle> triangles){
	
	std::vector<RayTriangleIntersection> intersections;
	glm::vec3 normalisedRayDirection = glm::normalize(rayDirection);

	for (size_t i = 0; i < triangles.size(); i++){
		ModelTriangle triangle = triangles[i];
		glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
		glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
		glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
		glm::mat3 DEMatrix(-normalisedRayDirection, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

		if (isValidSolution(possibleSolution)) {
			glm::vec3 intersectionPoint = cameraPosition + possibleSolution[0] * normalisedRayDirection;
			RayTriangleIntersection rayTriangleIntersection = RayTriangleIntersection(
				intersectionPoint,
				possibleSolution[0],
				triangle,
				i
			);
			intersections.push_back(rayTriangleIntersection);
		};
	}

	return intersections;
}

Colour adjustBrightness(Colour colour, float brightness) {

	return Colour(
		float(colour.red) * brightness,
		float(colour.green) * brightness, 
		float(colour.blue) * brightness
	);
}

float getSpecularCoefficient(glm::vec3 normal, glm::vec3 incidenceDirection, glm::vec3 viewDirection){
	glm::vec3 reflectionDirection = incidenceDirection - 2.0f * normal * glm::dot(normal, incidenceDirection);
	float specularCoefficent = glm::dot(reflectionDirection, normal);
	return pow(specularCoefficent, 256);
}

void rayTraceModel(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		CameraEnvironment &cameraEnv,
		glm::vec3 light){
	float RAY_SCALING = 1.0 / PLANE_SCALING;
	for (int x = 0; x < WIDTH; x++){
		for (int y = 0; y < HEIGHT; y++){
			float u = (float(x) - (WIDTH / 2)) * RAY_SCALING;
			float v = -1 * (float(y) - (WIDTH / 2)) * RAY_SCALING;

			glm::vec3 imagePlanePoint = glm::vec3(u, v, -cameraEnv.focalLength) + cameraEnv.position;
			glm::vec3 rayDirection = imagePlanePoint - cameraEnv.position;
			glm::vec3 rotatedRayDirection = cameraEnv.rotation * rayDirection;

			std::vector<RayTriangleIntersection> intersections = getIntersections(cameraEnv.position, rotatedRayDirection, triangles);
			if (!intersections.empty()){
				RayTriangleIntersection intersection = getClosestIntersection(intersections);
				glm::vec3 lightRay = light - intersection.intersectionPoint;
				float distanceToLight = glm::length(lightRay);

				std::vector<RayTriangleIntersection> shadowIntersections = getIntersections(
					intersection.intersectionPoint,
					lightRay,
					triangles
				); 
				

				Colour colour = intersection.intersectedTriangle.colour;

				
				float brightness = BRIGHTNESS_SCALING / (distanceToLight * distanceToLight);
				
				glm::vec3 normalisedLightRay = glm::normalize(lightRay);


				float angleOfIncidence = glm::dot(normalisedLightRay, intersection.intersectedTriangle.normal);
				angleOfIncidence = std::max(angleOfIncidence, float(0.0));
				
				brightness = brightness * angleOfIncidence;

				//std::vector<RayTriangleIntersection> validShadowIntersections;
				//for (size_t i = 0; i < shadowIntersections.size(); i++) {
				//	if (shadowIntersections[i].triangleIndex != intersection.triangleIndex) {
				//		validShadowIntersections.push_back(shadowIntersections[i]);
				//	}
				//}
				//if (!validShadowIntersections.empty()) {
				//	RayTriangleIntersection shadowIntersection = getClosestIntersection(validShadowIntersections);
				//	if (shadowIntersection.distanceFromCamera < distanceToLight) {
				//		brightness *= SHADOW_FADE;
				//	}
				//} else {
				//	brightness += 0.1f * specularCoeffcient;
				//}

				float specularCoeffcient = getSpecularCoefficient(
					intersection.intersectedTriangle.normal, 
					-normalisedLightRay, 
					-rotatedRayDirection
				);

				brightness += 0.3f * specularCoeffcient;

				float minBrightness = 0.2;

				brightness = std::min(brightness + minBrightness, 1.0f);

				Colour adjustedColour = adjustBrightness(colour, brightness);

				window.setPixelColour(x, y, colourToCode(adjustedColour));
			}
			
		}
	}
};

void rayTrace(
		DrawingWindow &window,
		std::vector<ModelTriangle> triangles,
		std::vector<Material> materials,
		CameraEnvironment &cameraEnv,
		glm::vec3 lightPosition){

	window.clearPixels();

	rayTraceModel(window, triangles, materials, cameraEnv, lightPosition);
}

// SHADING

// EVENT LOOPS

void update(DrawingWindow &window, CameraEnvironment &cameraEnv) {

}

void handleEvent(
		SDL_Event event,
		DrawingWindow &window,
		CameraEnvironment &cameraEnv,
		RenderingMethod &renderingMethod,
		glm::vec3 &lightPosition) {
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

		else if (event.key.keysym.sym == SDLK_x) cameraEnv.rotateX(-ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_c) cameraEnv.rotateX(ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_u) cameraEnv.rotateY(-ROTATION_STEP);
		else if (event.key.keysym.sym == SDLK_y) cameraEnv.rotateY(ROTATION_STEP);

		else if (event.key.keysym.sym == SDLK_o) cameraEnv.lookAt(glm::vec3(0.0, 0.0, 0.0));

		else if (event.key.keysym.sym == SDLK_1) renderingMethod = RASTERISE;
		else if (event.key.keysym.sym == SDLK_2) renderingMethod = WIREFRAME;
		else if (event.key.keysym.sym == SDLK_3) renderingMethod = RAY_TRACE;

		else if (event.key.keysym.sym == SDLK_j) lightPosition += glm::vec3(-TRANSLATION_STEP, 0.0, 0.0);
		else if (event.key.keysym.sym == SDLK_l) lightPosition += glm::vec3(TRANSLATION_STEP, 0.0, 0.0);
		else if (event.key.keysym.sym == SDLK_i) lightPosition += glm::vec3(0.0, TRANSLATION_STEP, 0.0);
		else if (event.key.keysym.sym == SDLK_k) lightPosition += glm::vec3(0.0, -TRANSLATION_STEP, 0.0);


	} else if (event.type == SDL_MOUSEBUTTONDOWN) window.savePPM("output.ppm");
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	std::map<std::string, Material> materialMap = loadMaterialsFromMTL("cornell-box.mtl");
	std::vector<Material> materials;
	std::vector<ModelTriangle> triangles = loadFromOBJ("sphere.obj", materialMap, materials);
	Material red;
	red.colour = Colour(255,0,0);
	red.type = TEXTURE;

	for (int i = 0; i < triangles.size(); i++){
		materials.push_back(red);
		triangles[i].colour = Colour(255,0,0);
		std::cout << triangles[i] << std::endl;
	}


	CameraEnvironment cameraEnv;
	cameraEnv.position = glm::vec3(0.0, 0.25, 1.0);
	cameraEnv.focalLength = 1.2;
	cameraEnv.rotation = glm::mat3(
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0
	);

	RenderingMethod renderingMethod = RAY_TRACE;

	// FOR RASTERISING
	float depthBuffer[WIDTH][HEIGHT];

	// FOR RAY TRACING
	glm::vec3 lightPosition = glm::vec3(0.5, 0.5, 0.5);

	
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, cameraEnv, renderingMethod, lightPosition);
		
		update(window, cameraEnv);
		if (renderingMethod == RASTERISE) {
			rasterise(
				window,
				triangles,
				materials,
				depthBuffer,
				cameraEnv
			);
		} else if (renderingMethod == WIREFRAME) {
			drawWireframeModel(window, triangles, materials, cameraEnv);
		} else if (renderingMethod == RAY_TRACE) {
			rayTrace(window, triangles, materials, cameraEnv, lightPosition);
		} 

		window.renderFrame();
	}
}