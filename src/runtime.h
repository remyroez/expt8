#pragma once

#include <array>
#include <span>

namespace expt8 {

using coordinate_t = int32_t;
using index_t = uint8_t;
using attribute_t = uint8_t;

using pixel_t = index_t;
using palette_color_index_t = index_t;
using hardware_color_index_t = index_t;
using hardware_color_t = uint32_t;

struct palette {
	static constexpr size_t num_colors = 4;
	std::array<index_t, num_colors> colors;

	auto color(size_t index) const { return colors[index % num_colors]; }
};

struct pattern {
	static constexpr size_t width = 8;
	static constexpr size_t height = 8;
	static constexpr size_t num_pixels = width * height;

	std::array<pixel_t, num_pixels> pixels;

	pixel_t pixel(size_t position) const {
		return pixels[position % num_pixels];
	}

	pixel_t pixel(size_t x, size_t y) const {
		return pixel((y % height) * width + (x % width));
	}
};

struct pattern_table {
	static constexpr size_t num_patterns = 256;

	std::array<pattern, num_patterns> patterns;

	const pattern &get_pattern(size_t position) const {
		return patterns[position % num_patterns];
	}

	pixel_t get_pixel(size_t position, size_t x, size_t y) const {
		return get_pattern(position).pixel(x, y);
	}
};

struct block {
	static constexpr size_t width = 2;
	static constexpr size_t height = 2;
	static constexpr size_t num_palettes = width * height;
	std::array<index_t, num_palettes> palette_indices;

	const auto &get(size_t position) const {
		return palette_indices[position];
	}

	const auto &get(size_t x, size_t y) const {
		return get((y % height) * width + (x % width));
	}
};

struct block_table {
	static constexpr size_t width = 16;
	static constexpr size_t height = 15;
	static constexpr size_t num_blocks = width * height;

	std::array<block, num_blocks> blocks;

	const auto &get(size_t position) const {
		return blocks[position];
	}

	const auto &get(size_t x, size_t y) const {
		return get((y % height) * width + (x % width));
	}
};

struct tile_table {
	static constexpr size_t width = 32;
	static constexpr size_t height = 30;
	static constexpr size_t num_tiles = width * height;

	std::array<index_t, num_tiles> tile_indices;

	auto get(size_t position) const {
		return tile_indices[position];
	}

	auto get(size_t x, size_t y) const {
		return get((y % height) * width + (x % width));
	}
};

struct name_table {
	tile_table tile_table;
	block_table block_table;

	auto get(size_t x, size_t y, index_t &out_tile_index, index_t &out_palette_index) const {
		auto tile_x = x / pattern::width;
		auto tile_y = y / pattern::height;
		out_tile_index = tile_table.get(tile_x, tile_y);

		auto block_x = x / block_table::width;
		auto block_y = y / block_table::height;
		auto &block = block_table.get(block_x, block_y);

		auto palette_x = tile_x / block::width;
		auto palette_y = tile_y / block::height;
		out_palette_index = block.get(block_x, block_y);

		return true;
	}
};

struct background_plane {
	static constexpr size_t width = 2;
	static constexpr size_t height = 2;
	static constexpr size_t num_name_tables = width * height;
	static constexpr size_t num_palettes = 4;

	std::array<name_table, num_name_tables> name_tables;
	std::array<palette, num_palettes> palettes;
	pattern_table pattern_table;

	const auto &get_name_table(size_t position) const {
		return name_tables[position];
	}

	const auto &get_name_table(size_t x, size_t y) const {
		return get_name_table((y % height) * width + (x % width));
	}

	const auto &get_palette(size_t position) const {
		return palettes[position % num_palettes];
	}

	auto render(size_t x, size_t y) const {
		auto name_table_x = x / width;
		auto name_table_y = y / height;
		auto &name_table = get_name_table(name_table_x, name_table_y);

		auto name_x = x % width;
		auto name_y = y % height;
		index_t tile_index = 0;
		index_t palette_index = 0;
		name_table.get(name_x, name_y, tile_index, palette_index);

		auto tile_x = x % pattern::width;
		auto tile_y = y % pattern::height;
		auto palette_color_index = pattern_table.get_pixel(tile_index, tile_x, tile_y);

		auto &palette = get_palette(palette_index);
		return palette.color(palette_color_index);
	}
};

struct sprite {
	enum attribute {
		_priority_back,
		_flip_horizontally,
		_flip_vertically,

		priority_back = 1 << _priority_back,
		flip_horizontally = 1 << _flip_horizontally,
		flip_vertically = 1 << _flip_vertically,
	};

	coordinate_t x = 0;
	coordinate_t y = 0;
	index_t tile_index = 0;
	index_t palette_index = 0;
	attribute_t attributes = 0;
};

struct sprite_plane {
	static constexpr size_t num_sprites = 64;
	static constexpr size_t num_palettes = 4;

	std::array<sprite, num_sprites> sprites;
	std::array<palette, num_palettes> palettes;
	pattern_table pattern_table;

	const auto *find_sprite(coordinate_t x, coordinate_t y) const {
		const sprite *found = nullptr;
		for (auto &it : sprites) {
			if (it.tile_index == 0xFF) {
				// unused
			} else if (x < it.x) {
				// no hit
			} else if (y < it.y) {
				// no hit
			} else if (x > (it.x + pattern::width)) {
				// no hit
			} else if (y > (it.y + pattern::height)) {
				// no hit
			} else {
				found = &it;
				break;
			}
		}
		return found;
	}

};

class ppu {
public:
	template<std::size_t ExtentF, std::size_t ExtentP>
	bool render(
		std::span<pixel_t, ExtentF> framebuffer,
		std::span<hardware_color_t, ExtentP> hardware_palette
	) {

	}

private:
	background_plane _background_plane;
	sprite_plane _sprite_plane;
};

class runtime {
public:
public:
	runtime() {}

public:
	template<std::size_t ExtentF, std::size_t ExtentP>
	bool render_picture(
		std::span<pixel_t, ExtentF> &&framebuffer,
		std::span<hardware_color_t, ExtentP> &&hardware_palette
	) {
		return _ppu(std::move(framebuffer), std::move(hardware_palette));
	}

private:
	ppu _ppu;
};

} // namespace expt8
