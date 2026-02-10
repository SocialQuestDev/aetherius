import json

def generate():
    with open('items.json', 'r', encoding='utf-8') as f:
        items = {it['name']: it['id'] for it in json.load(f)}
    with open('blocks.json', 'r', encoding='utf-8') as f:
        # Берем defaultState для сетевых пакетов
        blocks = {bl['name']: {'id': bl['id'], 'state': bl['defaultState']} for bl in json.load(f)}

    all_names = sorted(set(list(items.keys()) + list(blocks.keys())))

    hpp = """#ifndef MINECRAFT_REGISTRY_HPP
#define MINECRAFT_REGISTRY_HPP

#include <string>

struct RegistryEntry {
    const char* name;
    int itemId;
    int blockId;
    int stateId; // Добавили поле для State ID
};

class MinecraftRegistry {
public:
    static inline const RegistryEntry table[] = {
"""
    
    lines = []
    for name in all_names:
        i_id = items.get(name, -1)
        b_info = blocks.get(name, {'id': -1, 'state': -1})
        lines.append(f'        {{"{name}", {i_id}, {b_info["id"]}, {b_info["state"]}}}')
    
    hpp += ",\n".join(lines)
    hpp += f"""
    }};

    static constexpr size_t tableSize = {len(lines)};

    static const RegistryEntry* getByItemId(int id) {{
        if (id == -1) return nullptr;
        for (const auto& entry : table) {{ if (entry.itemId == id) return &entry; }}
        return nullptr;
    }}
}};
#endif"""

    with open('../include/utility/MinecraftRegistry.hpp', 'w', encoding='utf-8') as f:
        f.write(hpp)

generate()