#ifndef CS3P98_TEST_CAMERA_H
#define CS3P98_TEST_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*
    COSC 3P98 - Term Project
    @author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
    @author Daniel Sokic | 6164545 | ds16sz@brocku.ca
    @author Aditya Rajyaguru | 6582282 | ar18xp@brocku.ca
    @date 05.03.2021
*/

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    PITCHDOWN,
    PITCHUP,
    YAWLEFT,
    YAWRIGHT,
    ROLLLEFT,
    ROLLRIGHT,
    STARTTHRUST,
    ENDTHRUST,
};

// Default camera values
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float ROLL = 1.0f;
const float SPEED = 200.0f;
const float PITCHSPEED = 50.0f;
const float YAWSPEED = 100.0f;
const float ROLLSPEED = 1.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
    // camera Attributes
    glm::vec3 camPos;
    glm::vec3 camForward;
    glm::vec3 camUp;
    glm::vec3 camRight;
    glm::vec3 globalUp;
    // euler Angles
    float yaw;
    float pitch;
    float roll;
    // camera options
    float MovementSpeed;
    float PitchSpeed;
    float YawSpeed;
    float RollSpeed;
    float Zoom;
    float upOffsetX;
    float upOffsetY;
    float upOffsetZ;
    float momentum;
    // projection matrix
    glm::mat4 proj;                 // camera projection matrix
    float renderDist;               // render distance of this camera in world space (default 100.0)

    bool swap = false;
    bool flipped = false;

    // constructor with vectors
    Camera(float screenAspectRatio, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH, float roll = ROLL) :
        camForward(glm::vec3(0.0f, 0.0f, -1.0f)),
        MovementSpeed(SPEED),
        PitchSpeed(PITCHSPEED),
        YawSpeed(YAWSPEED),
        RollSpeed(ROLLSPEED),
        Zoom(ZOOM), 
        renderDist(100.0f)
    {
        redefineProjectionMatrix(screenAspectRatio);
        camPos = position;
        globalUp = up;
        yaw = yaw;
        pitch = pitch;
        upOffsetX = 0.0;
        upOffsetY = 0.0;
        upOffsetZ = 0.0;
        momentum = 0.0;
        updateCameraVectors();
    }


    // compute projection matrix for this camera object
    void redefineProjectionMatrix(float aspectRatio) {
        proj = glm::perspective(glm::radians(45.0f), 1000.0f/1000.0f, 0.1f, 10000.0f);           // horizon on earth is approximately 4.7km (4700 m) away from 5ft height, based on height from ground
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(camPos, camPos + camForward, camUp);
    }

    // Applies gravity to the camera
    void applyGravity(float deltaTime) {
        float pitchVelocity = PitchSpeed * deltaTime;
        camPos.y -= 0.02;
        pitch -= (pitchVelocity * 0.5 + (pitch * 0.0001));
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void processKeyControls(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        float pitchVelocity = PitchSpeed * deltaTime;
        float yawVelocity = YawSpeed * deltaTime;
        float rollVelocity = RollSpeed * deltaTime;
        float oldYaw = yaw;
        if (direction == PITCHDOWN) {
            pitch += pitchVelocity;
        }
        if (direction == PITCHUP)
            pitch -= pitchVelocity;
        if (direction == YAWLEFT) {
            yaw -= yawVelocity;
        }
        if (direction == YAWRIGHT) {
            yaw += yawVelocity;
        }
        if (direction == STARTTHRUST) {
            camPos += camForward * velocity * momentum;
            if (momentum < 1.0) {
                momentum += 0.001;
            }
        }
        if (direction == ENDTHRUST) {
            camPos += camForward * velocity * momentum;
            if (momentum > 0.0) {
                momentum -= 0.001;
            }
        }
        if (direction == ROLLLEFT) {
            if (!swap) {    //Normally rotate to left
                if (upOffsetX > -0.3 && upOffsetZ > -0.3) {
                    if (upOffsetY > -1.0) {
                        upOffsetX -= rollVelocity;
                        upOffsetZ -= rollVelocity;
                    }
                    else {
                        upOffsetX += rollVelocity;
                        upOffsetZ += rollVelocity;
                    }
                    if (upOffsetX > 0.0 && upOffsetZ > 0.0) {
                        upOffsetY += rollVelocity;
                    }
                    else {
                        upOffsetY -= rollVelocity;
                    }
                }
            }
            else {       //Reverse roll
                if (upOffsetX < 0.3 && upOffsetZ < 0.3) {
                    if (upOffsetY > -1.0) {
                        upOffsetX += rollVelocity;
                        upOffsetZ += rollVelocity;
                    }
                    else {
                        upOffsetX -= rollVelocity;
                        upOffsetZ -= rollVelocity;
                    }
                    if (upOffsetX > 0.0 && upOffsetZ > 0.0) {
                        upOffsetY -= rollVelocity;
                    }
                    else {
                        upOffsetY += rollVelocity;
                    }
                }
            }
        }
        if (direction == ROLLRIGHT) {
            if (!swap) {    //Normally roll to right
                if (upOffsetX < 0.3 && upOffsetZ < 0.3) {
                    if (upOffsetY > -1.0) {
                        upOffsetX += rollVelocity;
                        upOffsetZ += rollVelocity;
                    }
                    else {
                        upOffsetX -= rollVelocity;
                        upOffsetZ -= rollVelocity;
                    }
                    if (upOffsetX > 0.0 && upOffsetZ > 0.0) {
                        upOffsetY -= rollVelocity;
                    }
                    else {
                        upOffsetY += rollVelocity;
                    }
                }
            }
            else {         //Reverse roll
                if (upOffsetX > -0.3 && upOffsetZ > -0.3) {
                    if (upOffsetY > -1.0) {
                        upOffsetX -= rollVelocity;
                        upOffsetZ -= rollVelocity;
                    }
                    else {
                        upOffsetX += rollVelocity;
                        upOffsetZ += rollVelocity;
                    }
                    if (upOffsetX > 0.0 && upOffsetZ > 0.0) {
                        upOffsetY += rollVelocity;
                    }
                    else {
                        upOffsetY -= rollVelocity;
                    }
                }
            }
        }

        //Limit Yaw to be in range 0-360
        if (yaw > 360) {
            yaw = 0;
        }
        if (yaw < 0) {
            yaw = 360;
        }

        //These changes require the roll to swap to stay consistent
        if (oldYaw <= 45 && yaw >= 45) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }
        if (yaw <= 45 && oldYaw >= 45) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }

        if (oldYaw <= 225 && yaw >= 225) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }
        if (yaw <= 225 && oldYaw >= 225) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }

        //Bound X offset for roll
        if (upOffsetX < -0.3) upOffsetX = -0.3;
        if (upOffsetX > 0.3) upOffsetX = 0.3;

        //Bound Y offset for roll
        if (upOffsetY < -5) upOffsetY = -5;
        if (upOffsetY > 5) upOffsetY = 5;

        //Bound Z offset for roll
        if (upOffsetZ < -0.3) upOffsetZ = -0.3;
        if (upOffsetZ > 0.3) upOffsetZ = 0.3;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
        updateCameraVectors();

        //Update camera up based on offsets
        camUp.x += upOffsetX;
        camUp.y += upOffsetY;
        camUp.z += upOffsetZ;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseControls (float xoffset, float yoffset)
    {
        xoffset *= SENSITIVITY;
        yoffset *= SENSITIVITY;
        float oldYaw = yaw;

        yaw += xoffset;
        pitch += yoffset;

        //These changes require the roll to swap to stay consistent
        if (oldYaw <= 45 && yaw >= 45) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }
        if (yaw <= 45 && oldYaw >= 45) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }

        if (oldYaw <= 225 && yaw >= 225) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }
        if (yaw <= 225 && oldYaw >= 225) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }

        //Bound X offset for roll
        if (upOffsetX < -0.3) upOffsetX = -0.3;
        if (upOffsetX > 0.3) upOffsetX = 0.3;

        //Bound Y offset for roll
        if (upOffsetY < -5) upOffsetY = -5;
        if (upOffsetY > 5) upOffsetY = 5;

        //Bound Z offset for roll
        if (upOffsetZ < -0.3) upOffsetZ = -0.3;
        if (upOffsetZ > 0.3) upOffsetZ = 0.3;

        //Bound pitch to be straight up or straight down
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        updateCameraVectors();

        //Update camera up based on offsets
        camUp.x += upOffsetX;
        camUp.y += upOffsetY;
        camUp.z += upOffsetZ;
    }

private:

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        camForward = glm::normalize(front);
        // also re-calculate the Right and Up vector
        camRight = glm::normalize(glm::cross(camForward, globalUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        camUp = glm::normalize(glm::cross(camRight, camForward));
    }
};

#endif