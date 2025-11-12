// skeletal_animation.cpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <glm/gtx/string_cast.hpp>


#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// movement
glm::vec3 charPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 charFront = glm::vec3(0.0f, 0.0f, -1.0f); // initial forward
glm::vec3 charFrontTarget = glm::vec3(0.0f, 0.0f, -1.0f); // initial forward
float charSpeed = 2.5f;

// Hat Type
enum HatType
{
	Ghost = 1,
	Slime,
	Mario
};
HatType currentHatType = HatType::Ghost;


enum AnimState {
	IDLE = 1,
	IDLE_PUNCH,
	PUNCH_IDLE,
	IDLE_KICK,
	KICK_IDLE,
	IDLE_WALK,
	WALK_IDLE,
	WALK
};

// camera
float cameraRadius = 10.0f;          // distance from model
float orbitYaw = 0.0f;        // horizontal angle (degrees)
float orbitPitch = 20.0f;     // vertical angle (degrees)
float smoothSpeed = 8.0f;  // higher = faster interpolation
float targetYaw = orbitYaw;
float targetPitch = orbitPitch;

// hat pickups
struct HatPickup {
	glm::vec3 position;
	float size; // radius for collision
	HatType type;
};

std::vector<HatPickup> hats = {
	{ glm::vec3(1.0f, 0.2f, -3.0f), 0.5f, HatType::Ghost },
	{ glm::vec3(-1.0f, 0.2f, -3.0f), 0.5f, HatType::Slime },
	{ glm::vec3(-3.0f, 0.2f, -3.0f), 0.5f, HatType::Mario }
};

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader ourShader("anim_model.vs", "anim_model.fs");
	Shader hatShader(
		FileSystem::getPath("src/8.guest/2020/skeletal_animation/1.model_loading.vs").c_str(), 
		FileSystem::getPath("src/8.guest/2020/skeletal_animation/1.model_loading.fs").c_str()
	);

	// load models
	// -----------
	// idle 3.3, walk 2.06, run 0.83, punch 1.03, kick 1.6
	//Model ourModel(FileSystem::getPath("resources/objects/Skeletal/starmieIdle5.dae"));
	/*Animation idleAnimation(FileSystem::getPath("resources/objects/Skeletal/starmieIdle5.dae"), &ourModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/Skeletal/starmieWalking2.dae"), &ourModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/Skeletal/starmieRunning2.dae"), &ourModel);
	Animation punchAnimation(FileSystem::getPath("resources/objects/Skeletal/starmiePunching2.dae"), &ourModel);
	Animation kickAnimation(FileSystem::getPath("resources/objects/Skeletal/starmieMmaKick2.dae"), &ourModel);*/

	Model ourModel(FileSystem::getPath("resources/objects/Skeletal/baeIdle.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/Skeletal/baeIdle.dae"), &ourModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/Skeletal/baeWalking.dae"), &ourModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/Skeletal/baeRunning.dae"), &ourModel);
	Animation punchAnimation(FileSystem::getPath("resources/objects/Skeletal/baePunching.dae"), &ourModel);
	Animation kickAnimation(FileSystem::getPath("resources/objects/Skeletal/baeKick.dae"), &ourModel);

	Animator animator(&idleAnimation);
	enum AnimState charState = IDLE;
	float blendAmount = 0.0f;
	float blendRate = 0.055f;

	// hat object
	Model slimeHatModel(FileSystem::getPath("resources/objects/Skeletal/Hat/slimeHat2.dae"));
	Model ghostHatModel(FileSystem::getPath("resources/objects/Skeletal/Hat/ghostHat2.dae"));
	Model marioHatModel(FileSystem::getPath("resources/objects/Skeletal/Hat/marioHat.dae"));

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
			animator.PlayAnimation(&idleAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
			animator.PlayAnimation(&walkAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
			animator.PlayAnimation(&punchAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
			animator.PlayAnimation(&kickAnimation, NULL, 0.0f, 0.0f, 0.0f);


		switch (charState) {
		case IDLE:
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || 
				glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || 
				glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_WALK;
			}
			else if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &punchAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_PUNCH;
			}
			else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_KICK;
			}
			//printf("idle \n");
			break;
		case IDLE_WALK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = WALK;
			}
			//printf("idle_walk \n");
			break;
		case WALK:
			animator.PlayAnimation(&walkAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (glfwGetKey(window, GLFW_KEY_W) != GLFW_PRESS &&
				glfwGetKey(window, GLFW_KEY_S) != GLFW_PRESS &&
				glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS &&
				glfwGetKey(window, GLFW_KEY_D) != GLFW_PRESS) {
				charState = WALK_IDLE;
			}
			//printf("walking\n");
			break;
		case WALK_IDLE:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = IDLE;
			}
			//printf("walk_idle \n");
			break;
		case IDLE_PUNCH:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &punchAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&punchAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = PUNCH_IDLE;
			}
			//printf("idle_punch\n");
			break;
		case PUNCH_IDLE:
			if (animator.m_CurrentTime > 0.7f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&punchAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				//printf("punch_idle \n");
			}
			else {
				// punching
				//printf("punching \n");
			}
			break;
		case IDLE_KICK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&kickAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = KICK_IDLE;
			}
			//printf("idle_kick\n");
			break;
		case KICK_IDLE:
			if (animator.m_CurrentTime > 1.0f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&kickAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				//printf("kick_idle \n");
			}
			else {
				// punching
				//printf("kicking animator.m_CurrentTime = %f\n", animator.m_CurrentTime);
			}
			break;
		}

		for (auto& hat : hats)
		{
			float distance = glm::length(charPosition - hat.position);
			if (distance < hat.size)
			{
				currentHatType = hat.type;
				std::cout << "Picked up hat type: " << hat.type << std::endl;
			}
		}

		// lerp rotation
		float t = 0.1;
		charFront = glm::mix(charFront, charFrontTarget, t);

		animator.UpdateAnimation(deltaTime);

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// don't forget to enable shader before setting uniforms
		ourShader.use();

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		
		// Smoothly interpolate camera orbit
		float lerpFactor = 1.0f - expf(-smoothSpeed * deltaTime);
		orbitYaw = glm::mix(orbitYaw, targetYaw, lerpFactor);
		orbitPitch = glm::mix(orbitPitch, targetPitch, lerpFactor);

		// Camera uses smoothed orbit
		float smoothYawRad = glm::radians(orbitYaw);
		float smoothPitchRad = glm::radians(orbitPitch);

		// Model uses target orbit (instant)
		float targetYawRad = glm::radians(targetYaw);
		float targetPitchRad = glm::radians(targetPitch);

		float camX = cameraRadius * cos(smoothPitchRad) * sin(smoothYawRad);
		float camY = cameraRadius * sin(smoothPitchRad);
		float camZ = cameraRadius * cos(smoothPitchRad) * cos(smoothYawRad);

		glm::vec3 cameraPos = charPosition + glm::vec3(camX, camY, camZ);

		glm::vec3 charPosWithOffset = charPosition;
		charPosWithOffset.y += 1.0f;
		glm::mat4 view = glm::lookAt(cameraPos, charPosWithOffset, glm::vec3(0.0f, 1.0f, 0.0f));

		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
		
		model = glm::translate(model, charPosition);

		// rotate model to face movement
		if (glm::length(charFront) > 0.0f)
		{
			float angle = atan2(charFront.x, charFront.z); // yaw
			model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		ourShader.setMat4("model", model);
		ourModel.Draw(ourShader);

		hatShader.use();

		hatShader.setMat4("projection", projection);
		hatShader.setMat4("view", view);

		float hatsize = 70.0f; // ghosthat 90
		glm::vec3 hatoffset(0.0f, 0.5f, -0.1f);
		switch (currentHatType)
		{
		case HatType::Ghost:
			hatsize = 72.0f;
			hatoffset = glm::vec3(0.0f, 0.2f, -0.1f);
			break;
		case HatType::Slime:
			hatsize = 70.0f;
			hatoffset = glm::vec3(0.0f, 0.5f, -0.1f);
			break;
		case HatType::Mario:
			hatsize = 52.0f;
			hatoffset = glm::vec3(0.0f, 0.7f, -0.1f);
			break;
		}

		/*for (auto& kv : animator.m_BoneGlobals)
			std::cout << kv.first << "\n";*/
		glm::mat4 characterModelMatrix = glm::translate(glm::mat4(1.0f), charPosition);

		if (glm::length(charFront) > 0.0f)
		{
			float angle = atan2(charFront.x, charFront.z);
			characterModelMatrix = glm::rotate(characterModelMatrix, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		glm::mat4 headBoneMatrix = animator.GetBoneGlobalTransform("mixamorig_Head");
		glm::mat4 hatMatrix = characterModelMatrix * headBoneMatrix;
		hatMatrix = glm::scale(hatMatrix, glm::vec3(hatsize));
		hatMatrix = glm::translate(hatMatrix, hatoffset);

		hatShader.setMat4("model", hatMatrix);

		switch (currentHatType)
		{
			case HatType::Ghost:
				ghostHatModel.Draw(hatShader);
				break;
			case HatType::Slime:
				slimeHatModel.Draw(hatShader);
				break;
			case HatType::Mario:
				marioHatModel.Draw(hatShader);
				break;
		}

		float propHatSize = 1.0f;
		glm::mat4 hat1model = glm::mat4(1.0f);
		hat1model = glm::scale(hat1model, glm::vec3(propHatSize));	// it's a bit too big for our scene, so scale it down
		hat1model = glm::translate(hat1model, glm::vec3(1.0f, 0.2f, -3.0f)); // translate it down so it's at the center of the scene
		hatShader.setMat4("model", hat1model);
		ghostHatModel.Draw(hatShader);

		glm::mat4 hat2model = glm::mat4(1.0f);
		hat2model = glm::scale(hat2model, glm::vec3(propHatSize));	// it's a bit too big for our scene, so scale it down
		hat2model = glm::translate(hat2model, glm::vec3(-1.0f, 0.2f, -3.0f)); // translate it down so it's at the center of the scene
		hatShader.setMat4("model", hat2model);
		slimeHatModel.Draw(hatShader);

		glm::mat4 hat3model = glm::mat4(1.0f);
		hat3model = glm::scale(hat3model, glm::vec3(propHatSize));	// it's a bit too big for our scene, so scale it down
		hat3model = glm::translate(hat3model, glm::vec3(-3.0f, 0.2f, -3.0f)); // translate it down so it's at the center of the scene
		hatShader.setMat4("model", hat3model);
		marioHatModel.Draw(hatShader);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{

	glm::vec3 moveDir(0.0f);

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		moveDir += glm::vec3(0.0f, 0.0f, -1.0f);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		moveDir += glm::vec3(0.0f, 0.0f, 1.0f);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		moveDir += glm::vec3(-1.0f, 0.0f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		moveDir += glm::vec3(1.0f, 0.0f, 0.0f);

	if (glm::length(moveDir) > 0.0f)
	{
		moveDir = glm::normalize(moveDir);
		charPosition += moveDir * charSpeed * deltaTime;

		// update facing direction
		charFrontTarget = moveDir;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	// Apply to target rotation only
	targetYaw -= xoffset;
	targetPitch -= yoffset;

	// Clamp pitch
	if (targetPitch > 89.0f) targetPitch = 89.0f;
	if (targetPitch < -89.0f) targetPitch = -89.0f;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
