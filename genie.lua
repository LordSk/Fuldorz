--
dofile("config.lua");

PROJ_DIR = path.getabsolute("..")
BUILD_DIR = path.join(PROJ_DIR, "build")

solution "solution"
	location "build"
	
	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"x64"
	}
	
	language "C++"
	
	configuration {"Debug"}
		targetsuffix "_debug"
		flags {
			"Symbols"
		}
		defines {
			"DEBUG",
			"CONF_DEBUG"
		}
	
	configuration {"Release"}
		targetsuffix "_release"
		flags {
			"Optimize"
		}
		defines {
			"NDEBUG",
			"CONF_RELEASE"
		}
	
	configuration {}
	
	targetdir(BUILD_DIR)
	
	includedirs {
		"src",
		SDL2_include
	}
	
	links {
		"user32",
		"shell32",
		"winmm",
		"ole32",
		"oleaut32",
		"imm32",
		"version",
		"ws2_32",
		"advapi32",
        "gdi32",
		"glu32",
		"opengl32",
		"comctl32",
		SDL2_lib
	}
	
	flags {
		"NoExceptions",
		"NoRTTI",
	}
	
	defines {
		"_CRTDBG_MAP_ALLOC"
	}
	
	-- disable exception related warnings
	buildoptions{ "/wd4577", "/wd4530" }
	

	
project "fuldorz_app"
	kind "WindowedApp"

	configuration {}
	
	files {
		"src/*.h",
		"src/*.cpp",
		"src/imgui/*.h",
		"src/imgui/*.cpp",
	}