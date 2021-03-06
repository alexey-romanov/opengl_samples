#pragma optimize("", off)

#include <iostream>
#include <vector>
#include <chrono>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <GL/glew.h>

// Imgui + bindings
#include "imgui.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_opengl3.h"

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// STB, load images
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// Math constant and routines for OpenGL interop
#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "opengl_shader.h"

#include "obj_model.h"

static void glfw_error_callback(int error, const char *description)
{
   throw std::runtime_error(fmt::format("Glfw Error {}: {}\n", error, description));
}

void create_quad(GLuint &vbo, GLuint &vao, GLuint &ebo)
{
   // create the triangle
   const float vertices[] = {
       -1, 1,
       0, 1,

       -1, -1,
       0, 0,

       1, -1,
       1, 0,

       1, 1,
       1, 1,
   };
   const unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

   glGenVertexArrays(1, &vao);
   glGenBuffers(1, &vbo);
   glGenBuffers(1, &ebo);
   glBindVertexArray(vao);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
   glEnableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
}

struct render_target_t
{
   render_target_t(int res_x, int res_y);
   ~render_target_t();

   GLuint fbo_;
   GLuint color_, depth_;
   int width_, height_;
};

render_target_t::render_target_t(int res_x, int res_y)
{
   width_ = res_x;
   height_ = res_y;

   glGenTextures(1, &color_);
   glBindTexture(GL_TEXTURE_2D, color_);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, res_x, res_y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

   glGenTextures(1, &depth_);
   glBindTexture(GL_TEXTURE_2D, depth_);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, res_x, res_y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &fbo_);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

   glFramebufferTexture2D(GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D,
      color_,
      0);

   glFramebufferTexture2D(GL_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT,
      GL_TEXTURE_2D,
      depth_,
      0);

   GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   if (status != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("Framebuffer incomplete");

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

render_target_t::~render_target_t()
{
   glDeleteFramebuffers(1, &fbo_);
   glDeleteTextures(1, &depth_);
   glDeleteTextures(1, &color_);
}


int main(int, char **)
{
   try
   {
      // Use GLFW to create a simple window
      glfwSetErrorCallback(glfw_error_callback);
      if (!glfwInit())
         throw std::runtime_error("glfwInit error");

      // GL 3.3 + GLSL 330
      const char *glsl_version = "#version 330";
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
      //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

      // Create window with graphics context
      GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui - Conan", NULL, NULL);
      if (window == NULL)
         throw std::runtime_error("Can't create glfw window");

      glfwMakeContextCurrent(window);
      glfwSwapInterval(1); // Enable vsync

      if (glewInit() != GLEW_OK)
         throw std::runtime_error("Failed to initialize glew");

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      auto bunny = create_model("bunny.obj");
      render_target_t rt(512, 512);

      GLuint vbo, vao, ebo;
      create_quad(vbo, vao, ebo);

      // init shader
      shader_t quad_shader("simple-shader.vs", "simple-shader.fs");
      shader_t bunny_shader("model.vs", "model.fs");

      // Setup GUI context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO &io = ImGui::GetIO();
      ImGui_ImplGlfw_InitForOpenGL(window, true);
      ImGui_ImplOpenGL3_Init(glsl_version);
      ImGui::StyleColorsDark();

      auto const start_time = std::chrono::steady_clock::now();

      while (!glfwWindowShouldClose(window))
      {
         glfwPollEvents();

         // Get windows size
         int display_w, display_h;
         glfwGetFramebufferSize(window, &display_w, &display_h);


         // Gui start new frame
         ImGui_ImplOpenGL3_NewFrame();
         ImGui_ImplGlfw_NewFrame();
         ImGui::NewFrame();

         // GUI
         //ImGui::Begin("Triangle Position/Color");
         //static float rotation = 0.0;
         //ImGui::SliderFloat("rotation", &rotation, 0, 2 * glm::pi<float>());
         //static float translation[] = { 0.0, 0.0 };
         //ImGui::SliderFloat2("position", translation, -1.0, 1.0);
         //static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
         //ImGui::ColorEdit3("color", color);
         //ImGui::End();

         float const time_from_start = (float)(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count() / 1000.0);


         // Render offscreen
         {
            auto model = glm::rotate(glm::mat4(1), glm::radians(time_from_start * 10), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(7, 7, 7));
            auto view = glm::lookAt<float>(glm::vec3(0, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            auto projection = glm::perspective<float>(90, float(rt.width_) / rt.height_, 0.1, 100);
            auto mvp = projection * view * model;


            glBindFramebuffer(GL_FRAMEBUFFER, rt.fbo_);
            glViewport(0, 0, rt.width_, rt.height_);
            glEnable(GL_DEPTH_TEST);
            glColorMask(1, 1, 1, 1);
            glDepthMask(1);
            glDepthFunc(GL_LEQUAL);

            glClearColor(0.3, 0.3, 0.3, 1);
            glClearDepth(1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            bunny_shader.use();
            bunny_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
            bunny_shader.set_uniform("u_model", glm::value_ptr(model));

            glm::vec3 light_dir = glm::rotateY(glm::vec3(1, 0, 0), glm::radians(time_from_start * 60));

            bunny_shader.set_uniform<float>("u_color", 0.83, 0.64, 0.31);
            bunny_shader.set_uniform<float>("u_light", light_dir.x, light_dir.y, light_dir.z);
            bunny->draw();

            glDisable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
         }

         // Render main
         {
            auto model = glm::rotate<float>(glm::mat4(1), 0.1 * (-1 + 2 * cos(time_from_start) * cos(time_from_start)), glm::vec3(0, 1, 0));// *glm::scale(glm::vec3(7, 7, 7));
            auto view = glm::lookAt<float>(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            auto projection = glm::perspective<float>(90, float(display_w) / display_h, 0.1, 100);
            auto mvp = projection * view * model;

            glViewport(0, 0, display_w, display_h);

            glClearColor(0.30f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);

            quad_shader.use();
            quad_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
            quad_shader.set_uniform("u_tex", int(0));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rt.color_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
         }

         // Generate gui render commands
         ImGui::Render();

         // Execute gui render commands using OpenGL backend
         ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

         // Swap the backbuffer with the frontbuffer that is used for screen display
         glfwSwapBuffers(window);
      }

      // Cleanup
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();

      glfwDestroyWindow(window);
      glfwTerminate();
   }
   catch (std::exception const & e)
   {
      spdlog::critical("{}", e.what());
      return 1;
   }

   return 0;
}
