include_guard(GLOBAL)

# https://gist.github.com/Qix-/f4090181e55ea365633da8c3d0ab5249

# LICENSE: CC0 - go nuts.

# Hi :) This is what I used to generate the ICNS for my game, Tide.
# Both input formats (SVG vs PNG) work just fine, but in my experience
# the SVG came out with noticeably better results (although PNG wasn't
# a catastrophe either). The logo for the game was simple enough that
# SVG was indeed an option.

# To use:
#
#    make_icns( INPUT "path/to/img.{svg,png}"
#               OUTPUT ICNS_PATH )
#
# Then add it as a custom target or use it as a
# dependency somewhere - I give you that option.
#
# For example:
#
#    add_custom_target( my-icns ALL DEPENDS "${ICNS_PATH}" )
#
# For the associated utilities:
#
# - PNG: brew install imagemagick
# - SVG: brew cask install inkscape
#
# Enjoy!

function (make_icns_from_png)
	cmake_parse_arguments (
		ARG
		""             # Boolean args
		"INPUT;OUTPUT" # List of single-value args
		""             # Multi-valued args
		${ARGN})

	find_program (
		magick
		NAMES "magick"
		DOC "Path to ImageMagick")
	if (NOT magick)
		message (FATAL_ERROR "Could not find ImageMagick - is ImageMagick installed?")
	endif ()

	get_filename_component (ARG_INPUT_ABS "${ARG_INPUT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component (ARG_INPUT_ABS_BIN "${ARG_INPUT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	get_filename_component (ARG_INPUT_FN "${ARG_INPUT_ABS_BIN}" NAME_WE)
	get_filename_component (ARG_INPUT_DIR "${ARG_INPUT_ABS_BIN}" DIRECTORY)

	set (sourceimg "${ARG_INPUT_ABS}")

	set (basepath "${ARG_INPUT_DIR}/${ARG_INPUT_FN}")
	set (output_icns "${basepath}.icns")
	set (iconset "${basepath}.iconset")

	set (deplist "")

	make_directory("${iconset}/")

	foreach (size IN ITEMS 16 32 128 256 512)
		math (EXPR size2x "2 * ${size}")

		set (ipath "${iconset}/icon_${size}x${size}.png")
		set (ipath2x "${iconset}/icon_${size}x${size}@2x.png")

		list (APPEND deplist "${ipath}" "${ipath2x}")

		execute_process(
			COMMAND "${magick}" "${sourceimg}" -resize "${size}x${size}" "${ipath2x}")

		execute_process(
			COMMAND "${magick}" "${sourceimg}" -resize "${size2x}x${size2x}" "${ipath2x}")
	endforeach ()

	execute_process(
		COMMAND iconutil --convert icns --output "${output_icns}" "${iconset}")

	if (ARG_OUTPUT)
		file(RENAME "${output_icns}" "${ARG_OUTPUT}")
	endif ()

	file(REMOVE_RECURSE "${iconset}/")
endfunction ()
