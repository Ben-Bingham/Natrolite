#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"

#include "Utilities/OpenGl/Context.h"
#include "Utilities/OpenGl/Shader.h"
#include "Utilities/OpenGl/Texture.h"
#include "Utilities/OpenGl/SSBO.h"
#include "Utilities/ImGui/ImGuiInstance.h"
#include "Utilities/OpenGl/VertexAttributeObject.h"
#include "Utilities/OpenGl/Buffer.h"
#include "Utilities/OpenGl/Framebuffer.h"
#include "Utilities/OpenGl/Renderbuffer.h"
#include "Utilities/Camera.h"
#include "Utilities/ReadFile.h"

#include "Editor.h"

Camera cam{ };

std::shared_ptr<Window> window{ };

glm::ivec2 imGuiWindowSize{ 100, 100 };
glm::ivec2 lastImGuiWindowSize{ imGuiWindowSize };
glm::ivec2 viewPortOffset{ };

std::chrono::duration<float> lastCompile{ 0.0f };

int main() {
    window = std::make_shared<Window>(glm::ivec2{ 1600, 1000 }, "Natrolite");

    Context context{ *window };
   
    ImGuiInstance imGui{ };
    imGui.Init(window->handle);

    std::unique_ptr<Shader> mainShader{ };
    auto start = std::chrono::steady_clock::now();
    mainShader = std::make_unique<Shader>("assets\\shaders\\main.vert", "assets\\shaders\\main.frag");
    lastCompile = std::chrono::steady_clock::now() - start;

    mainShader->Bind();
    mainShader->SetVec4("color", glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });

    Framebuffer framebuffer{ };
    framebuffer.Bind();

    Texture2D texture{ imGuiWindowSize, TextureParameters{
        TextureFormat::RGB,
        TextureStorageType::UNSIGNED_BYTE,
        TextureWrapMode::REPEAT,
        TextureFilteringMode::LINEAR
    } };

    framebuffer.AddTexture(texture, Framebuffer::TextureUses::COLOR_0);

    Renderbuffer renderbuffer{ imGuiWindowSize };

    framebuffer.AddRenderbuffer(renderbuffer, Framebuffer::RenderbufferUses::DEPTH_STENCIL);

    if (!framebuffer.Check()) {
        std::cout << "ERROR, Framebuffer is not complete" << std::endl;
    }

    framebuffer.Unbind();
    texture.Unbind();
    renderbuffer.Unbind();

    std::vector<float> vertices{
        -0.5f, 0.0f, 0.0f,
        0.5f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f
    };

    std::vector<unsigned int> indices{
        1, 2, 3
    };

    VertexAttributeObject vao{ };

    VertexBufferObject vbo{ vertices };
    ElementBufferObject ebo{ indices };

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
    glEnableVertexAttribArray(0);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window->handle)) {
        float currentFrame = (float)glfwGetTime();

        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        imGui.StartNewFrame();
        
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Show GUI
        { ImGui::Begin("Preview");
            imGuiWindowSize = glm::ivec2{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };

            if (imGuiWindowSize != lastImGuiWindowSize && !(imGuiWindowSize.x == 0 || imGuiWindowSize.y == 0)) {
                framebuffer.Bind();

                texture.Bind();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imGuiWindowSize.x, imGuiWindowSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.Get(), 0);

                renderbuffer.Bind();
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, imGuiWindowSize.x, imGuiWindowSize.y);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.Get());

                framebuffer.Check();

                framebuffer.Unbind();
                texture.Unbind();
                renderbuffer.Unbind();

                lastImGuiWindowSize = imGuiWindowSize;
            }

            viewPortOffset = glm::ivec2{ (int)ImGui::GetCursorPos().x, (int)ImGui::GetCursorPos().y };
            ImGui::Image((ImTextureID)texture.Get(), ImVec2{ (float)imGuiWindowSize.x, (float)imGuiWindowSize.y });

        } ImGui::End();

        { ImGui::Begin("Editor");
            Natrolite::Editor();
        } ImGui::End();

        ImGui::ShowDemoWindow();

        { ImGui::Begin("Settings");
            if (ImGui::Button("Recompile")) {
                mainShader.reset();

                auto start = std::chrono::steady_clock::now();
                mainShader = std::make_unique<Shader>("assets\\shaders\\main.vert", "assets\\shaders\\main.frag");
                lastCompile = std::chrono::steady_clock::now() - start;

                mainShader->Bind();
                mainShader->SetVec4("color", glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
            }

            std::chrono::microseconds timeInMs = std::chrono::duration_cast<std::chrono::microseconds>(lastCompile);
            ImGui::Text("Last Compilation Took: %d microseconds", timeInMs.count());
        } ImGui::End();

        if (glfwGetKey(window->handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window->handle, true);
        }

        glViewport(0, 0, imGuiWindowSize.x, imGuiWindowSize.y);
        framebuffer.Bind();

        // Prep for rendering
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model{ 1.0f };
        glm::mat4 view = cam.ViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)imGuiWindowSize.x / (float)imGuiWindowSize.y, 0.01f, 100.0f);

        glm::mat4 mvp = projection * view * model;

        mainShader->Bind();
        mainShader->SetMat4("mvp", mvp);

        glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, nullptr);

        framebuffer.Unbind();

        // Finish the GUI frame
        imGui.FinishFrame();

        // Finish the frame
        glfwSwapBuffers(window->handle);
        glfwPollEvents();
    }

    imGui.Cleanup();
}
