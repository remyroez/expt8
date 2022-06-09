#pragma once

#include <array>
#include <vector>
#include <span>
#include <algorithm>

namespace expt8 {

using coordinate_t = int32_t;
using index_t = uint8_t;
using attribute_t = uint8_t;

using palette_color_index_t = index_t;
using hardware_color_index_t = index_t;

using color_t = hardware_color_index_t;
using pixel_t = palette_color_index_t;

struct palette {
	static constexpr size_t num_colors = 4;
	std::array<color_t, num_colors> colors;

	auto color(size_t index) const { return colors[index % num_colors]; }
	void color(size_t index, color_t new_color) { colors[index % num_colors] = new_color; }
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

	void write(std::span<pixel_t> src) {
		std::copy(src.begin(), src.end(), pixels.begin());
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

	void write(size_t position, std::span<pixel_t> &&src) {
		patterns[position % num_patterns].write(std::move(src));
	}
};

struct block {
	static constexpr size_t width = 2;
	static constexpr size_t height = 2;
	static constexpr size_t num_palettes = width * height;
	static constexpr size_t num_horizontal_tiles = 4;
	static constexpr size_t num_vertical_tiles = 4;

	std::array<index_t, num_palettes> palette_indices;
	index_t palette_index = 0;

	auto get(size_t position) const {
		return palette_indices[position % num_palettes];
	}

	auto set(size_t position, index_t index) {
		palette_indices[position % num_palettes] = index;
	}

	auto get(size_t x, size_t y) const {
		return get((y % height) * width + (x % width));
	}

	auto set(size_t x, size_t y, index_t index) {
		return set((y % height) * width + (x % width), index);
	}
};

struct block_table {
	static constexpr size_t width = 16;
	static constexpr size_t height = 15;
	static constexpr size_t num_blocks = width * height;

	std::array<block, num_blocks> blocks;

	auto &get(size_t position) const {
		return blocks[position];
	}

	auto &get(size_t position) {
		return blocks[position];
	}

	auto &get(size_t x, size_t y) const {
		return get((y % height) * width + (x % width));
	}

	auto &get(size_t x, size_t y) {
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

	auto set(size_t position, index_t index) {
		tile_indices[position] = index;
	}

	auto get(size_t x, size_t y) const {
		return get((y % height) * width + (x % width));
	}

	auto set(size_t x, size_t y, index_t index) {
		return set((y % height) * width + (x % width), index);
	}
};

struct name_table {
	tile_table tile_table;
	block_table block_table;

	auto get(size_t x, size_t y, index_t &out_tile_index, index_t &out_palette_index) const {
		auto tile_x = x / pattern::width;
		auto tile_y = y / pattern::height;
		out_tile_index = tile_table.get(tile_x, tile_y);

		auto block_x = x / (block::width * pattern::width);
		auto block_y = y / (block::height * pattern::height);
		auto &block = block_table.get(block_x, block_y);

		auto palette_x = tile_x / block::width;
		auto palette_y = tile_y / block::height;
		out_palette_index = block.palette_index;// get(palette_x, palette_y);

		return true;
	}

	auto set_tile(size_t x, size_t y, index_t index) {
		tile_table.set(x, y, index);
	}

	auto set_tile_palette(size_t x, size_t y, index_t index) {
		block_table.get(x / block::width, y / block::height).palette_index = index;// set(x % block::width, y % block::height, index);
	}
};

struct background_plane {
	static constexpr size_t width = 2;
	static constexpr size_t height = 2;
	static constexpr size_t num_name_tables = width * height;
	static constexpr size_t num_palettes = 4;
	static constexpr size_t pixel_width = tile_table::width * pattern::width;
	static constexpr size_t pixel_height = tile_table::height * pattern::height;

	std::array<name_table, num_name_tables> name_tables;
	std::array<palette, num_palettes> palettes;
	index_t pattern_table_index = 0;

	const auto &get_name_table(size_t position) const {
		return name_tables[position];
	}

	auto &get_name_table(size_t position) {
		return name_tables[position];
	}

	const auto &get_name_table(size_t x, size_t y) const {
		return get_name_table((y % height) * width + (x % width));
	}

	auto &get_name_table(size_t x, size_t y) {
		return get_name_table((y % height) * width + (x % width));
	}

	const auto &get_palette(size_t position) const {
		return palettes[position % num_palettes];
	}

	auto &get_palette(size_t position) {
		return palettes[position % num_palettes];
	}

