#ifndef CS3P98_TEST_CAMERA_H
#define CS3P98_TEST_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
const float SPEED = 200.0f;                     // commercial jet speeds are around 150-250 m/s
const float PITCHSPEED = 50.0f;
const float YAWSPEED = 100.0f;
const float ROLLSPEED = 1.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

/*
	First person camera class using the lookAt technique - https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h
	** We'll need a true flight sim camera with YAW, PITCH, and ROLL
	** ROLL is not supported here
    ** probably use quaternions to avoid gimbal lock - https://gamedev.stackexchange.com/questions/103502/how-can-i-implement-a-quaternion-camera
                                                     - https://www.gamedev.net/tutorials/programming/math-and-physics/a-simple-quaternion-based-camera-r1997/
    ** SUGGESTED CONTROLS FOR PLANE FLIGHTSIM:
    *   ENTER - Apply thrust (Move forward)
    *   W - Tilt down
    *   S - Tilt up
    *   A - Tilt Left (roll cam left and gradually change yaw)
    *   D - Tilt right
    
    ** Make Camera class owner of mouse and keyboard input handling functions
    * 
    * W - 
    * 
*/
class Camera {
public:
    // camera Attributes
    glm::vec3 camPos;
    glm::vec3 camForward;
    glm::vec3 camUp;
    glm::vec3 camRight;
    glm::vec3 globalUp;
    // euler Angles
    float Yaw;
    float Pitch;
    float Roll;
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
        Yaw = yaw;
        Pitch = pitch;
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

    void applyGravity(float deltaTime) {
        float pitchVelocity = PitchSpeed * deltaTime;
        camPos.y -= 0.04;
        Pitch -= pitchVelocity * 0.5;
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        float pitchVelocity = PitchSpeed * deltaTime;
        float yawVelocity = YawSpeed * deltaTime;
        float rollVelocity = RollSpeed * deltaTime;
        float oldYaw = Yaw;
        if (direction == PITCHDOWN) {
            Pitch += pitchVelocity;
        }
        if (direction == PITCHUP)
            Pitch -= pitchVelocity;
        if (direction == YAWLEFT) {
            Yaw -= yawVelocity;
            //printf("Yaw: %f", Yaw);
        }
        if (direction == YAWRIGHT) {
            Yaw += yawVelocity;
            //printf("Yaw: %f", Yaw);
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
                momentum -= 0.0003;
            }
        }
        if (direction == ROLLLEFT) {
            if (!swap) {
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
            else {
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
            if (!swap) {
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
            else {
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

        if (Yaw > 360) {
            Yaw = 0;
        }
        if (Yaw < 0) {
            Yaw = 360;
        }

        if (oldYaw <= 45 && Yaw >= 45) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }
        if (Yaw <= 45 && oldYaw >= 45) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }

        if (oldYaw <= 225 && Yaw >= 225) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }
        if (Yaw <= 225 && oldYaw >= 225) {
            upOffsetX *= -1;
            upOffsetZ *= -1;
            swap = !swap;
        }

        if (upOffsetX < -0.3) upOffsetX = -0.3;
        if (upOffsetY < -5) upOffsetY = -5;
        if (upOffsetZ < -0.3) upOffsetZ = -0.3;
        if (upOffsetX > 0.3) upOffsetX = 0.3;
        if (upOffsetY > 5) upOffsetY = 5;
        if (upOffsetZ > 0.3) upOffsetZ = 0.3;

        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
        updateCameraVectors();

        camUp.x += upOffsetX;
        camUp.y += upOffsetY;
        camUp.z += upOffsetZ;
    }

   /* // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }*/

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    /*void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }*/

private:

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        camForward = glm::normalize(front);
        // also re-calculate the Right and Up vector
        camRight = glm::normalize(glm::cross(camForward, globalUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        camUp = glm::normalize(glm::cross(camRight, camForward));
    }
};

#endif