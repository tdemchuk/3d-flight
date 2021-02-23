#ifndef CS3P98_SHADER_H
#define CS3P98_SHADER_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <utility>          // std::pair


/*
	Shader class
	Represents a compiled shader program
	Slightly modified version of https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader.h written by Joey De Vries
*/
class Shader {
private:

    // check compilation status of shader for errors
    static void checkShaderCompileStatus(unsigned int shader, const std::string& type, const std::string& name) {
        int status;
        constexpr int errlog_size = 1024;
        char errlog[errlog_size];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == 0) {
            glGetShaderInfoLog(shader, errlog_size, nullptr, errlog);
            printf("%s shader %d [%s] failed to compile. Error Log:\n %s\n", type.c_str(), shader, name.c_str(), errlog);
            exit(EXIT_FAILURE);
        }
    }

    // check linking status of shader program for errors
    static void checkShaderLinkStatus(unsigned int shader, const std::string& name) {
        int status;
        constexpr int errlog_size = 1024;
        char errlog[errlog_size];
        glGetProgramiv(shader, GL_LINK_STATUS, &status);
        if (status == 0) {
            glGetProgramInfoLog(shader, errlog_size, nullptr, errlog);
            printf("Shader program %d [%s] failed to link. Error Log:\n %s\n", shader, name.c_str(), errlog);
            exit(EXIT_FAILURE);
        }
    }

public:

    // glob vars
    const unsigned int ID;            // shader program ID

    // constructor
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr) :
        ID(glCreateProgram())
    {

        // retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;

        std::string shaderName = std::string(vertexPath) + " | " + std::string(fragmentPath);
        if (geometryPath != nullptr) shaderName = shaderName + " | " + std::string(geometryPath);

        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            // if geometry shader path is present, also load a geometry shader
            if (geometryPath != nullptr)
            {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure & e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // compile shaders
        unsigned int vertex, fragment;

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkShaderCompileStatus(vertex, "Vertex", shaderName);

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkShaderCompileStatus(fragment, "Fragment", shaderName);

        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        if (geometryPath != nullptr)
        {
            const char* gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkShaderCompileStatus(geometry, "Geometry", shaderName);
        }

        // link shader prog
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometryPath != nullptr)
            glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkShaderLinkStatus(ID, shaderName);

        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr) glDeleteShader(geometry);
    }

    ~Shader() {			// destructor - perform cleanup
        glDeleteProgram(ID);
    }

    // activate the shader
    void use()
    {
        glUseProgram(ID);
    }

    // utility uniform functions
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};

#endif