	void set_palette(size_t palette_index, size_t palette_color_index, color_t new_color) {
		palettes[palette_index % palettes.size()].color(palette_color_index, new_color);
	}

	const auto &get(size_t x, size_t y, index_t &out_tile_index) const {
		auto name_table_x = x / pixel_width;
		auto name_table_y = y / pixel_height;
		auto &name_table = get_name_table(name_table_x, name_table_y);

		auto name_x = x % pixel_width;
		auto name_y = y % pixel_height;
		index_t tile_index = 0;
		index_t palette_index = 0;
		name_table.get(name_x, name_y, tile_index, palette_index);

		out_tile_index = tile_index;
		return get_palette(palette_index);
	}

	auto set_tile(size_t name_table_index, size_t x, size_t y, index_t index) {
		get_name_table(name_table_index).set_tile(x, y, index);
	}

	auto set_tile_palette(size_t name_table_index, size_t x, size_t y, index_t index) {
		get_name_table(name_table_index).set_tile_palette(x, y, index);
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
	index_t tile_index = 0xFF;
	index_t palette_index = 0;
	attribute_t attributes = 0;

	auto left() const { return x; }
	auto right() const { return left() + pattern::width; }
	auto top() const { return y; }
	auto bottom() const { return top() + pattern::height; }
	bool has_attribute(uint32_t attr) const { return (attributes & attr) != 0; }
};

struct sprite_plane {
	static constexpr size_t num_sprites = 64;
	static constexpr size_t num_palettes = 4;

	std::array<sprite, num_sprites> sprites;
	std::array<palette, num_palettes> palettes;
	index_t pattern_table_index = 0;

	const auto &get_sprite(size_t position) const {
		return sprites[position % sprites.size()];
	}

	void set_sprite(
		size_t position,
		coordinate_t x = 0,
		coordinate_t y = 0,
		index_t tile_index = 0,
		index_t palette_index = 0,
		attribute_t attributes = 0
	) {
		auto &target = sprites[position % sprites.size()];
		target.x = x;
		target.y = y;
		target.tile_index = tile_index;
		target.palette_index = palette_index;
		target.attributes = attributes;
	}

	const auto &get_palette(size_t position) const {
		return palettes[position % palettes.size()];
	}

	void set_palette(size_t palette_index, size_t palette_color_index, color_t new_color) {
		palettes[palette_index % palettes.size()].color(palette_color_index, new_color);
	}

	bool find_sprites(coordinate_t x, coordinate_t y, std::vector<const sprite *> &out_front_sprites, std::vector<const sprite *> &out_back_sprites) const {
		bool found = false;
		for (auto &it : sprites) {
			if (it.tile_index == 0xFF) {
				// unused
			} else if (x < it.x) {
				// no hit
			} else if (y < it.y) {
				// no hit
			} else if (x >= (it.x + pattern::width)) {
				// no hit
			} else if (y >= (it.y + pattern::height)) {
				// no hit
			} else {
				if (it.has_attribute(sprite::priority_back)) {
					out_back_sprites.push_back(&it);
				} else {
					out_front_sprites.push_back(&it);
				}
				found = true;
			}
		}
		return found;
	}

	bool find_sprites(coordinate_t y, std::vector<const sprite *> &out_front_sprites, std::vector<const sprite *> &out_back_sprites) const {
		bool found = false;
		for (auto &it : sprites) {
			if (it.tile_index == 0xFF) {
				// unused
			} else if (y < it.y) {
				// no hit
			} else if (y >= (it.y + pattern::height)) {
				// no hit
			} else {
				if (it.has_attribute(sprite::priority_back)) {
					out_back_sprites.push_back(&it);
				} else {
					out_front_sprites.push_back(&it);
				}
				found = true;
			}
		}
		return found;
	}
};

class ppu {
public:
	static constexpr size_t num_pattern_tables = 2;

public:
	bool render(std::span<color_t> framebuffer, size_t width, size_t height) {
		std::vector<const sprite *> front_sprites;
		std::vector<const sprite *> back_sprites;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				auto color = _background_color;
				bool found_color = false;

				front_sprites.clear();
				back_sprites.clear();
				_sprite_plane.find_sprites(x, y, front_sprites, back_sprites);

				if (front_sprites.size() > 0) {
					for (auto *sprite : front_sprites) {
						auto palette = _sprite_plane.get_palette(sprite->palette_index);
						auto pixel = _pattern_tables[_sprite_plane.pattern_table_index].get_pixel(sprite->tile_index, x - sprite->x, y - sprite->y);
						if (pixel > 0) {
							color = palette.color(pixel);
							found_color = true;
							break;
						}
					}
				}

				if (!found_color) {
					index_t tile_index = 0;
					auto palette = _background_plane.get(x, y, tile_index);
					auto pixel = _pattern_tables[_background_plane.pattern_table_index].get_pixel(tile_index, x, y);
					if (pixel > 0) {
						color = palette.color(pixel);
						found_color = true;
					}
				}

				if (!found_color && (back_sprites.size() > 0)) {
					for (auto *sprite : back_sprites) {
						auto palette = _sprite_plane.get_palette(sprite->palette_index);
						auto pixel = _pattern_tables[_sprite_plane.pattern_table_index].get_pixel(sprite->tile_index, x - sprite->x, y - sprite->y);
						if (pixel > 0) {
							color = palette.color(pixel);
							found_color = true;
							break;
						}
					}
				}

				framebuffer[y * width + x] = color;
			}
		}
		return true;
	}

