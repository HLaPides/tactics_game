#include "map.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdio>

GameMap::GameMap() : GameMap(32) {}

GameMap::GameMap(int tile_size) {
    this->tile_size = tile_size;
    cols = 0;
    rows = 0;
}

// ─── tileset loader ───────────────────────────────────────────────────────────

void GameMap::load_tileset(const std::string& tsx_path) {
    std::string src = read_file(tsx_path);
    if (src.empty()) return;

    // default all 30 tiles to floor/walkable
    tile_table.assign(30, Tile{});

    size_t search = 0;
    while (true) {
        size_t tile_tag = src.find("<tile id=\"", search);
        if (tile_tag == std::string::npos) break;

        size_t id_start = tile_tag + 10;
        size_t id_end   = src.find('"', id_start);
        if (id_end == std::string::npos) break;

        int id = -1;
        try { id = std::stoi(src.substr(id_start, id_end - id_start)); }
        catch (...) { search = id_end + 1; continue; }

        if (id < 0 || id >= 30) { search = id_end + 1; continue; }

        size_t tile_end = src.find("</tile>", id_end);
        if (tile_end == std::string::npos) break;
        std::string block = src.substr(tile_tag, tile_end - tile_tag + 7);
        search = tile_end + 7;

        std::string type_str  = "floor";
        std::string cover_str = "none";
        bool        walkable  = true;

        size_t prop_search = 0;
        while (true) {
            size_t prop = block.find("<property name=\"", prop_search);
            if (prop == std::string::npos) break;

            size_t name_start = prop + 16;
            size_t name_end   = block.find('"', name_start);
            if (name_end == std::string::npos) break;
            std::string prop_name = block.substr(name_start, name_end - name_start);

            size_t val_pos = block.find("value=\"", name_end);
            if (val_pos == std::string::npos) break;
            val_pos += 7;
            size_t val_end = block.find('"', val_pos);
            if (val_end == std::string::npos) break;
            std::string val = block.substr(val_pos, val_end - val_pos);

            if      (prop_name == "type")     type_str  = val;
            else if (prop_name == "cover")    cover_str = val;
            else if (prop_name == "walkable") walkable  = (val == "true");

            prop_search = val_end + 1;
        }

        Tile t;
        t.tile_id      = id;
        t.walkable     = walkable;
        t.is_objective = (type_str == "objective");

        if      (type_str == "wall")      t.type = TILE_WALL;
        else if (type_str == "barrel")    t.type = TILE_BARREL;
        else if (type_str == "objective") t.type = TILE_OBJECTIVE;
        else if (type_str == "railing")   t.type = TILE_RAILING;
        else                              t.type = TILE_FLOOR;

        if      (cover_str == "full") t.cover = COVER_FULL;
        else if (cover_str == "half") t.cover = COVER_HALF;
        else                          t.cover = COVER_NONE;

        // barrel tiles with cover face all directions, everything else faces none
        bool faces_all = (t.cover != COVER_NONE);
        t.faces = {faces_all, faces_all, faces_all, faces_all};

        tile_table[id] = t;
    }
}

// ─── tile from id ─────────────────────────────────────────────────────────────

Tile GameMap::tile_from_id(int id) const {
    // JSON tile IDs are 1-based (firstgid=1), shift down to 0-based
    int idx = id - 1;
    if (idx < 0 || idx >= (int)tile_table.size()) {
        Tile t; t.tile_id = id; return t;
    }
    Tile t    = tile_table[idx];
    t.tile_id = id;
    return t;
}

// ─── JSON helpers ─────────────────────────────────────────────────────────────

std::string GameMap::read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        printf("[MAP] ERROR: could not open file: %s\n", path.c_str());
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
    return content;
}

int GameMap::json_int(const std::string& src, const std::string& key, int def) {
    std::string search = "\"" + key + "\":";
    size_t pos = src.find(search);
    if (pos == std::string::npos) {
        search = "\"" + key + "\": ";
        pos = src.find(search);
        if (pos == std::string::npos) return def;
    }
    pos = src.find(':', pos);
    if (pos == std::string::npos) return def;
    pos++;
    while (pos < src.size() && std::isspace((unsigned char)src[pos])) pos++;
    if (pos >= src.size()) return def;
    try { return std::stoi(src.substr(pos)); }
    catch (...) { return def; }
}

std::string GameMap::json_string(const std::string& src, const std::string& key,
                                  const std::string& def) {
    std::string search = "\"" + key + "\":";
    size_t pos = src.find(search);
    if (pos == std::string::npos) {
        search = "\"" + key + "\": ";
        pos = src.find(search);
        if (pos == std::string::npos) return def;
    }
    pos = src.find(':', pos);
    if (pos == std::string::npos) return def;
    pos = src.find('"', pos);
    if (pos == std::string::npos) return def;
    pos++;
    size_t end = src.find('"', pos);
    if (end == std::string::npos) return def;
    return src.substr(pos, end - pos);
}

