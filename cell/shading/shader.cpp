#include "shader.h"

#include <utility/logging/log.h>

#include "../glad/glad.h"

#include <fstream>
#include <string>

namespace Cell
{
    // ------------------------------------------------------------------------
    Shader::Shader()
    {

    }
    // ------------------------------------------------------------------------
    Shader::Shader(std::string name, std::string vsCode, std::string fsCode, std::vector<std::string> defines)
    {      
        Load(name, vsCode, fsCode, defines);
    }
    // ------------------------------------------------------------------------
    void Shader::Load(std::string name, std::string vsCode, std::string fsCode, std::vector<std::string> defines)
    {
        // NOTE(Joey): compile both shaders and link them
        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        ID = glCreateProgram();
        int status;
        char log[1024];

        
        // NOTE(Joey): if a list of define statements is specified, add these 
        // to the start of the shader source, s.t. we can selectively compile
        // different shaders based on the defines we set.
        if (defines.size() > 0)
        {
            std::vector<std::string> vsMergedCode;
            std::vector<std::string> fsMergedCode;
            // NOTE(Joey): first determine if the user supplied a #version 
            // directive at the top of the shader code, in which case we 
            // extract it and add it 'before' the list of define code.
            // The GLSL version specifier is only valid as the first line of
            // the GLSL code; otherwise the GLSL version defaults to 1.1.
            std::string firstLine = vsCode.substr(0, vsCode.find("\n"));
            if (firstLine.find("#version") != std::string::npos)
            {
                // NOTE(Joey): strip shader code of first line and add to list
                // of shader code strings
                vsCode = vsCode.substr(vsCode.find("\n") + 1, vsCode.length() - 1);
                vsMergedCode.push_back(firstLine + "\n");
            }
            firstLine = fsCode.substr(0, fsCode.find("\n"));
            if (firstLine.find("#version") != std::string::npos)
            {
                // NOTE(Joey): strip shader code of first line and add to list
                // of shader code strings
                fsCode = fsCode.substr(fsCode.find("\n") + 1, fsCode.length() - 1);
                fsMergedCode.push_back(firstLine + "\n");
            }
            // NOTE(Joey): then add define statements to the shader string list
            for (unsigned int i = 0; i < defines.size(); ++i)
            {
                std::string define = "#define " + defines[i] + "\n";
                vsMergedCode.push_back(define);
                fsMergedCode.push_back(define);
            }
            // then addremaining shader code to merged result and pass result
            // to glShaderSource
            vsMergedCode.push_back(vsCode);
            fsMergedCode.push_back(fsCode);
            // NOTE(Joey): note that we manually build an array of C style 
            // strings as glShaderSource doesn't expect it in any other format.
            // Note that all strings are null-terminated so we can pass NULL
            // as glShaderSource's final argument.
            const char **vsStringsC = new const char*[vsMergedCode.size()];
            const char **fsStringsC = new const char*[fsMergedCode.size()];
            for (unsigned int i = 0; i < vsMergedCode.size(); ++i)
                vsStringsC[i] = vsMergedCode[i].c_str();
            for (unsigned int i = 0; i < fsMergedCode.size(); ++i)
                fsStringsC[i] = fsMergedCode[i].c_str();
            glShaderSource(vs, vsMergedCode.size(), vsStringsC, NULL);
            glShaderSource(fs, fsMergedCode.size(), fsStringsC, NULL);
            delete[] vsStringsC;
            delete[] fsStringsC;
        }
        else
        {
            const char *vsSourceC = vsCode.c_str();
            const char *fsSourceC = fsCode.c_str();
            glShaderSource(vs, 1, &vsSourceC, NULL);
            glShaderSource(fs, 1, &fsSourceC, NULL);
        }
        glCompileShader(vs);
        glCompileShader(fs);

        glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            glGetShaderInfoLog(vs, 1024, NULL, log);
            Log::Message("Vertex shader compilation error at: " + name + "!\n" + std::string(log), LOG_ERROR);
        }
        glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            glGetShaderInfoLog(fs, 1024, NULL, log);
            Log::Message("Fragment shader compilation error at: " + name + "!\n" + std::string(log), LOG_ERROR);
        }

        glAttachShader(ID, vs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &status);
        if (!status)
        {
            glGetProgramInfoLog(ID, 1024, NULL, log);
            Log::Message("Shader program linking error: \n" + std::string(log), LOG_ERROR);
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        // NOTE(Joey): query the number of active uniforms and attributes
        int nrAttributes, nrUniforms;
        glGetProgramiv(ID, GL_ACTIVE_ATTRIBUTES, &nrAttributes);
        glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &nrUniforms);
        Attributes.resize(nrAttributes);
        Uniforms.resize(nrUniforms);

        // NOTE(Joey): iterate over all active attributes
        char buffer[128];
        for (unsigned int i = 0; i < nrAttributes; ++i)
        {
            GLenum glType;
            glGetActiveAttrib(ID, i, sizeof(buffer), 0, &Attributes[i].Size, &glType, buffer);
            Attributes[i].Name = std::string(buffer);
            Attributes[i].Type = SHADER_TYPE_BOOL; // TODO(Joey): think of clean way to manage type conversions of OpenGL and custom type

            Attributes[i].Location = glGetAttribLocation(ID, buffer);
        }

        // NOTE(Joey): iterate over all active uniforms
        for (unsigned int i = 0; i < nrUniforms; ++i)
        {
            GLenum glType;
            glGetActiveUniform(ID, i, sizeof(buffer), 0, &Uniforms[i].Size, &glType, buffer);
            Uniforms[i].Name = std::string(buffer);
            Uniforms[i].Type = SHADER_TYPE_BOOL;  // TODO(Joey): think of clean way to manage type conversions of OpenGL and custom type

            Uniforms[i].Location = glGetUniformLocation(ID, buffer);
        }
    }
    // ------------------------------------------------------------------------
    void Shader::Use()
    {
        glUseProgram(ID);
    }
    // ------------------------------------------------------------------------
    bool Shader::HasUniform(std::string name)
    {
        for (unsigned int i = 0; i < Uniforms.size(); ++i)
        {
            if(Uniforms[i].Name == name)
                return true;
        }
        return false;
    }
    // ------------------------------------------------------------------------
    void Shader::SetInt(std::string location, int value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniform1i(loc, value);
       /* else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetBool(std::string location, bool value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniform1i(loc, (int)value);
      /*  else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetFloat(std::string location, float value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniform1f(loc, value);
       /* else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetVector(std::string location, math::vec2 value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniform2fv(loc, 1, &value[0]);
       /* else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetVector(std::string location, math::vec3 value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniform3fv(loc, 1, &value[0]);
      /*  else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetVector(std::string location, math::vec4 value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniform4fv(loc, 1, &value[0]);
       /* else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetMatrix(std::string location, math::mat2 value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniformMatrix2fv(loc, 1, GL_FALSE, &value[0][0]);
      /*  else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetMatrix(std::string location, math::mat3 value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniformMatrix3fv(loc, 1, GL_FALSE, &value[0][0]);
       /* else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    void Shader::SetMatrix(std::string location, math::mat4 value)
    {
        int loc = getUniformLocation(location);
        if (loc >= 0)
            glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
       /* else
            Log::Message("Shader uniform at location: " + location + " not found in shader.", LOG_WARNING);*/
    }
    // ------------------------------------------------------------------------
    int Shader::getUniformLocation(std::string name)
    {
        // TODO(Joey): read from uniform/attribute array as originally obtained from OpenGL
        for (unsigned int i = 0; i < Uniforms.size(); ++i)
        {
            if(Uniforms[i].Name == name)
                return Uniforms[i].Location;
        }
        return -1;
    }


}