#include "shader.h"
#ifndef __glad_h_
#include <glad/glad.h>
#endif

void Shader::Init(std::string name, std::string vertCode, std::string fragmentCode, std::string geomCode, std::string tcsCode, std::string tesCode)
{
    this->name = name;
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);

    bool hasGeom = geomCode.size() > 0;

    unsigned int gShader = 0;
    if (hasGeom)
    {
        gShader = glCreateShader(GL_GEOMETRY_SHADER);
    }

    hasTess = tcsCode.size() > 0;
    
    unsigned int tcsShader = 0;
    unsigned int tesShader = 0;

    if (hasTess)
    {
        assert(tesCode.size() > 0);

        tcsShader = glCreateShader(GL_TESS_CONTROL_SHADER);
        tesShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
    }

    ID = glCreateProgram();

    int status;
    char logInfo[1024];

    const char *vertSource = vertCode.c_str();
    const char *fragSource = fragmentCode.c_str();
    glShaderSource(vShader, 1, &vertSource, nullptr);
    glShaderSource(fShader, 1, &fragSource, nullptr);

    glCompileShader(vShader);
    glCompileShader(fShader);

    glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog(vShader, sizeof(logInfo), nullptr, logInfo);
        spdlog::info("vert shader compile error: {}", logInfo);
    }
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog(fShader, sizeof(logInfo), nullptr, logInfo);
        spdlog::info("frag shader compile error: {}", logInfo);
    }

    glAttachShader(ID, vShader);
    glAttachShader(ID, fShader);

    if (hasGeom)
    {
        const char* geomSource = geomCode.c_str();
        glShaderSource(gShader, 1, &geomSource, nullptr);
        glCompileShader(gShader);
        glGetShaderiv(gShader, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            glGetShaderInfoLog(gShader, sizeof(logInfo), nullptr, logInfo);
            spdlog::info("geom shader compile error: {}", logInfo);
        }
        glAttachShader(ID, gShader);
    }

    if (hasTess)
    {
        const char* tcsSource = tcsCode.c_str();
        glShaderSource(tcsShader, 1, &tcsSource, nullptr);
        glCompileShader(tcsShader);
        glGetShaderiv(tcsShader, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            glGetShaderInfoLog(tcsShader, sizeof(logInfo), nullptr, logInfo);
            spdlog::info("tcs shader compile error: {}", logInfo);
        }
        glAttachShader(ID, tcsShader);

        const char* tesSource = tesCode.c_str();
        glShaderSource(tesShader, 1, &tesSource, nullptr);
        glCompileShader(tesShader);
        glGetShaderiv(tesShader, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            glGetShaderInfoLog(tesShader, sizeof(logInfo), nullptr, logInfo);
            spdlog::info("tes shader compile error: {}", logInfo);
        }
        glAttachShader(ID, tesShader);
    }

    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &status);
    if (!status)
    {
        glGetProgramInfoLog(ID, 1024, nullptr, logInfo);
        spdlog::info("shader link error: {}", logInfo);
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    if (hasGeom)
    {
        glDeleteShader(gShader);
    }

    if (hasTess)
    {
        glDeleteShader(tcsShader);
        glDeleteShader(tesShader);
    }

    int uniformCount;
    
    glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &uniformCount);

    char nameBuffer[1024];
    for (int i = 0; i < uniformCount; i++)
    {
        unsigned int glType;
        UniformInfo uniform;
        
        glGetActiveUniform(ID, i, sizeof(nameBuffer), 0, &uniform.size, &glType, nameBuffer);
        uniform.name = nameBuffer;
        uniform.location = glGetUniformLocation(ID, nameBuffer);

        uniforms[uniform.name] = uniform;
    }
}

void Shader::Use()
{
    glUseProgram(ID);
}

void Shader::SetInt(std::string location, int value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform1i(loc, value);
    }
}

void Shader::SetBool(std::string location, bool value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform1i(loc, (int)value);
    }
}

void Shader::SetFloat(std::string location, float value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform1f(loc, value);
    }
}

void Shader::SetVec2(std::string location, glm::vec2 value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform2fv(loc, 1, glm::value_ptr(value));
    }
}

void Shader::SetVec3(std::string location, glm::vec3 value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform3fv(loc, 1, glm::value_ptr(value));
    }
}

void Shader::SetVec4(std::string location, glm::vec4 value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform4fv(loc, 1, glm::value_ptr(value));
    }
}

void Shader::SetVec2Array(std::string location, const std::vector<glm::vec2> value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform2fv(loc, value.size(), glm::value_ptr(value[0]));
    }
}

void Shader::SetVec3Array(std::string location, const std::vector<glm::vec3> value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform3fv(loc, value.size(), glm::value_ptr(value[0]));
    }
}

void Shader::SetVec4Array(std::string location, const std::vector<glm::vec4> value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniform4fv(loc, value.size(), glm::value_ptr(value[0]));
    }
}

void Shader::SetMat2(std::string location, glm::mat2 value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniformMatrix2fv(loc, 1, false, glm::value_ptr(value));
    }
}

void Shader::SetMat3(std::string location, glm::mat3 value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniformMatrix3fv(loc, 1, false, glm::value_ptr(value));
    }
}

void Shader::SetMat4(std::string location, glm::mat4 value)
{
    int loc = GetUniform(location);
    if (loc >= 0)
    {
        glUniformMatrix4fv(loc, 1, false, glm::value_ptr(value));
    }
}

int Shader::GetUniform(std::string name)
{
    std::map<std::string, UniformInfo>::iterator it = uniforms.find(name);
    if (it != uniforms.end())
    {
        return it->second.location;
    }

    return -1;
}

void Shader::SetUniformBlock(const std::string& location, int index)
{
    auto it = uniformBlockIndexMap.find(location);
    unsigned int uniformBlockIndex = -1;
    if (it != uniformBlockIndexMap.cend())
    {
        uniformBlockIndex = uniformBlockIndexMap[location];
    }
    else
    {
        uniformBlockIndex = glGetUniformBlockIndex(ID, location.c_str());
        uniformBlockIndexMap[location] = uniformBlockIndex;
    }

    glUniformBlockBinding(ID, uniformBlockIndex, index);
}