	void write_pattern(size_t pattern_table_index, size_t tile_index, std::span<pixel_t> &&src) {
		_pattern_tables[pattern_table_index % num_pattern_tables].write(tile_index, std::move(src));
	}

	void set_sprite_palette(size_t palette_index, size_t palette_color_index, color_t new_color) {
		_sprite_plane.set_palette(palette_index, palette_color_index, new_color);
	}

	void set_sprite_palette(size_t palette_index, color_t new_color1, color_t new_color2, color_t new_color3, color_t new_color4) {
		_sprite_plane.set_palette(palette_index, 0, new_color1);
		_sprite_plane.set_palette(palette_index, 1, new_color2);
		_sprite_plane.set_palette(palette_index, 2, new_color3);
		_sprite_plane.set_palette(palette_index, 3, new_color4);
	}

	void set_sprite_pattern_table(index_t index) {
		_sprite_plane.pattern_table_index = index % num_pattern_tables;
	}

	void set_background_palette(size_t palette_index, size_t palette_color_index, color_t new_color) {
		_background_plane.set_palette(palette_index, palette_color_index, new_color);
	}

	void set_background_palette(size_t palette_index, color_t new_color1, color_t new_color2, color_t new_color3, color_t new_color4) {
		_background_plane.set_palette(palette_index, 0, new_color1);
		_background_plane.set_palette(palette_index, 1, new_color2);
		_background_plane.set_palette(palette_index, 2, new_color3);
		_background_plane.set_palette(palette_index, 3, new_color4);
	}

	void set_background_pattern_table(index_t index) {
		_background_plane.pattern_table_index = index % num_pattern_tables;
	}

	void set_background_color(color_t color) { _background_color = color; }

	void set_sprite(
		size_t position,
		coordinate_t x = 0,
		coordinate_t y = 0,
		index_t tile_index = 0xFF,
		index_t palette_index = 0,
		attribute_t attributes = 0
	) {
		_sprite_plane.set_sprite(position, x, y, tile_index, palette_index, attributes);
	}

	auto set_tile(size_t name_table_index, size_t x, size_t y, index_t index) {
		_background_plane.set_tile(name_table_index, x, y, index);
	}

	auto set_tile_palette(size_t name_table_index, size_t x, size_t y, index_t index) {
		_background_plane.set_tile_palette(name_table_index, x, y, index);
	}

private:
	sprite_plane _sprite_plane{};
	background_plane _background_plane{};
	color_t _background_color = 0;
	std::array<pattern_table, num_pattern_tables> _pattern_tables{};
};

class runtime {
public:
public:
	runtime() {}

public:
#define INSTALL_PPU_FN_EX(NAME, FN) template<typename... Args> auto NAME(Args&&... args) { return _ppu.FN(std::forward<Args>(args)...); }
#define INSTALL_PPU_FN(NAME) INSTALL_PPU_FN_EX(NAME, NAME)

	INSTALL_PPU_FN_EX(render_picture, render);

	INSTALL_PPU_FN(write_pattern);

	INSTALL_PPU_FN(set_sprite);
	INSTALL_PPU_FN(set_sprite_palette);
	INSTALL_PPU_FN(set_sprite_pattern_table);

	INSTALL_PPU_FN(set_background_palette);
	INSTALL_PPU_FN(set_background_pattern_table);
	INSTALL_PPU_FN(set_background_color);

	INSTALL_PPU_FN(set_tile);
	INSTALL_PPU_FN(set_tile_palette);

#undef INSTALL_PPU_FN
#undef INSTALL_PPU_FN_EX

private:
	ppu _ppu;
};

} // namespace expt8
