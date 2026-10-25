# Génère les pièces du mannequin de test à leur vraie taille, avec Geometry Script, dans /Game/_Claude/Parts/.
# Généré par Claude (règle : assets de test seulement dans Content/_Claude/).
#
# Pourquoi : les formes du moteur étirées de façon inégale (cylindre 0,1 × 0,1 × 0,4) ont un champ de distance
# faux. Ici chaque pièce est modélisée à sa taille (échelle 1), arrondie (capsule ou ellipsoïde), et son
# Mesh Distance Field est construit avec l'asset (r.GenerateMeshDistanceFields du projet).
#
# Entrée : Saved/Automation/CharacterKit_ProxySpecs.json (test CharacterKit.Export.ProxySpecs).
# Convention : X latéral, Y profondeur, Z le long de l'os, centré (comme les formes du moteur).

import unreal, json, os

SPECS = os.path.join(unreal.Paths.project_saved_dir(), "Automation", "CharacterKit_ProxySpecs.json")
FOLDER = "/Game/_Claude/Parts"

with open(SPECS, encoding="utf-8") as f:
    specs = json.load(f)

GP = unreal.GeometryScript_Primitives
GT = unreal.GeometryScript_MeshTransforms
GA = unreal.GeometryScript_NewAssetUtils if hasattr(unreal, "GeometryScript_NewAssetUtils") else unreal.GeometryScript_AssetUtils

options = unreal.GeometryScriptPrimitiveOptions()
asset_options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
asset_options.set_editor_property("enable_recompute_normals", False)
asset_options.set_editor_property("enable_collision", False)

created = 0
for s in specs:
    w, d, l = max(s["x"], 0.5), max(s["y"], 0.5), max(s["z"], 0.5)
    mesh = unreal.DynamicMesh()
    if s["shape"] == "Sphere":
        r0 = 50.0
        GP.append_sphere_lat_long(mesh, options, unreal.Transform(), r0, 16, 24, unreal.GeometryScriptPrimitiveOriginMode.CENTER)
        height = 2 * r0
        sx, sy = w / (2 * r0), d / (2 * r0)
    else:
        # Capsule : rayon = demi-épaisseur la plus petite, puis mise à l'échelle des trois axes (cuite dans la géométrie)
        r0 = 0.5 * min(w, d)
        line = max(l - 2 * r0, 0.0)
        GP.append_capsule(mesh, options, unreal.Transform(), r0, line, 8, 20, 4, unreal.GeometryScriptPrimitiveOriginMode.CENTER)
        height = 2 * r0 + line
        sx, sy = w / (2 * r0), d / (2 * r0)
    GT.scale_mesh(mesh, unreal.Vector(sx, sy, l / height), unreal.Vector(0, 0, 0), True)

    path = f"{FOLDER}/SM_{s['slot']}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    result = GA.create_new_static_mesh_asset_from_mesh(mesh, path, asset_options)
    sm = result[0] if isinstance(result, tuple) else result
    if sm:
        sm.set_editor_property("light_map_resolution", 4)
        unreal.EditorAssetLibrary.save_loaded_asset(sm)
        created += 1
    else:
        unreal.log_error(f"CLAUDE: échec pour {path}")

unreal.log(f"CLAUDE: {created} pièces générées dans {FOLDER}")
