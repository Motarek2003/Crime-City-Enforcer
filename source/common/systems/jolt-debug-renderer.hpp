#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "../common/shader/shader.hpp"
#include <vector>

// Jolt Headers
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h> 

namespace our {

    class JoltDebugRenderer : public JPH::DebugRendererSimple {
    private:
        // OpenGL Resources
        GLuint lineVAO = 0;
        GLuint lineVBO = 0;
        ShaderProgram* debugShader = nullptr;

        struct LineVertex {
            glm::vec3 position;
            glm::vec4 color;
        };


        std::vector<LineVertex> lineVertices;

        glm::mat4 viewProjection;

    public:
        JoltDebugRenderer();
        ~JoltDebugRenderer();

        void Initialize();
        void Cleanup();
        void Clear();
        void Render();
        
        bool IsInitialized() const { return debugShader != nullptr; }
        void SetViewProjection(const glm::mat4& vp) { viewProjection = vp; }


        // 1. Lines
        virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

        // 2. Text empty
        virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, 
                               JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override;

    private:
        // Helpers
        glm::vec4 JoltColorToGLM(JPH::ColorArg color) {
            return glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        }
        glm::vec3 JoltVec3ToGLM(JPH::RVec3Arg v) {
            return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
        }
    };
}