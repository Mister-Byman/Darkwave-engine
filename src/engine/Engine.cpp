#include "engine/Engine.h"
#include <iostream>
#include "math/Mat4.h"
#include <cmath>

bool Engine::init() {
    bool fullscreen = false; // стартуем в окне
    if (!window_.create("cs_like", 1280, 720, fullscreen)) return false;

    time_.start();
    input_.setRelativeMouse(true); // как в шутере сразу
    player_.position = { 0.0f, 0.0f, 3.0f }; // старт на полу
    cam_.setPosition({ player_.position.x, player_.position.y + player_.eyeHeight, player_.position.z });
    cam_.setMoveSpeed(4.0f);
    cam_.setMouseSensitivity(0.0025f);

    if (!vk_.init(window_.sdl())) return false;
    if (!renderer_.init(vk_, window_.width(), window_.height())) return false;

    running_ = true;
    std::cout << "Engine started\n";
    return true;
}

void Engine::run() {
    while (running_) {
        time_.tick();
        input_.beginFrame();

        window_.pollEvents(running_, [&](const SDL_Event& e) {
            input_.handleEvent(e);
            });

        // F11 fullscreen toggle (как раньше)
        if (window_.consumeToggleFullscreenRequested()) {
            window_.toggleFullscreen();
            renderer_.recreateSwapchain(vk_, window_.width(), window_.height());
            window_.resetResizedFlag();
        }

        if (input_.keyPressed(SDL_SCANCODE_F6)) {
            renderer_.setPreferredPresentMode(Swapchain::PresentMode::FIFO);
            renderer_.recreateSwapchain(vk_, window_.width(), window_.height());
        }
        if (input_.keyPressed(SDL_SCANCODE_F7)) {
            renderer_.setPreferredPresentMode(Swapchain::PresentMode::MAILBOX);
            renderer_.recreateSwapchain(vk_, window_.width(), window_.height());
        }
        if (input_.keyPressed(SDL_SCANCODE_F8)) {
            renderer_.setPreferredPresentMode(Swapchain::PresentMode::IMMEDIATE);
            renderer_.recreateSwapchain(vk_, window_.width(), window_.height());
        }


// Look (мышь крутит взгляд)
        cam_.updateLook(input_.mouse().dx, input_.mouse().dy);

        // Сбор wishDir в мировых координатах (движение по XZ)
        Vec3 wish{ 0.0f, 0.0f, 0.0f };

        Vec3 fwd = cam_.forward();
        fwd.y = 0.0f;
        fwd = normalize(fwd);

        Vec3 rig = cam_.right();
        rig.y = 0.0f;
        rig = normalize(rig);

        if (input_.keyDown(SDL_SCANCODE_W)) wish += fwd;
        if (input_.keyDown(SDL_SCANCODE_S)) wish += (fwd * -1.0f);
        if (input_.keyDown(SDL_SCANCODE_D)) wish += rig;
        if (input_.keyDown(SDL_SCANCODE_A)) wish += (rig * -1.0f);

        // Прыжок — строго по нажатию в этом кадре
        bool jumpPressed = input_.keyPressed(SDL_SCANCODE_SPACE);

        // Обновляем игрока
        player_.update((float)time_.deltaSeconds(), wish, jumpPressed);

        // Камера сидит в "голове"
        cam_.setPosition({ player_.position.x, player_.position.y + player_.eyeHeight, player_.position.z });


        static double acc = 0.0;
        acc += time_.deltaSeconds();
        if (acc > 0.2) {
            acc = 0.0;
            auto p = cam_.position();
            SDL_Log("pos: %.2f %.2f %.2f yaw=%.2f pitch=%.2f", p.x, p.y, p.z, cam_.yaw(), cam_.pitch());
        }

        Vec3 eye = cam_.position();
        Vec3 center = eye + cam_.forward();
        Mat4 view = Mat4::lookAtRH(eye, center, { 0,1,0 });

        float aspect = (float)window_.width() / (float)window_.height();
        Mat4 proj = Mat4::perspectiveRH_ZO(70.0f * 3.1415926f / 180.0f, aspect, 0.1f, 100.0f);

        // Vulkan: обычно нужно инвертировать Y в projection
        proj.m[5] *= -1.0f;

        renderer_.setViewProj(view, proj);

        Mat4 cubeModel = Mat4::translation( 0.0f, 0.5f, 0.0f );
        renderer_.setCubeModel(cubeModel);

        // Коллайдер вокруг куба (куб 1x1x1, центр в (0,0.5,0))
        player_.wallBox.min = { -0.6f, 0.0f, -0.6f };
        player_.wallBox.max = { 0.6f, 1.2f,  0.6f };

        // Рендер
        bool ok = renderer_.drawFrame(vk_);
        if (!ok || window_.wasResized()) {
            window_.resetResizedFlag();
            renderer_.recreateSwapchain(vk_, window_.width(), window_.height());
        }
    }



}

void Engine::shutdown() {
    renderer_.shutdown(vk_);
    vk_.shutdown();
    window_.destroy();
}