std::vector<int> GameMap::json_int_array(const std::string& src, const std::string& key) {
    std::vector<int> result;
    std::string search = "\"" + key + "\":";
    size_t pos = src.find(search);
    if (pos == std::string::npos) {
        search = "\"" + key + "\": ";
        pos = src.find(search);
        if (pos == std::string::npos) return result;
    }
    pos = src.find('[', pos);
    if (pos == std::string::npos) return result;
    pos++;
    size_t end = src.find(']', pos);
    if (end == std::string::npos) return result;

    std::string array_str = src.substr(pos, end - pos);
    std::istringstream ss(array_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token.erase(std::remove_if(token.begin(), token.end(),
                    [](unsigned char c){ return std::isspace(c); }), token.end());
        if (!token.empty()) {
            try { result.push_back(std::stoi(token)); }
            catch (...) {}
        }
    }
    return result;
}

// ─── property parser (Tiled JSON properties array) ───────────────────────────

static std::string parse_property(const std::string& obj_str,
                                   const std::string& prop_name,
                                   const std::string& def = "") {
    size_t props_pos = obj_str.find("\"properties\"");
    if (props_pos == std::string::npos) return def;

    size_t arr_start = obj_str.find('[', props_pos);
    if (arr_start == std::string::npos) return def;
    size_t arr_end = obj_str.find(']', arr_start);
    if (arr_end == std::string::npos) return def;

    std::string props = obj_str.substr(arr_start, arr_end - arr_start + 1);

    size_t search = 0;
    while (true) {
        size_t obj = props.find('{', search);
        if (obj == std::string::npos) break;
        size_t end = props.find('}', obj);
        if (end == std::string::npos) break;
        std::string entry = props.substr(obj, end - obj + 1);
        search = end + 1;

        size_t name_pos = entry.find("\"name\"");
        if (name_pos == std::string::npos) continue;
        name_pos = entry.find('"', name_pos + 6);
        if (name_pos == std::string::npos) continue;
        name_pos++;
        size_t name_end = entry.find('"', name_pos);
        if (name_end == std::string::npos) continue;
        std::string found_name = entry.substr(name_pos, name_end - name_pos);
        if (found_name != prop_name) continue;

        size_t val_pos = entry.find("\"value\"");
        if (val_pos == std::string::npos) continue;
        val_pos = entry.find('"', val_pos + 7);
        if (val_pos == std::string::npos) continue;
        val_pos++;
        size_t val_end = entry.find('"', val_pos);
        if (val_end == std::string::npos) continue;
        return entry.substr(val_pos, val_end - val_pos);
    }
    return def;
}

// ─── load ─────────────────────────────────────────────────────────────────────

