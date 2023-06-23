//#include <GL/glew.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <functional>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"
#include "Camera.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "UnitCube.h"


void updateGPUGeometry(GPU_Geometry& gpuGeom, CPU_Geometry const& cpuGeom) {
	gpuGeom.bind();
	gpuGeom.setVerts(cpuGeom.verts);
	gpuGeom.setNormals(cpuGeom.normals);
	gpuGeom.setCols(cpuGeom.cols);
}

struct GameTexture {
	GameTexture(std::string path, GLenum interpolation) :
		textures(path, interpolation)
	{}

	Texture textures;
};

glm::mat4 translate(glm::vec3 point) {
	return glm::translate(glm::mat4(1.0f), point);
}

glm::mat4 rotation(float angle) {
	return glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 rotationAxis(float angle, glm::vec3 axis) {
	return glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
}

CPU_Geometry sphereGeometry(float radius, glm::vec3 center) {
	CPU_Geometry lgeom;
	CPU_Geometry cgeom;
	CPU_Geometry fgeom;

	//initial line
	for (float angle = 0.0f; angle < 181.0f; angle = angle + 4.0f) {
		float a = glm::radians(angle);
		glm::vec3 point = glm::vec3{ radius * sin(a), radius * cos(a), 0.0f };
		lgeom.verts.push_back(point);
	}

	//rotation
	for (int i = 0; i < 91; i++) { //line about axis
		for (int j = 0; j < lgeom.verts.size(); j++) { //point in line
			glm::vec3 point = rotation(4.0f * i) * glm::vec4(lgeom.verts[j], 1.0f);
			cgeom.verts.push_back(point + center);
			cgeom.cols.push_back(glm::vec2{ i * 4.0 /360.0f, j * 4.0 / 180.0f});
			cgeom.normals.push_back(point);
		}
	}

	//triangles
	std::vector<int> h = { 0, 1, ((int)lgeom.verts.size()), 1, (1 + (int)lgeom.verts.size()), ((int)lgeom.verts.size()) };
	int pos = -lgeom.verts.size();
	for (int i = 0; i < 90; i++) {
		pos = pos + lgeom.verts.size();
		for (int j = 0; j < lgeom.verts.size() - 1; j++) {
			for (int k = 0; k < h.size(); k++) {
				fgeom.verts.push_back(cgeom.verts[pos + h[k] + j]);
				fgeom.cols.push_back(cgeom.cols[pos + h[k] + j]);
				fgeom.normals.push_back(cgeom.normals[pos + h[k] + j]);
			}
		}
	}

	return fgeom;
}

CPU_Geometry saturnsRings(float radius, glm::vec3 center) {
	CPU_Geometry lgeom;
	CPU_Geometry cgeom;
	CPU_Geometry fgeom;

	//initial line
	for (float i = 0.0f; i < radius * 1.5; i = i + 0.1f) {
		glm::vec3 point = glm::vec3{ radius + 0.1 + i, 0.0f, 0.0f };
		lgeom.verts.push_back(point);
	}

	//rotation
	for (int i = 0; i < 91; i++) { //line about axis
		for (int j = 0; j < lgeom.verts.size(); j++) { //point in line
			glm::vec3 point = rotation(4.0f * i) * glm::vec4(lgeom.verts[j], 1.0f);
			cgeom.verts.push_back(point + center);
			cgeom.cols.push_back(glm::vec2{ float(j + 1) / float(lgeom.verts.size() + 1), i * 4.0 / 360.0f });
			cgeom.normals.push_back(point);
		}
	}

	//triangles
	std::vector<int> h = { 0, 1, ((int)lgeom.verts.size()), 1, (1 + (int)lgeom.verts.size()), ((int)lgeom.verts.size()) };
	int pos = -lgeom.verts.size();
	for (int i = 0; i < 90; i++) {
		pos = pos + lgeom.verts.size();
		for (int j = 0; j < lgeom.verts.size() - 1; j++) {
			for (int k = 0; k < h.size(); k++) {
				fgeom.verts.push_back(cgeom.verts[pos + h[k] + j]);
				fgeom.cols.push_back(cgeom.cols[pos + h[k] + j]);
				fgeom.normals.push_back(cgeom.normals[pos + h[k] + j]);
			}
		}
	}

	return fgeom;
}

struct GameObject {
	// Struct's constructor deals with the texture.
	// Also sets default position, theta, scale, and transformationMatrix
	//GameObject(std::string texturePath, GLenum textureInterpolation) :
	GameObject(std::shared_ptr<GameTexture> g, glm::vec3 c, float r, float a, float b) :
		cgeom(sphereGeometry(r, c)),
		ggeom(),
		texture(g),
		center(c),
		radius(r),
		rotAxisAngle(a),
		orbitAxisAngle(b),
		transformationMatrix(1.0f) // This constructor sets it as the identity matrix
	{}

	std::shared_ptr<GameTexture> texture;
	CPU_Geometry cgeom;
	GPU_Geometry ggeom;
	glm::vec3 center;
	float radius;
	float rotAxisAngle;
	float orbitAxisAngle;
	glm::mat4 transformationMatrix;
};

// EXAMPLE CALLBACKS
class Assignment4 : public CallbackInterface {

public:
	Assignment4()
		: camera(glm::radians(45.f), glm::radians(45.f), 3.0)
		, aspect(1.0f)
		, rightMouseDown(false)
		, mouseOldX(0.0)
		, mouseOldY(0.0)
	{}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			if (key == GLFW_KEY_SPACE) { //Pause
				if (pause) {
					pause = false;
				}
				else {
					pause = true;
				}
			}
			else if (key == GLFW_KEY_R) { //Restart
				restart = true;
			}
			else if (key == GLFW_KEY_RIGHT) { //Increase speed
				speed = speed + 0.2;
			}
			else if (key == GLFW_KEY_LEFT) { //Decrease speed
				if (speed > 0.1) {
					speed = speed - 0.2;
				}
			}
		}
	}
	virtual void mouseButtonCallback(int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS)			rightMouseDown = true;
			else if (action == GLFW_RELEASE)	rightMouseDown = false;
		}
	}
	virtual void cursorPosCallback(double xpos, double ypos) {
		if (rightMouseDown) {
			camera.incrementTheta(ypos - mouseOldY);
			camera.incrementPhi(xpos - mouseOldX);
		}
		mouseOldX = xpos;
		mouseOldY = ypos;
	}
	virtual void scrollCallback(double xoffset, double yoffset) {
		camera.incrementR(yoffset);
	}
	virtual void windowSizeCallback(int width, int height) {
		// The CallbackInterface::windowSizeCallback will call glViewport for us
		CallbackInterface::windowSizeCallback(width,  height);
		aspect = float(width)/float(height);
	}

	void viewPipeline(ShaderProgram &sp) {
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = camera.getView();
		//V = glm::lookAt(
		//	glm::vec3(V[3][0], V[3][0], V[3][0]), //camera position
		//	centerPoint, //point to center at
		//	glm::vec3(V[0][0], V[1][0], V[2][0]));//up axis
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		GLint location = glGetUniformLocation(sp, "lightPosition");
		glm::vec3 light = camera.getPos();
		glUniform3fv(location, 1, glm::value_ptr(light));
		GLint uniMat = glGetUniformLocation(sp, "M");
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(M));
		uniMat = glGetUniformLocation(sp, "V");
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(V));
		uniMat = glGetUniformLocation(sp, "P");
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(P));
	}

	float getSpeed() {
		return speed;
	}
	bool getPause() {
		return pause;
	}
	bool getRestart() {
		return restart;
	}
	void setRestart() {
		restart = false;
	}

	Camera camera;
