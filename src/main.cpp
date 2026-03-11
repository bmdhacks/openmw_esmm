#include <iostream>
#include <thread>
#include <chrono>
#include <dlfcn.h>

#include "AppContext.h"
#include "utils/Logger.h"
#include "utils/Utils.h"
#include "core/StateMachine.h"
#include "core/ModEngine.h"
#include "scenes/MainMenuScene.h"

// ImGui Includes
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

// Boost Includes for command-line parsing
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;


int main(int argc, char* argv[]) {
    void* gl_handle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
    if (!gl_handle) {
        std::cerr << "Warning: Could not pre-load libGL.so.1: " << dlerror() << std::endl;
        // This is not fatal; we can still let SDL try to load it.
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        // Use std::cerr here because the logger might not be set up yet.
        std::cerr << "FATAL: SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "FATAL: SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // --- Command-Line Argument Parsing ---
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "show help message")
        ("7zz", po::value<std::string>(), "Path to 7zzs executable")
        ("base-dir", po::value<std::string>(), "Base directory for all relative paths")
        ("mod-archive-dir", po::value<std::string>(), "Directory for mod archives")
        ("mod-data-dir", po::value<std::string>(), "Directory for extracted mod data")
        ("openmw-cfg-dir", po::value<std::string>(), "Directory containing openmw.cfg")
        ("esmm-cfg-dir", po::value<std::string>(), "Directory for esmm configs (ini)")
        ("esmm-utils-dir", po::value<std::string>(), "Directory for esmm utility scripts")
        ("esmm-tweaks-dir", po::value<std::string>(), "Directory for esmm tweak scripts")
        ("esmm-extras-dir", po::value<std::string>(), "Directory for esmm extra scripts")
        ("quiet", "Quieten down logging")
        ("verbose", "Enable verbose debug logging");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("quiet")) {
        Logger::get().set_level(LogLevel::WARN);
    }
    if (vm.count("verbose")) {
        Logger::get().set_level(LogLevel::DEBUG);
        LOG_DEBUG("Verbose logging enabled.");
    }

    AppContext ctx;
    // --- Path Initialization ---
    // 1. Establish the base path (defaults to current working directory)
    ctx.path_base = vm.count("base-dir") ? fs::path(vm["base-dir"].as<std::string>()) : fs::current_path();

    // Helper lambda to resolve paths
    auto resolve_path = [&](const std::string& arg_name, const fs::path& default_path) {
        if (vm.count(arg_name)) {
            fs::path p(vm[arg_name].as<std::string>());
            return p.is_absolute() ? p : fs::absolute(p, ctx.path_base);
        }
        return default_path;
    };

    // 2. Resolve all other paths based on the base path or command-line overrides
    ctx.path_esmm_cfg    = resolve_path("esmm-cfg-dir",    ctx.path_base / "esmm/");
    ctx.path_esmm_utils  = resolve_path("esmm-utils-dir",  ctx.path_esmm_cfg / "utils/");
    ctx.path_esmm_tweaks = resolve_path("esmm-tweaks-dir", ctx.path_esmm_cfg / "tweaks/");
    ctx.path_esmm_extras = resolve_path("esmm-extras-dir", ctx.path_esmm_cfg / "extras/");

    ctx.path_mod_archives = resolve_path("mod-archive-dir", ctx.path_base / "mods/");
    ctx.path_mod_data     = resolve_path("mod-data-dir",    ctx.path_base / "mod_data/");

    // Special handling for openmw.cfg itself, not just its directory
    if (vm.count("openmw-cfg-dir")) {
        fs::path p(vm["openmw-cfg-dir"].as<std::string>());
        p = p.is_absolute() ? p : fs::absolute(p, ctx.path_base);
        ctx.path_openmw_cfg = p / "openmw.cfg";
    } else {
        ctx.path_openmw_cfg = ctx.path_base / "openmw" / "openmw.cfg";
    }

    // 3. Find 7zz executable
    if (vm.count("7zz")) {
        ctx.exec_7zz = fs::path(vm["7zz"].as<std::string>());
    } else {
        ctx.exec_7zz = find_executable_on_path("7zzs");
        if (ctx.exec_7zz.empty()) {
             ctx.exec_7zz = find_executable_on_path("7z"); // Fallback
        }
    }

    if (!fs::exists(ctx.exec_7zz)) {
        LOG_ERROR("Could not find 7zzs or 7z executable. Please place '7zzs' in the base directory or install 7-zip and ensure it's in your PATH. You can also specify the path with --7zz.");
        // We don't exit here, but extraction will fail later if attempted.
    }

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        LOG_ERROR("SDL_GetDesktopDisplayMode failed: ", SDL_GetError());
        dm.w = 640;
        dm.h = 480; // Fallback
    }
    LOG_INFO("Desktop resolution: ", dm.w, "x", dm.h);

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    const float BASE_WIDTH = 640.0f;
    const float BASE_HEIGHT = 480.0f;
    float logical_width = BASE_WIDTH;
    float logical_height = BASE_HEIGHT;

    float native_aspect = (float)dm.w / (float)dm.h;
    float base_aspect = BASE_WIDTH / BASE_HEIGHT;

    if (native_aspect > base_aspect) { // Wider screen
        logical_width = BASE_HEIGHT * native_aspect;
    } else { // Taller screen
        logical_height = BASE_WIDTH / native_aspect;
    }

    ctx.window = SDL_CreateWindow("OpenMW ESMM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dm.w, dm.h, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL);
    if (!ctx.window) {
        LOG_ERROR("Window could not be created! SDL_Error: ", SDL_GetError());
        return 1;
    }

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer) {
        LOG_ERROR("Renderer could not be created! SDL_Error: ", SDL_GetError());
        return 1;
    }

    LOG_INFO("Logical resolution: ", logical_width, "x", logical_height);
    SDL_RenderSetLogicalSize(ctx.renderer, logical_width, logical_height);

    ctx.font = TTF_OpenFont(FONT_PATH, 16);
    if (!ctx.font) {
        LOG_ERROR("Failed to load font! TTF_Error: ", TTF_GetError());
        return 1;
    }

    // --- ImGui Init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enables keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enables gamepad controls

    io.IniFilename = NULL; // Disable .ini file saving
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(ctx.window, ctx.renderer);
    ImGui_ImplSDLRenderer2_Init(ctx.renderer); // CHANGED: Call the v2 Init function

    // Controller Init
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i)) {
            ctx.controller = SDL_GameControllerOpen(i);
            if (ctx.controller) break;
        }
    }

    // --- Scene & Main Loop ---
    StateMachine machine(ctx);

    ctx.engine = &machine.get_engine(); 

    machine.push_scene(std::make_unique<MainMenuScene>(machine));

    // Framerate stuff.
    const int TARGET_FPS = 60;
    const int FRAME_DELAY = 1000 / TARGET_FPS;
    Uint32 frame_start;
    int frame_time;

    while (ctx.running) {
        frame_start = SDL_GetTicks();

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT)
            {
                ctx.running = false;
                ctx.exit_code = 255;
            }

            // Let the scene handle everything else
            machine.handle_event(e);
        }
        
        machine.update();

        // Rendering
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        int w, h;
        SDL_RenderGetLogicalSize(ctx.renderer, &w, &h);
        io.DisplaySize = ImVec2((float)w, (float)h);

        ImGui::NewFrame();
        machine.render();
        SDL_SetRenderDrawColor(ctx.renderer, 20, 20, 40, 255);
        SDL_RenderClear(ctx.renderer);
        ImGui::Render();

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx.renderer);
        SDL_RenderPresent(ctx.renderer);

        // Cap FPS
        frame_time = SDL_GetTicks() - frame_start;
        if (FRAME_DELAY > frame_time) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    // --- Shutdown ---
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    return ctx.exit_code;
}
