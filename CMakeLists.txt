# Fichier de construction : il dit à CMake comment fabriquer l'application.
cmake_minimum_required(VERSION 3.22)

project(MonDAW VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Télécharge JUCE automatiquement au premier "cmake" (pas besoin de l'installer à la main).
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(JUCE)

# Déclare notre application de bureau.
juce_add_gui_app(MonDAW
    PRODUCT_NAME "MonDAW"
    COMPANY_NAME "Jules"
)

# Génère JuceHeader.h, qui regroupe tous les modules JUCE.
juce_generate_juce_header(MonDAW)

target_sources(MonDAW PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/WaveformView.cpp
)

target_compile_definitions(MonDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MonDAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MonDAW,JUCE_VERSION>"
)

target_link_libraries(MonDAW
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