private:
	bool rightMouseDown;
	float aspect;
	double mouseOldX;
	double mouseOldY;
	float speed = 1.0f;
	bool pause = false;
	bool restart = false;
	glm::vec3 centerPoint = glm::vec3(0.0f, 0.0f, 0.0f);
};

//tilts the planet to its appropriate axis
void tiltAxis(GameObject& planet) {
	glm::mat4 rotate = rotationAxis(-(planet.rotAxisAngle - planet.orbitAxisAngle), glm::vec3{ 0.0f, 0.0f, 1.0f });
	for (int i = 0; i < planet.cgeom.verts.size(); i++) {
		planet.cgeom.verts[i] = translate(planet.center) * rotate * translate(-planet.center) * glm::vec4(planet.cgeom.verts[i], 1.0f);
	}
}

void resetScene(GameObject& sun, GameObject& earth, GameObject& moon, GameObject& mercury, GameObject& venus, GameObject& mars, GameObject& marsMoon1, GameObject& marsMoon2, GameObject& jupiter, GameObject& jupiterMoon1, GameObject& jupiterMoon2, GameObject& jupiterMoon3, GameObject& saturn, GameObject& saturnRings, GameObject& saturnMoon1, GameObject& saturnMoon2, GameObject& saturnMoon3, GameObject& uranus, GameObject& uranusMoon1, GameObject& uranusMoon2, GameObject& uranusMoon3, GameObject& neptune, GameObject& neptuneMoon1, GameObject& neptuneMoon2, GameObject& neptuneMoon3) {
	sun.transformationMatrix = glm::mat4(1.0f);
	earth.transformationMatrix = glm::mat4(1.0f);
	moon.transformationMatrix = glm::mat4(1.0f);
	mercury.transformationMatrix = glm::mat4(1.0f);
	venus.transformationMatrix = glm::mat4(1.0f);
	mars.transformationMatrix = glm::mat4(1.0f);
	marsMoon1.transformationMatrix = glm::mat4(1.0f);
	marsMoon2.transformationMatrix = glm::mat4(1.0f);
	jupiter.transformationMatrix = glm::mat4(1.0f);
	jupiterMoon1.transformationMatrix = glm::mat4(1.0f);
	jupiterMoon2.transformationMatrix = glm::mat4(1.0f);
	jupiterMoon3.transformationMatrix = glm::mat4(1.0f);
	saturn.transformationMatrix = glm::mat4(1.0f);
	saturnRings.transformationMatrix = glm::mat4(1.0f);
	saturnMoon1.transformationMatrix = glm::mat4(1.0f);
	saturnMoon2.transformationMatrix = glm::mat4(1.0f);
	saturnMoon3.transformationMatrix = glm::mat4(1.0f);
	uranus.transformationMatrix = glm::mat4(1.0f);
	uranusMoon1.transformationMatrix = glm::mat4(1.0f);
	uranusMoon2.transformationMatrix = glm::mat4(1.0f);
	uranusMoon3.transformationMatrix = glm::mat4(1.0f);
	neptune.transformationMatrix = glm::mat4(1.0f);
	neptuneMoon1.transformationMatrix = glm::mat4(1.0f);
	neptuneMoon2.transformationMatrix = glm::mat4(1.0f);
	neptuneMoon3.transformationMatrix = glm::mat4(1.0f);

	sun.cgeom = sphereGeometry(sun.radius, sun.center);
	earth.cgeom = sphereGeometry(earth.radius, earth.center);
	moon.cgeom = sphereGeometry(moon.radius, moon.center);
	mercury.cgeom = sphereGeometry(mercury.radius, mercury.center);
	venus.cgeom = sphereGeometry(venus.radius, venus.center);
	mars.cgeom = sphereGeometry(mars.radius, mars.center);
	marsMoon1.cgeom = sphereGeometry(marsMoon1.radius, marsMoon1.center);
	marsMoon2.cgeom = sphereGeometry(marsMoon2.radius, marsMoon2.center);
	jupiter.cgeom = sphereGeometry(jupiter.radius, jupiter.center);
	jupiterMoon1.cgeom = sphereGeometry(jupiterMoon1.radius, jupiterMoon1.center);
	jupiterMoon2.cgeom = sphereGeometry(jupiterMoon2.radius, jupiterMoon2.center);
	jupiterMoon3.cgeom = sphereGeometry(jupiterMoon3.radius, jupiterMoon3.center);
	saturn.cgeom = sphereGeometry(saturn.radius, saturn.center);
	saturnRings.cgeom = saturnsRings(saturnRings.radius, saturnRings.center);
	saturnMoon1.cgeom = sphereGeometry(saturnMoon1.radius, saturnMoon1.center);
	saturnMoon2.cgeom = sphereGeometry(saturnMoon2.radius, saturnMoon2.center);
	saturnMoon3.cgeom = sphereGeometry(saturnMoon3.radius, saturnMoon3.center);
	uranus.cgeom = sphereGeometry(uranus.radius, uranus.center);
	uranusMoon1.cgeom = sphereGeometry(uranusMoon1.radius, uranusMoon1.center);
	uranusMoon2.cgeom = sphereGeometry(uranusMoon2.radius, uranusMoon2.center);
	uranusMoon3.cgeom = sphereGeometry(uranusMoon3.radius, uranusMoon3.center);
	neptune.cgeom = sphereGeometry(neptune.radius, neptune.center);
	neptuneMoon1.cgeom = sphereGeometry(neptuneMoon1.radius, neptuneMoon1.center);
	neptuneMoon2.cgeom = sphereGeometry(neptuneMoon2.radius, neptuneMoon2.center);
	neptuneMoon3.cgeom = sphereGeometry(neptuneMoon3.radius, neptuneMoon3.center);
	tiltAxis(earth);
	tiltAxis(moon);
	tiltAxis(mercury);
	tiltAxis(venus);
	tiltAxis(mars);
	tiltAxis(marsMoon1);
	tiltAxis(marsMoon2);
	tiltAxis(jupiter);
	tiltAxis(jupiterMoon1);
	tiltAxis(jupiterMoon2);
	tiltAxis(jupiterMoon3);
	tiltAxis(saturn);
	tiltAxis(saturnMoon1);
	tiltAxis(saturnMoon2);
	tiltAxis(saturnMoon3);
	tiltAxis(uranus);
	tiltAxis(uranusMoon1);
	tiltAxis(uranusMoon2);
	tiltAxis(uranusMoon3);
	tiltAxis(neptune);
	tiltAxis(neptuneMoon1);
	tiltAxis(neptuneMoon2);
	tiltAxis(neptuneMoon3);

	updateGPUGeometry(sun.ggeom, sun.cgeom);
	updateGPUGeometry(earth.ggeom, earth.cgeom);
	updateGPUGeometry(mercury.ggeom, mercury.cgeom);
	updateGPUGeometry(venus.ggeom, venus.cgeom);
	updateGPUGeometry(mars.ggeom, mars.cgeom);
	updateGPUGeometry(marsMoon1.ggeom, marsMoon1.cgeom);
	updateGPUGeometry(marsMoon2.ggeom, marsMoon2.cgeom);
	updateGPUGeometry(jupiter.ggeom, jupiter.cgeom);
	updateGPUGeometry(jupiterMoon1.ggeom, jupiterMoon1.cgeom);
	updateGPUGeometry(jupiterMoon2.ggeom, jupiterMoon2.cgeom);
	updateGPUGeometry(jupiterMoon3.ggeom, jupiterMoon3.cgeom);
	updateGPUGeometry(saturn.ggeom, saturn.cgeom);
	updateGPUGeometry(saturnRings.ggeom, saturnRings.cgeom);
	updateGPUGeometry(saturnMoon1.ggeom, saturnMoon1.cgeom);
	updateGPUGeometry(saturnMoon2.ggeom, saturnMoon2.cgeom);
	updateGPUGeometry(saturnMoon3.ggeom, saturnMoon3.cgeom);
	updateGPUGeometry(uranus.ggeom, uranus.cgeom);
	updateGPUGeometry(uranusMoon1.ggeom, uranusMoon1.cgeom);
	updateGPUGeometry(uranusMoon2.ggeom, uranusMoon2.cgeom);
	updateGPUGeometry(uranusMoon3.ggeom, uranusMoon3.cgeom);
	updateGPUGeometry(neptune.ggeom, neptune.cgeom);
	updateGPUGeometry(neptuneMoon1.ggeom, neptuneMoon1.cgeom);
	updateGPUGeometry(neptuneMoon2.ggeom, neptuneMoon2.cgeom);
	updateGPUGeometry(neptuneMoon3.ggeom, neptuneMoon3.cgeom);
}