bool GameMap::load(const std::string& filepath) {
    std::string src = read_file(filepath);
    if (src.empty()) {
        printf("[MAP] ERROR: file empty or not found\n");
        return false;
    }

    tiles.clear();
    player_spawns.clear();
    enemy_spawns.clear();
    objectives.clear();

    // load tileset first so tile_from_id has data
    load_tileset("levels/tileset_03.tsx");

    std::string header = src.substr(0, std::min((int)src.size(), 500));
    std::string footer = src.substr(std::max(0, (int)src.size() - 500));
    int map_h = json_int(header, "height", 0);
    int map_w = json_int(footer, "width",  0);
    if (map_w <= 0 || map_h <= 0) {
        printf("[MAP] ERROR: invalid dimensions\n");
        return false;
    }

    size_t layer_pos = src.find("\"tilelayer\"");
    if (layer_pos == std::string::npos) {
        printf("[MAP] ERROR: no tilelayer found\n");
        return false;
    }

    size_t layer_obj_start = src.rfind('{', layer_pos);
    if (layer_obj_start == std::string::npos) {
        printf("[MAP] ERROR: could not find layer object start\n");
        return false;
    }

    std::string tile_section = src.substr(layer_obj_start);
    std::vector<int> data = json_int_array(tile_section, "data");
    if ((int)data.size() != map_w * map_h) {
        printf("[MAP] ERROR: tile data size mismatch\n");
        return false;
    }

    tiles.resize(map_h, std::vector<Tile>(map_w));
    for (int row = 0; row < map_h; row++)
        for (int col = 0; col < map_w; col++)
            tiles[row][col] = tile_from_id(data[row * map_w + col]);

    rows = map_h;
    cols = map_w;

    size_t obj_pos = src.find("\"objectgroup\"");
    if (obj_pos == std::string::npos) return true;

    size_t objs_key = src.rfind("\"objects\"", obj_pos);
    if (objs_key == std::string::npos) return true;

    size_t arr_start = src.find('[', objs_key);
    if (arr_start == std::string::npos) return true;

    size_t arr_end = arr_start + 1;
    int bracket_depth = 1;
    while (arr_end < src.size() && bracket_depth > 0) {
        if (src[arr_end] == '[') bracket_depth++;
        else if (src[arr_end] == ']') bracket_depth--;
        arr_end++;
    }

    std::string objects_str = src.substr(arr_start, arr_end - arr_start);

    int obj_count = 0;
    size_t search_pos = 0;
    while (true) {
        size_t obj_start = objects_str.find('{', search_pos);
        if (obj_start == std::string::npos) break;

        int depth = 1;
        size_t p = obj_start + 1;
        while (p < objects_str.size() && depth > 0) {
            if (objects_str[p] == '{') depth++;
            else if (objects_str[p] == '}') depth--;
            p++;
        }
        std::string obj_str = objects_str.substr(obj_start, p - obj_start);
        search_pos = p;

        std::string obj_name = json_string(obj_str, "name", "");
        int x_raw = json_int(obj_str, "x", -1);
        int y_raw = json_int(obj_str, "y", -1);

        if (x_raw < 0 || y_raw < 0) { obj_count++; continue; }

        int col = x_raw / tile_size;
        int row = y_raw / tile_size;
        col = std::max(0, std::min(col, map_w - 1));
        row = std::max(0, std::min(row, map_h - 1));

        if (obj_name == "player_spawn") {
            int slot = 1;
            size_t slot_pos = obj_str.find("\"slot\"");
            if (slot_pos != std::string::npos) {
                std::string slot_section = obj_str.substr(slot_pos);
                slot = json_int(slot_section, "value", 1);
            }
            char type_char = '0' + slot;
            player_spawns.push_back({col, row, type_char});
        }
        else if (obj_name == "soldier_spawn")  { enemy_spawns.push_back({col, row, 'E'}); }
        else if (obj_name == "guard_spawn")    { enemy_spawns.push_back({col, row, 'G'}); }
        else if (obj_name == "captain_spawn")  { enemy_spawns.push_back({col, row, 'C'}); }
        else if (obj_name == "objective_kill") {
            Objective obj;
            obj.type   = Objective::Type::KILL_UNIT;
            obj.target = parse_property(obj_str, "target", "captain");
            std::string lbl = parse_property(obj_str, "label", "");
            if (lbl.empty()) {
                if      (obj.target == "captain") lbl = "Kill Captain Vane";
                else if (obj.target == "soldier") lbl = "Kill all soldiers";
                else if (obj.target == "guard")   lbl = "Kill all guards";
                else                              lbl = "Kill the target";
            }
            obj.label = lbl;
            objectives.push_back(obj);
        }
        else if (obj_name == "objective_hold") {
            Objective obj;
            obj.type  = Objective::Type::HOLD_TILE;
            obj.label = parse_property(obj_str, "label", "Secure the area");
            obj.col   = col;
            obj.row   = row;
            objectives.push_back(obj);
        }

        obj_count++;
    }

    // if no explicit hold objective, fall back to objective tiles in the map
    bool has_hold = false;
    for (auto& o : objectives)
        if (o.type == Objective::Type::HOLD_TILE) { has_hold = true; break; }
    if (!has_hold) {
        Objective obj;
        obj.type  = Objective::Type::HOLD_TILE;
        obj.label = "Secure the quarters";
        obj.col   = -1;
        obj.row   = -1;
        objectives.push_back(obj);
    }

    return true;
}

// ─── accessors ────────────────────────────────────────────────────────────────

int  GameMap::get_tile_size()   const { return tile_size; }
int  GameMap::get_x_dimension() const { return cols * tile_size; }
int  GameMap::get_y_dimension() const { return rows * tile_size; }
int  GameMap::getCols()         const { return cols; }
int  GameMap::getRows()         const { return rows; }

Tile GameMap::get_tile(int col, int row) const {
    if (row < 0 || row >= rows || col < 0 || col >= cols) {
        Tile t; return t;
    }
    return tiles[row][col];
}

bool GameMap::is_walkable(int col, int row) const {
    return get_tile(col, row).walkable;
}

bool GameMap::is_objective(int col, int row) const {
    if (row < 0 || row >= rows || col < 0 || col >= cols) return false;
    return tiles[row][col].is_objective;
}

const std::vector<std::vector<Tile>>& GameMap::get_tiles() const {
    return tiles;
}

const std::vector<Objective>& GameMap::get_objectives() const {
    return objectives;
}

std::vector<SpawnPoint> GameMap::get_player_spawns() { return player_spawns; }
std::vector<SpawnPoint> GameMap::get_enemy_spawns()  { return enemy_spawns; }