#include "jolt-debug-renderer.hpp"
#include <iostream>

namespace our {

    JoltDebugRenderer::JoltDebugRenderer() : JPH::DebugRendererSimple() {
        viewProjection = glm::mat4(1.0f);
    }

    JoltDebugRenderer::~JoltDebugRenderer() {
        Cleanup();
    }

    void JoltDebugRenderer::Initialize() {
            
            debugShader = new ShaderProgram();
            
            debugShader->attach("assets/shaders/debug.vert", GL_VERTEX_SHADER);
            debugShader->attach("assets/shaders/debug.frag", GL_FRAGMENT_SHADER);
            debugShader->link();


            glGenVertexArrays(1, &lineVAO);
            glGenBuffers(1, &lineVBO);

            glBindVertexArray(lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

            // Position attribute (location 0)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
            glEnableVertexAttribArray(0);

            // Color attribute (location 1)
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
    }

    void JoltDebugRenderer::Cleanup() {
        if (lineVAO) {
            glDeleteVertexArrays(1, &lineVAO);
            lineVAO = 0;
        }
        if (lineVBO) {
            glDeleteBuffers(1, &lineVBO);
            lineVBO = 0;
        }
        if (debugShader) {
            delete debugShader;
            debugShader = nullptr;
        }
    }

    void JoltDebugRenderer::Clear() {
        lineVertices.clear();
    }

    void JoltDebugRenderer::Render() {
        if (!debugShader) return;

        debugShader->use();
        debugShader->set("VP", viewProjection);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (!lineVertices.empty()) {
            glBindVertexArray(lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(LineVertex), 
                        lineVertices.data(), GL_DYNAMIC_DRAW);

            glDrawArrays(GL_LINES, 0, lineVertices.size());
            glBindVertexArray(0);
        }

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }


    void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) {
        glm::vec4 color = JoltColorToGLM(inColor);
        lineVertices.push_back({ JoltVec3ToGLM(inFrom), color });
        lineVertices.push_back({ JoltVec3ToGLM(inTo), color });
    }

    void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, 
                                       JPH::ColorArg inColor, float inHeight) {
    }

} 