void drawPlanet(GameObject& planet, ShaderProgram& sp) {
	GLint uniMat = glGetUniformLocation(sp, "M");
	GLint centerloc = glGetUniformLocation(sp, "center");
	GLint normalLoc = glGetUniformLocation(sp, "Norm");

	glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(planet.transformationMatrix));
	glUniform3fv(centerloc, 1, glm::value_ptr(planet.center));

	glm::mat3 normal = transpose(inverse(planet.transformationMatrix));
	glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normal));

	planet.ggeom.bind();
	planet.texture->textures.bind();
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(planet.cgeom.verts.size()));
	planet.texture->textures.unbind();
}

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	Window window(800, 800, "CPSC 453 - Assignment 3");

	GLDebug::enable();

	// CALLBACKS
	auto a4 = std::make_shared<Assignment4>();
	window.setCallbacks(a4);

	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	UnitCube cube;
	cube.generateGeometry();

	std::shared_ptr<GameTexture> sunTexture = std::make_shared<GameTexture>(
		"textures/sun.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> earthTexture = std::make_shared<GameTexture>(
		"textures/earth.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> moonTexture = std::make_shared<GameTexture>(
		"textures/moon.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> spaceTexture = std::make_shared<GameTexture>(
		"textures/space.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> mercuryTexture = std::make_shared<GameTexture>(
		"textures/mercury.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> venusTexture = std::make_shared<GameTexture>(
		"textures/venus.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> marsTexture = std::make_shared<GameTexture>(
		"textures/mars.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> jupiterTexture = std::make_shared<GameTexture>(
		"textures/jupiter.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> saturnTexture = std::make_shared<GameTexture>(
		"textures/saturn.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> uranusTexture = std::make_shared<GameTexture>(
		"textures/uranus.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> neptuneTexture = std::make_shared<GameTexture>(
		"textures/neptune.jpg",
		GL_NEAREST
		);
	std::shared_ptr<GameTexture> saturnRingsTexture = std::make_shared<GameTexture>(
		"textures/saturnRings.png",
		GL_NEAREST
		);

	GameObject sun(sunTexture, glm::vec3{ 0.0f, 0.0f, 0.0f }, 0.8f, 0.0f, 0.0f);
	tiltAxis(sun);
	updateGPUGeometry(sun.ggeom, sun.cgeom);

	//Earth
	float distanceFromParent = 2.0f;
	float orbitAngle = 20.0f;
	float tiltAngle = 23.4f;
	GameObject earth(earthTexture, (sun.center + glm::vec3{ distanceFromParent*cos(glm::radians(orbitAngle)), distanceFromParent*sin(glm::radians(orbitAngle)), 0.0f}), 0.08f, tiltAngle, orbitAngle);
	tiltAxis(earth);
	updateGPUGeometry(earth.ggeom, earth.cgeom);

	//Earth moon
	distanceFromParent = 0.2f;
	float orbitAngle2 = 10.0f; //from earth orbit angle
	tiltAngle = 10.0f;
	GameObject moon(moonTexture,(earth.center + glm::vec3{ distanceFromParent *cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent *sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.02f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(moon);
	updateGPUGeometry(moon.ggeom, moon.cgeom);

	//Mercury
	distanceFromParent = 1.2f;
	orbitAngle = 17.0f;
	tiltAngle = 0.04f;
	GameObject mercury(mercuryTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.027f, tiltAngle, orbitAngle);
	tiltAxis(mercury);
	updateGPUGeometry(mercury.ggeom, mercury.cgeom);

	//Venus
	distanceFromParent = 1.6f;
	orbitAngle = 13.4f;
	tiltAngle = 177.4f;
	GameObject venus(venusTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.075f, tiltAngle, orbitAngle);
	tiltAxis(venus);
	updateGPUGeometry(venus.ggeom, venus.cgeom);

	//Mars
	distanceFromParent = 3.4f;
	orbitAngle = 11.8f;
	tiltAngle = 25.2f;
	GameObject mars(marsTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.05f, tiltAngle, orbitAngle);
	tiltAxis(mars);
	updateGPUGeometry(mars.ggeom, mars.cgeom);

	//Mars moon1
	distanceFromParent = 0.2f;
	orbitAngle2 = 10.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject marsMoon1(moonTexture, (mars.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.025f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(marsMoon1);
	updateGPUGeometry(marsMoon1.ggeom, marsMoon1.cgeom);

	//Mars moon2
	distanceFromParent = 0.25f;
	orbitAngle2 = 60.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject marsMoon2(moonTexture, (mars.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.02f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(marsMoon2);
	updateGPUGeometry(marsMoon2.ggeom, marsMoon2.cgeom);

	//Jupiter
	distanceFromParent = 8.0f;
	orbitAngle = 11.3f;
	tiltAngle = 3.1f;
	GameObject jupiter(jupiterTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.48f, tiltAngle, orbitAngle);
	tiltAxis(jupiter);
	updateGPUGeometry(jupiter.ggeom, jupiter.cgeom);

	//Jupiter moon1
	distanceFromParent = 0.9f;
	orbitAngle2 = 10.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject jupiterMoon1(moonTexture, (jupiter.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.06f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(jupiterMoon1);
	updateGPUGeometry(jupiterMoon1.ggeom, jupiterMoon1.cgeom);

	//Jupiter moon2
	distanceFromParent = 1.2f;
	orbitAngle2 = 60.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject jupiterMoon2(moonTexture, (jupiter.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.08f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(jupiterMoon2);
	updateGPUGeometry(jupiterMoon2.ggeom, jupiterMoon2.cgeom);

	//Jupiter moon3
	distanceFromParent = 1.4f;
	orbitAngle2 = 110.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject jupiterMoon3(moonTexture, (jupiter.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.1f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(jupiterMoon3);
	updateGPUGeometry(jupiterMoon3.ggeom, jupiterMoon3.cgeom);

	//Saturn
	distanceFromParent = 16.0f;
	orbitAngle = 15.0f;
	tiltAngle = 26.7f;
	GameObject saturn(saturnTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.4f, tiltAngle, orbitAngle);
	tiltAxis(saturn);
	updateGPUGeometry(saturn.ggeom, saturn.cgeom);

	//Saturns Rings
	distanceFromParent = 0.0f;
	orbitAngle = 0.0f;
	tiltAngle = 0.0f;
	GameObject saturnRings(saturnRingsTexture, (saturn.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.45f, tiltAngle, orbitAngle);
	saturnRings.cgeom.verts.clear();
	saturnRings.cgeom.cols.clear();
	saturnRings.cgeom.normals.clear();
	saturnRings.cgeom = saturnsRings(saturnRings.radius, saturnRings.center);
	updateGPUGeometry(saturnRings.ggeom, saturnRings.cgeom);

	//Saturn moon1
	distanceFromParent = 0.8f;
	orbitAngle2 = 10.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject saturnMoon1(moonTexture, (saturn.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.05f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(saturnMoon1);
	updateGPUGeometry(saturnMoon1.ggeom, saturnMoon1.cgeom);

	//Saturn moon2
	distanceFromParent = 1.0f;
	orbitAngle2 = 50.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject saturnMoon2(moonTexture, (saturn.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.06f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(saturnMoon2);
	updateGPUGeometry(saturnMoon2.ggeom, saturnMoon2.cgeom);

	//Saturn moon3
	distanceFromParent = 1.3f;
	orbitAngle2 = 120.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject saturnMoon3(moonTexture, (saturn.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.07f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(saturnMoon3);
	updateGPUGeometry(saturnMoon3.ggeom, saturnMoon3.cgeom);

	//Uranus
	distanceFromParent = 32.0f;
	orbitAngle = 10.8f;
	tiltAngle = 97.8f;
	GameObject uranus(uranusTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.17f, tiltAngle, orbitAngle);
	tiltAxis(uranus);
	updateGPUGeometry(uranus.ggeom, uranus.cgeom);

	//Uranus moon1
	distanceFromParent = 0.24f;
	orbitAngle2 = 10.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject uranusMoon1(moonTexture, (uranus.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.04f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(uranusMoon1);
	updateGPUGeometry(uranusMoon1.ggeom, uranusMoon1.cgeom);

	//Uranus moon2
	distanceFromParent = 0.35f;
	orbitAngle2 = 90.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject uranusMoon2(moonTexture, (uranus.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.05f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(uranusMoon2);
	updateGPUGeometry(uranusMoon2.ggeom, uranusMoon2.cgeom);

	//Uranus moon3
	distanceFromParent = 0.5f;
	orbitAngle2 = 140.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject uranusMoon3(moonTexture, (uranus.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.06f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(uranusMoon3);
	updateGPUGeometry(uranusMoon3.ggeom, uranusMoon3.cgeom);

	//Neptune
	distanceFromParent = 44.0f;
	orbitAngle = 11.8f;
	tiltAngle = 28.3f;
	GameObject neptune(neptuneTexture, (sun.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle)), 0.0f }), 0.16f, tiltAngle, orbitAngle);
	tiltAxis(neptune);
	updateGPUGeometry(neptune.ggeom, neptune.cgeom);

	//Neptune moon1
	distanceFromParent = 0.2f;
	orbitAngle2 = 10.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject neptuneMoon1(moonTexture, (neptune.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.02f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(neptuneMoon1);
	updateGPUGeometry(neptuneMoon1.ggeom, neptuneMoon1.cgeom);

	//Neptune moon2
	distanceFromParent = 0.3f;
	orbitAngle2 = 70.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject neptuneMoon2(moonTexture, (neptune.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.02f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(neptuneMoon2);
	updateGPUGeometry(neptuneMoon2.ggeom, neptuneMoon2.cgeom);

	//Neptune moon3
	distanceFromParent = 0.4f;
	orbitAngle2 = 100.0f; //from mars orbit angle
	tiltAngle = 10.0f;
	GameObject neptuneMoon3(moonTexture, (neptune.center + glm::vec3{ distanceFromParent * cos(glm::radians(orbitAngle2 + orbitAngle)), distanceFromParent * sin(glm::radians(orbitAngle2 + orbitAngle)), 0.0f }), 0.02f, 0.0f, orbitAngle2 + orbitAngle);
	tiltAxis(neptuneMoon3);
	updateGPUGeometry(neptuneMoon3.ggeom, neptuneMoon3.cgeom);




	GameObject space(spaceTexture, glm::vec3{ 0.0f, 0.0f, 0.0f },200.0f, 0.0f, 0.0f);
	updateGPUGeometry(space.ggeom, space.cgeom);

	CPU_Geometry testceom;
	GPU_Geometry testgeom;
	testceom.verts.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	testceom.verts.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	testceom.verts.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	testceom.verts.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	testceom.verts.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	testceom.verts.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	testceom.cols.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	testceom.cols.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	testceom.cols.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	testceom.cols.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	testceom.cols.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	testceom.cols.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	updateGPUGeometry(testgeom, testceom);


	glPointSize(10.0f);

	glm::vec3 orbitAxis2 = glm::vec3{ -sin(glm::radians(moon.orbitAxisAngle)), cos(glm::radians(moon.orbitAxisAngle)), 0.0f };
	float speed = 1.0f;
	bool restart = false;
	auto timeElapsed = glfwGetTime();

	// RENDER LOOP
	while (!window.shouldClose()) {
		auto newTimeEleapsed = glfwGetTime();
		auto dt = newTimeEleapsed - timeElapsed;
		timeElapsed = timeElapsed + dt;
		dt = speed * dt;


		glfwPollEvents();

		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		shader.use();

		a4->viewPipeline(shader);

		//RESTARTING ANIMATION
		if (a4->getRestart() != restart) {
			resetScene(sun, earth, moon, mercury, venus, mars, marsMoon1, marsMoon2, jupiter, jupiterMoon1, jupiterMoon2, jupiterMoon3, saturn, saturnRings, saturnMoon1, saturnMoon2, saturnMoon3, uranus, uranusMoon1, uranusMoon2, uranusMoon3, neptune, neptuneMoon1, neptuneMoon2,  neptuneMoon3);
			a4->setRestart();
			glfwSetTime(0.0f);
			timeElapsed = 0.0f + dt / speed;
		}

		//PLANET TRANSFORMATIONS
		if (a4->getPause() == false) {
			sun.transformationMatrix = rotationAxis(45.0f * dt, sun.cgeom.verts[0] - sun.center) * sun.transformationMatrix;

			glm::vec3 orbitAxis = glm::vec3{ -sin(glm::radians(earth.orbitAxisAngle)), cos(glm::radians(earth.orbitAxisAngle)), 0.0f };
			//EARTH ORBIT
			earth.transformationMatrix = rotationAxis(30.0f * dt, orbitAxis) * earth.transformationMatrix;
			//EARTH ROTATION
			//earth.transformationMatrix = translate(earth.transformationMatrix * glm::vec4(earth.center, 1.0f)) * rotationAxis(-(30.0f * dt), orbitAxis) * translate(-(earth.transformationMatrix * glm::vec4(earth.center, 1.0f))) * earth.transformationMatrix;
			earth.transformationMatrix = translate(earth.transformationMatrix * glm::vec4(earth.center, 1.0f)) * rotationAxis(360.0f * dt, earth.cgeom.verts[0] - earth.center) * translate(-(earth.transformationMatrix * glm::vec4(earth.center, 1.0f))) * earth.transformationMatrix;

			//MOVING WITH EARTH
			orbitAxis2 = glm::vec3{ -sin(glm::radians(moon.orbitAxisAngle)), cos(glm::radians(moon.orbitAxisAngle)), 0.0f };
			moon.transformationMatrix = translate(rotationAxis(30.0f * dt, orbitAxis) * (glm::vec4((moon.center - earth.center), 1.0f))) * rotationAxis(30.0f * dt, orbitAxis) * translate(-(moon.center - earth.center)) * moon.transformationMatrix;
			//ORBITING EARTH
			//moon.transformationMatrix = translate(earth.transformationMatrix * glm::vec4(earth.center, 1.0f)) * rotationAxis(-(30.0f * dt), orbitAxis) * translate(-(earth.transformationMatrix * glm::vec4(earth.center, 1.0f))) * moon.transformationMatrix;
			moon.transformationMatrix = translate(earth.transformationMatrix * glm::vec4(earth.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(earth.transformationMatrix * glm::vec4(earth.center, 1.0f))) * moon.transformationMatrix;
			//MOON ROTATION
			//moon.transformationMatrix = translate(moon.transformationMatrix * glm::vec4(moon.center, 1.0f)) * rotationAxis(-(126.0f * dt), orbitAxis2) * translate(-(moon.transformationMatrix * glm::vec4(moon.center, 1.0f))) * moon.transformationMatrix;
			moon.transformationMatrix = translate(moon.transformationMatrix * glm::vec4(moon.center, 1.0f)) * rotationAxis(100.0f * dt, moon.cgeom.verts[0] - moon.center) * translate(-(moon.transformationMatrix * glm::vec4(moon.center, 1.0f))) * moon.transformationMatrix;


			//MERCURY
			orbitAxis = glm::vec3{ -sin(glm::radians(mercury.orbitAxisAngle)), cos(glm::radians(mercury.orbitAxisAngle)), 0.0f };
			mercury.transformationMatrix = rotationAxis(124.4f * dt, orbitAxis) * mercury.transformationMatrix;
			mercury.transformationMatrix = translate(mercury.transformationMatrix * glm::vec4(mercury.center, 1.0f)) * rotationAxis(2.05f * dt, mercury.cgeom.verts[0] - mercury.center) * translate(-(mercury.transformationMatrix * glm::vec4(mercury.center, 1.0f))) * mercury.transformationMatrix;

			//VENUS
			orbitAxis = glm::vec3{ -sin(glm::radians(venus.orbitAxisAngle)), cos(glm::radians(venus.orbitAxisAngle)), 0.0f };
			venus.transformationMatrix = rotationAxis(48.7f * dt, orbitAxis) * venus.transformationMatrix;
			venus.transformationMatrix = translate(venus.transformationMatrix * glm::vec4(venus.center, 1.0f)) * rotationAxis(3.08f * dt, venus.cgeom.verts[0] - venus.center) * translate(-(venus.transformationMatrix * glm::vec4(venus.center, 1.0f))) * venus.transformationMatrix;

			//MARS
			orbitAxis = glm::vec3{ -sin(glm::radians(mars.orbitAxisAngle)), cos(glm::radians(mars.orbitAxisAngle)), 0.0f };
			mars.transformationMatrix = rotationAxis(15.9f * dt, orbitAxis) * mars.transformationMatrix;
			mars.transformationMatrix = translate(mars.transformationMatrix * glm::vec4(mars.center, 1.0f)) * rotationAxis(349.8f * dt, mars.cgeom.verts[0] - mars.center) * translate(-(mars.transformationMatrix * glm::vec4(mars.center, 1.0f))) * mars.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(marsMoon1.orbitAxisAngle)), cos(glm::radians(marsMoon1.orbitAxisAngle)), 0.0f };
			marsMoon1.transformationMatrix = translate(rotationAxis(15.9f * dt, orbitAxis) * (glm::vec4((marsMoon1.center - mars.center), 1.0f))) * rotationAxis(15.9f * dt, orbitAxis) * translate(-(marsMoon1.center - mars.center)) * marsMoon1.transformationMatrix;
			marsMoon1.transformationMatrix = translate(mars.transformationMatrix * glm::vec4(mars.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(mars.transformationMatrix * glm::vec4(mars.center, 1.0f))) * marsMoon1.transformationMatrix;
			marsMoon1.transformationMatrix = translate(marsMoon1.transformationMatrix * glm::vec4(marsMoon1.center, 1.0f)) * rotationAxis(100.0f * dt, marsMoon1.cgeom.verts[0] - marsMoon1.center) * translate(-(marsMoon1.transformationMatrix * glm::vec4(marsMoon1.center, 1.0f))) * marsMoon1.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(marsMoon2.orbitAxisAngle)), cos(glm::radians(marsMoon2.orbitAxisAngle)), 0.0f };
			marsMoon2.transformationMatrix = translate(rotationAxis(15.9f * dt, orbitAxis) * (glm::vec4((marsMoon2.center - mars.center), 1.0f))) * rotationAxis(15.9f * dt, orbitAxis) * translate(-(marsMoon2.center - mars.center)) * marsMoon2.transformationMatrix;
			marsMoon2.transformationMatrix = translate(mars.transformationMatrix * glm::vec4(mars.center, 1.0f)) * rotationAxis(60.0f * dt, orbitAxis2) * translate(-(mars.transformationMatrix * glm::vec4(mars.center, 1.0f))) * marsMoon2.transformationMatrix;
			marsMoon2.transformationMatrix = translate(marsMoon2.transformationMatrix * glm::vec4(marsMoon2.center, 1.0f)) * rotationAxis(100.0f * dt, marsMoon2.cgeom.verts[0] - marsMoon2.center) * translate(-(marsMoon2.transformationMatrix * glm::vec4(marsMoon2.center, 1.0f))) * marsMoon2.transformationMatrix;

			//JUPITER
			orbitAxis = glm::vec3{ -sin(glm::radians(jupiter.orbitAxisAngle)), cos(glm::radians(jupiter.orbitAxisAngle)), 0.0f };
			jupiter.transformationMatrix = rotationAxis(2.5f * dt, orbitAxis) * jupiter.transformationMatrix;
			jupiter.transformationMatrix = translate(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f)) * rotationAxis(872.7f * dt, jupiter.cgeom.verts[0] - jupiter.center) * translate(-(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f))) * jupiter.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(jupiterMoon1.orbitAxisAngle)), cos(glm::radians(jupiterMoon1.orbitAxisAngle)), 0.0f };
			jupiterMoon1.transformationMatrix = translate(rotationAxis(2.5f * dt, orbitAxis) * (glm::vec4((jupiterMoon1.center - jupiter.center), 1.0f))) * rotationAxis(2.5f * dt, orbitAxis) * translate(-(jupiterMoon1.center - jupiter.center)) * jupiterMoon1.transformationMatrix;
			jupiterMoon1.transformationMatrix = translate(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f))) * jupiterMoon1.transformationMatrix;
			jupiterMoon1.transformationMatrix = translate(jupiterMoon1.transformationMatrix * glm::vec4(jupiterMoon1.center, 1.0f)) * rotationAxis(100.0f * dt, jupiterMoon1.cgeom.verts[0] - jupiterMoon1.center) * translate(-(jupiterMoon1.transformationMatrix * glm::vec4(jupiterMoon1.center, 1.0f))) * jupiterMoon1.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(jupiterMoon2.orbitAxisAngle)), cos(glm::radians(jupiterMoon2.orbitAxisAngle)), 0.0f };
			jupiterMoon2.transformationMatrix = translate(rotationAxis(2.5f * dt, orbitAxis) * (glm::vec4((jupiterMoon2.center - jupiter.center), 1.0f))) * rotationAxis(2.5f * dt, orbitAxis) * translate(-(jupiterMoon2.center - jupiter.center)) * jupiterMoon2.transformationMatrix;
			jupiterMoon2.transformationMatrix = translate(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f)) * rotationAxis(80.0f * dt, orbitAxis2) * translate(-(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f))) * jupiterMoon2.transformationMatrix;
			jupiterMoon2.transformationMatrix = translate(jupiterMoon2.transformationMatrix * glm::vec4(jupiterMoon2.center, 1.0f)) * rotationAxis(100.0f * dt, jupiterMoon2.cgeom.verts[0] - jupiterMoon2.center) * translate(-(jupiterMoon2.transformationMatrix * glm::vec4(jupiterMoon2.center, 1.0f))) * jupiterMoon2.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(jupiterMoon3.orbitAxisAngle)), cos(glm::radians(jupiterMoon3.orbitAxisAngle)), 0.0f };
			jupiterMoon3.transformationMatrix = translate(rotationAxis(2.5f * dt, orbitAxis) * (glm::vec4((jupiterMoon3.center - jupiter.center), 1.0f))) * rotationAxis(2.5f * dt, orbitAxis) * translate(-(jupiterMoon3.center - jupiter.center)) * jupiterMoon3.transformationMatrix;
			jupiterMoon3.transformationMatrix = translate(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f)) * rotationAxis(40.0f * dt, orbitAxis2) * translate(-(jupiter.transformationMatrix * glm::vec4(jupiter.center, 1.0f))) * jupiterMoon3.transformationMatrix;
			jupiterMoon3.transformationMatrix = translate(jupiterMoon3.transformationMatrix * glm::vec4(jupiterMoon3.center, 1.0f)) * rotationAxis(100.0f * dt, jupiterMoon3.cgeom.verts[0] - jupiterMoon3.center) * translate(-(jupiterMoon3.transformationMatrix * glm::vec4(jupiterMoon3.center, 1.0f))) * jupiterMoon3.transformationMatrix;

			//SATURN
			orbitAxis = glm::vec3{ -sin(glm::radians(saturn.orbitAxisAngle)), cos(glm::radians(saturn.orbitAxisAngle)), 0.0f };
			saturn.transformationMatrix = rotationAxis(1.02f * dt, orbitAxis) * saturn.transformationMatrix;
			saturn.transformationMatrix = translate(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f)) * rotationAxis(807.5f * dt, saturn.cgeom.verts[0] - saturn.center) * translate(-(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f))) * saturn.transformationMatrix;

			saturnRings.transformationMatrix = rotationAxis(1.02f * dt, orbitAxis) * saturnRings.transformationMatrix;
			//saturnRings.transformationMatrix = translate(saturnRings.transformationMatrix * glm::vec4(saturnRings.center, 1.0f)) * rotationAxis(807.5f * dt, saturnRings.cgeom.verts[0] - saturnRings.center) * translate(-(saturnRings.transformationMatrix * glm::vec4(saturnRings.center, 1.0f))) * saturnRings.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(saturnMoon1.orbitAxisAngle)), cos(glm::radians(saturnMoon1.orbitAxisAngle)), 0.0f };
			saturnMoon1.transformationMatrix = translate(rotationAxis(1.02f * dt, orbitAxis) * (glm::vec4((saturnMoon1.center - saturn.center), 1.0f))) * rotationAxis(1.02f * dt, orbitAxis) * translate(-(saturnMoon1.center - saturn.center)) * saturnMoon1.transformationMatrix;
			saturnMoon1.transformationMatrix = translate(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f))) * saturnMoon1.transformationMatrix;
			saturnMoon1.transformationMatrix = translate(saturnMoon1.transformationMatrix * glm::vec4(saturnMoon1.center, 1.0f)) * rotationAxis(100.0f * dt, saturnMoon1.cgeom.verts[0] - saturnMoon1.center) * translate(-(saturnMoon1.transformationMatrix * glm::vec4(saturnMoon1.center, 1.0f))) * saturnMoon1.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(saturnMoon2.orbitAxisAngle)), cos(glm::radians(saturnMoon2.orbitAxisAngle)), 0.0f };
			saturnMoon2.transformationMatrix = translate(rotationAxis(1.02f * dt, orbitAxis) * (glm::vec4((saturnMoon2.center - saturn.center), 1.0f))) * rotationAxis(1.02f * dt, orbitAxis) * translate(-(saturnMoon2.center - saturn.center)) * saturnMoon2.transformationMatrix;
			saturnMoon2.transformationMatrix = translate(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f)) * rotationAxis(70.0f * dt, orbitAxis2) * translate(-(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f))) * saturnMoon2.transformationMatrix;
			saturnMoon2.transformationMatrix = translate(saturnMoon2.transformationMatrix * glm::vec4(saturnMoon2.center, 1.0f)) * rotationAxis(100.0f * dt, saturnMoon2.cgeom.verts[0] - saturnMoon2.center) * translate(-(saturnMoon2.transformationMatrix * glm::vec4(saturnMoon2.center, 1.0f))) * saturnMoon2.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(saturnMoon3.orbitAxisAngle)), cos(glm::radians(saturnMoon3.orbitAxisAngle)), 0.0f };
			saturnMoon3.transformationMatrix = translate(rotationAxis(1.02f * dt, orbitAxis) * (glm::vec4((saturnMoon3.center - saturn.center), 1.0f))) * rotationAxis(1.02f * dt, orbitAxis) * translate(-(saturnMoon3.center - saturn.center)) * saturnMoon3.transformationMatrix;
			saturnMoon3.transformationMatrix = translate(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f)) * rotationAxis(50.0f * dt, orbitAxis2) * translate(-(saturn.transformationMatrix * glm::vec4(saturn.center, 1.0f))) * saturnMoon3.transformationMatrix;
			saturnMoon3.transformationMatrix = translate(saturnMoon3.transformationMatrix * glm::vec4(saturnMoon3.center, 1.0f)) * rotationAxis(100.0f * dt, saturnMoon3.cgeom.verts[0] - saturnMoon3.center) * translate(-(saturnMoon3.transformationMatrix * glm::vec4(saturnMoon3.center, 1.0f))) * saturnMoon3.transformationMatrix;

			//URANUS
			orbitAxis = glm::vec3{ -sin(glm::radians(uranus.orbitAxisAngle)), cos(glm::radians(uranus.orbitAxisAngle)), 0.0f };
			uranus.transformationMatrix = rotationAxis(0.4f * dt, orbitAxis) * uranus.transformationMatrix;
			uranus.transformationMatrix = translate(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f)) * rotationAxis(502.3f * dt, uranus.cgeom.verts[0] - uranus.center) * translate(-(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f))) * uranus.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(uranusMoon1.orbitAxisAngle)), cos(glm::radians(uranusMoon1.orbitAxisAngle)), 0.0f };
			uranusMoon1.transformationMatrix = translate(rotationAxis(0.4f * dt, orbitAxis) * (glm::vec4((uranusMoon1.center - uranus.center), 1.0f))) * rotationAxis(0.4f * dt, orbitAxis) * translate(-(uranusMoon1.center - uranus.center)) * uranusMoon1.transformationMatrix;
			uranusMoon1.transformationMatrix = translate(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f))) * uranusMoon1.transformationMatrix;
			uranusMoon1.transformationMatrix = translate(uranusMoon1.transformationMatrix * glm::vec4(uranusMoon1.center, 1.0f)) * rotationAxis(100.0f * dt, uranusMoon1.cgeom.verts[0] - uranusMoon1.center) * translate(-(uranusMoon1.transformationMatrix * glm::vec4(uranusMoon1.center, 1.0f))) * uranusMoon1.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(uranusMoon2.orbitAxisAngle)), cos(glm::radians(uranusMoon2.orbitAxisAngle)), 0.0f };
			uranusMoon2.transformationMatrix = translate(rotationAxis(0.4f * dt, orbitAxis) * (glm::vec4((uranusMoon2.center - uranus.center), 1.0f))) * rotationAxis(0.4f * dt, orbitAxis) * translate(-(uranusMoon2.center - uranus.center)) * uranusMoon2.transformationMatrix;
			uranusMoon2.transformationMatrix = translate(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f)) * rotationAxis(60.0f * dt, orbitAxis2) * translate(-(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f))) * uranusMoon2.transformationMatrix;
			uranusMoon2.transformationMatrix = translate(uranusMoon2.transformationMatrix * glm::vec4(uranusMoon2.center, 1.0f)) * rotationAxis(100.0f * dt, uranusMoon2.cgeom.verts[0] - uranusMoon2.center) * translate(-(uranusMoon2.transformationMatrix * glm::vec4(uranusMoon2.center, 1.0f))) * uranusMoon2.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(uranusMoon3.orbitAxisAngle)), cos(glm::radians(uranusMoon3.orbitAxisAngle)), 0.0f };
			uranusMoon3.transformationMatrix = translate(rotationAxis(0.4f * dt, orbitAxis) * (glm::vec4((uranusMoon3.center - uranus.center), 1.0f))) * rotationAxis(0.4f * dt, orbitAxis) * translate(-(uranusMoon3.center - uranus.center)) * uranusMoon3.transformationMatrix;
			uranusMoon3.transformationMatrix = translate(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f)) * rotationAxis(30.0f * dt, orbitAxis2) * translate(-(uranus.transformationMatrix * glm::vec4(uranus.center, 1.0f))) * uranusMoon3.transformationMatrix;
			uranusMoon3.transformationMatrix = translate(uranusMoon3.transformationMatrix * glm::vec4(uranusMoon3.center, 1.0f)) * rotationAxis(100.0f * dt, moon.cgeom.verts[0] - uranusMoon3.center) * translate(-(uranusMoon3.transformationMatrix * glm::vec4(uranusMoon3.center, 1.0f))) * uranusMoon3.transformationMatrix;

			//NEPTUNE
			orbitAxis = glm::vec3{ -sin(glm::radians(neptune.orbitAxisAngle)), cos(glm::radians(neptune.orbitAxisAngle)), 0.0f };
			neptune.transformationMatrix = rotationAxis(0.3f * dt, orbitAxis) * neptune.transformationMatrix;
			neptune.transformationMatrix = translate(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f)) * rotationAxis(536.6f * dt, neptune.cgeom.verts[0] - neptune.center) * translate(-(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f))) * neptune.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(neptuneMoon1.orbitAxisAngle)), cos(glm::radians(neptuneMoon1.orbitAxisAngle)), 0.0f };
			neptuneMoon1.transformationMatrix = translate(rotationAxis(0.3f * dt, orbitAxis) * (glm::vec4((neptuneMoon1.center - neptune.center), 1.0f))) * rotationAxis(0.3f * dt, orbitAxis) * translate(-(neptuneMoon1.center - neptune.center)) * neptuneMoon1.transformationMatrix;
			neptuneMoon1.transformationMatrix = translate(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f))) * neptuneMoon1.transformationMatrix;
			neptuneMoon1.transformationMatrix = translate(neptuneMoon1.transformationMatrix * glm::vec4(neptuneMoon1.center, 1.0f)) * rotationAxis(100.0f * dt, neptuneMoon1.cgeom.verts[0] - neptuneMoon1.center) * translate(-(neptuneMoon1.transformationMatrix * glm::vec4(neptuneMoon1.center, 1.0f))) * neptuneMoon1.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(neptuneMoon2.orbitAxisAngle)), cos(glm::radians(neptuneMoon2.orbitAxisAngle)), 0.0f };
			neptuneMoon2.transformationMatrix = translate(rotationAxis(0.3f * dt, orbitAxis) * (glm::vec4((neptuneMoon2.center - neptune.center), 1.0f))) * rotationAxis(0.3f * dt, orbitAxis) * translate(-(neptuneMoon2.center - neptune.center)) * neptuneMoon2.transformationMatrix;
			neptuneMoon2.transformationMatrix = translate(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f))) * neptuneMoon2.transformationMatrix;
			neptuneMoon2.transformationMatrix = translate(neptuneMoon2.transformationMatrix * glm::vec4(neptuneMoon2.center, 1.0f)) * rotationAxis(100.0f * dt, neptuneMoon2.cgeom.verts[0] - neptuneMoon2.center) * translate(-(neptuneMoon2.transformationMatrix * glm::vec4(neptuneMoon2.center, 1.0f))) * neptuneMoon2.transformationMatrix;

			orbitAxis2 = glm::vec3{ -sin(glm::radians(neptuneMoon3.orbitAxisAngle)), cos(glm::radians(neptuneMoon3.orbitAxisAngle)), 0.0f };
			neptuneMoon3.transformationMatrix = translate(rotationAxis(0.3f * dt, orbitAxis) * (glm::vec4((neptuneMoon3.center - neptune.center), 1.0f))) * rotationAxis(0.3f * dt, orbitAxis) * translate(-(neptuneMoon3.center - neptune.center)) * neptuneMoon3.transformationMatrix;
			neptuneMoon3.transformationMatrix = translate(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f)) * rotationAxis(126.0f * dt, orbitAxis2) * translate(-(neptune.transformationMatrix * glm::vec4(neptune.center, 1.0f))) * neptuneMoon3.transformationMatrix;
			neptuneMoon3.transformationMatrix = translate(neptuneMoon3.transformationMatrix * glm::vec4(neptuneMoon3.center, 1.0f)) * rotationAxis(100.0f * dt, neptuneMoon3.cgeom.verts[0] - neptuneMoon3.center) * translate(-(neptuneMoon3.transformationMatrix * glm::vec4(neptuneMoon3.center, 1.0f))) * neptuneMoon3.transformationMatrix;

		}
		else {
			timeElapsed = timeElapsed - dt / speed;
			dt = 0;
			glfwSetTime(timeElapsed);
		}

		float s = 1.0f;
		GLint loc = glGetUniformLocation(shader, "sun");
		GLint uniMat = glGetUniformLocation(shader, "M");
		GLint centerloc = glGetUniformLocation(shader, "center");
		GLint normalLoc = glGetUniformLocation(shader, "Norm");

		//SUN
		glUniform1f(loc, s);
		drawPlanet(sun, shader);

		//EARTH
		s = 0.0f;
		glUniform1f(loc, s);
		drawPlanet(earth, shader);

		//MOON
		drawPlanet(moon, shader);

		//ALL OTHER PLANETS AND THEIR MOONS
		drawPlanet(mercury, shader);
		drawPlanet(venus, shader);
		drawPlanet(mars, shader);
		drawPlanet(marsMoon1, shader);
		drawPlanet(marsMoon2, shader);
		drawPlanet(jupiter, shader);
		drawPlanet(jupiterMoon1, shader);
		drawPlanet(jupiterMoon2, shader);
		drawPlanet(jupiterMoon3, shader);
		drawPlanet(saturn, shader);
		drawPlanet(saturnRings, shader);
		drawPlanet(saturnMoon1, shader);
		drawPlanet(saturnMoon2, shader);
		drawPlanet(saturnMoon3, shader);
		drawPlanet(uranus, shader);
		drawPlanet(uranusMoon1, shader);
		drawPlanet(uranusMoon2, shader);
		drawPlanet(uranusMoon3, shader);
		drawPlanet(neptune, shader);
		drawPlanet(neptuneMoon1, shader);
		drawPlanet(neptuneMoon2, shader);
		drawPlanet(neptuneMoon3, shader);

		//SPACE
		s = 1.0f;
		glUniform1f(loc, s);
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(space.transformationMatrix));

		space.ggeom.bind();
		space.texture->textures.bind();
		glDrawArrays(GL_TRIANGLES, 0, GLsizei(space.cgeom.verts.size()));
		space.texture->textures.bind();

		//X, Y, Z AXIS
		testgeom.bind();
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(testceom.verts.size()));

		glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui
		window.swapBuffers();

		if (a4->getSpeed() != speed) {
			speed = a4->getSpeed();
		}
	}
	glfwTerminate();
	return 0;
}
