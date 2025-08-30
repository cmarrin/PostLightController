# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

set(BaseDir ../..)
set(ESPlibDir ../../../ESPlib)
set(CloverDir ../../../Clover/Runtime)
set(FormatDir ../../../Format)

idf_component_register(
    SRCS
        main.cpp
        ${BaseDir}/PostLightController.cpp
        ${BaseDir}/InterpretedEffect.cpp
        ${BaseDir}/Flash.cpp
        ${ESPlibDir}/Application.cpp
        ${ESPlibDir}/BrightnessManager.cpp
        ${ESPlibDir}/ButtonManager.cpp
        ${ESPlibDir}/Clock.cpp
        ${ESPlibDir}/LittleFSShim.cpp
        ${ESPlibDir}/JsonStreamingParser.cpp
        ${ESPlibDir}/TimeWeatherServer.cpp
        ${ESPlibDir}/WebFileSystem.cpp
        ${CloverDir}/Interpreter.cpp
        ${CloverDir}/NativeCoreRuntime.cpp
        ${FormatDir}/Format.cpp
        
    INCLUDE_DIRS
        ${BaseDir}
        ${ESPlibDir}
        ${CloverDir}
        ${FormatDir}

    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
