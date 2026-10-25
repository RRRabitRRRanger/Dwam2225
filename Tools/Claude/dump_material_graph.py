# Lecture SEULE d'un matériau ou d'une Material Function : liste les nœuds, leurs réglages et leurs liens.
# N'enregistre rien. Sortie dans Saved/Automation/<nom>_graph.txt.
# Usage : Tools/RunPython.ps1 Tools/Claude/dump_material_graph.py  (chemin de l'asset ci-dessous)

import unreal, os

ASSET = "/Game/AVOLABO/Shaders/MF_Light2"

asset = unreal.load_asset(ASSET)
out = []
out.append(f"ASSET {ASSET} : {asset.get_class().get_name()}")

if isinstance(asset, unreal.MaterialFunction):
    exprs = unreal.MaterialEditingLibrary.get_material_function_expressions(asset) if hasattr(unreal.MaterialEditingLibrary, "get_material_function_expressions") else []
else:
    exprs = []
if not exprs:
    exprs = asset.get_editor_property("expression_collection").get_editor_property("expressions") if hasattr(asset, "expression_collection") else []

names = {}
for i, e in enumerate(exprs):
    names[e.get_name()] = i

props_of_interest = ["parameter_name", "default_value", "r", "const_a", "const_b", "input_name", "description",
                     "code", "output_type", "input_type", "function", "material_function", "coordinate_index",
                     "use_custom_primitive_data", "primitive_data_index", "world_position_shader_offset", "desc"]

for i, e in enumerate(exprs):
    cls = e.get_class().get_name()
    line = f"[{i}] {cls} ({e.get_name()})"
    for p in props_of_interest:
        try:
            v = e.get_editor_property(p)
            if v is None or v == "" :
                continue
            if p == "code":
                line += f"\n    CODE:\n{v}"
            else:
                line += f"\n    {p} = {v}"
        except Exception:
            pass
    try:
        ins = unreal.MaterialEditingLibrary.get_inputs_for_material_expression(asset, e)
        for j, src in enumerate(ins):
            if src:
                line += f"\n    entrée {j} <- [{names.get(src.get_name(), '?')}] {src.get_class().get_name()}"
    except Exception as ex:
        line += f"\n    (entrées illisibles : {ex})"
    out.append(line)

path = os.path.join(unreal.Paths.project_saved_dir(), "Automation", os.path.basename(ASSET) + "_graph.txt")
os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, "w", encoding="utf-8") as f:
    f.write("\n".join(out))
unreal.log(f"CLAUDE: graphe écrit dans {path} ({len(exprs)} nœuds)")
