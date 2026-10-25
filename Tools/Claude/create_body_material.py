# Crée /Game/_Claude/M_Claude_Body : matériau de corps du mannequin, jointures fondues en permanence (M4).
# Généré par Claude (règle : assets de test seulement dans Content/_Claude/).
#
# Mélange = méthode de l'auteur (MF_Light2) : trois sondes à D, D+1, D+2 le long de la normale → pseudo-AO,
# normale = lerp(gradient du GDF à la surface, normale du mesh, AO). D vient de la Custom Primitive Data 0,
# réglée par UCharacterAssemblyComponent d'après la taille du corps.
# + rejet du sol (de la variante hybride) : pas de fondu vers le sol sous les pieds.
# Éclairage provisoire calculé dans le shader (soleil + ciel), unlit, en attendant l'éclairage maison.
# Paramètre DebugBlend = 1 : affiche la part de mélange en orange.

import unreal

PATH = "/Game/_Claude"
NAME = "M_Claude_Body"
MEL = unreal.MaterialEditingLibrary

if unreal.EditorAssetLibrary.does_asset_exist(f"{PATH}/{NAME}"):
    unreal.EditorAssetLibrary.delete_asset(f"{PATH}/{NAME}")
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)


def node(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def link(a, b, b_in):
    MEL.connect_material_expressions(a, "", b, b_in)


def scalar(name, value, y, cpd=None):
    s = node(unreal.MaterialExpressionScalarParameter, -1500, y)
    s.set_editor_property("parameter_name", name)
    s.set_editor_property("default_value", value)
    if cpd is not None:
        s.set_editor_property("use_custom_primitive_data", True)
        s.set_editor_property("primitive_data_index", cpd)
    return s


wp = node(unreal.MaterialExpressionWorldPosition, -1500, 0)
vn = node(unreal.MaterialExpressionVertexNormalWS, -1500, 150)
dist = scalar("JointBlendDistance", 10.0, 300, cpd=0)
debug = scalar("DebugBlend", 0.0, 450)
reject = scalar("GroundRejection", 0.9, 600)
cell = scalar("Cell", 0.35, 750)
sun = node(unreal.MaterialExpressionVectorParameter, -1500, 900)
sun.set_editor_property("parameter_name", "SunDir")
sun.set_editor_property("default_value", unreal.LinearColor(0.45, 0.55, 0.7, 0.0))
albedo = node(unreal.MaterialExpressionVectorParameter, -1500, 1050)
albedo.set_editor_property("parameter_name", "Albedo")
albedo.set_editor_property("default_value", unreal.LinearColor(0.72, 0.66, 0.56, 1.0))


def probe(extra, y):
    """Distance du GDF à WorldPos + Normal × (D + extra)."""
    add_d = node(unreal.MaterialExpressionAdd, -1300, y)
    link(dist, add_d, "A")
    add_d.set_editor_property("const_b", extra)
    mul = node(unreal.MaterialExpressionMultiply, -1150, y)
    link(vn, mul, "A")
    link(add_d, mul, "B")
    add = node(unreal.MaterialExpressionAdd, -1000, y)
    link(wp, add, "A")
    link(mul, add, "B")
    d = node(unreal.MaterialExpressionDistanceToNearestSurface, -850, y)
    link(add, d, "Position")
    return d


d0, d1, d2 = probe(0.0, 0), probe(1.0, 200), probe(2.0, 400)
g0 = node(unreal.MaterialExpressionDistanceFieldGradient, -850, 600)
link(wp, g0, "Position")

custom = node(unreal.MaterialExpressionCustom, -400, 300)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "ClaudeBody")
custom.set_editor_property("include_file_paths", ["/Plugin/DwamLighting/NormalTransfer.ush", "/Plugin/DwamLighting/Patterns.ush"])
names = ["N", "WP", "D", "DA0", "DA1", "DA2", "G0", "DebugBlend", "GroundRejection", "Cell", "SunDir", "Albedo"]
inputs = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("code", """
float3 Nn = normalize(N);
float Dd = max(D, 0.5);
// Méthode de l'auteur (MF_Light2)
float AO = smoothstep(0.0, 1.0, (DA0 / Dd) * lerp(0.25, 1.0, saturate(DA1 / (Dd + 1.0))) * lerp(0.25, 1.0, saturate(DA2 / (Dd + 2.0))));
float3 G = DwamSafeNormalize(G0, Nn);
float Mix = 1.0 - AO;
// Rejet du sol (variante hybride) : face tournée vers le bas dont la normale cible pointe vers le haut
Mix *= 1.0 - saturate(GroundRejection) * saturate(-Nn.z) * saturate(G.z);
// Ne jamais retourner la normale
Mix *= saturate(dot(G, Nn) * 4.0 + 1.0);
float3 NS = normalize(lerp(Nn, G, Mix));
// Albédo + XOR filtré de Quilez (lecture des formes)
float3 An = abs(Nn);
float2 UV = (An.z > An.x && An.z > An.y) ? WP.xy : ((An.x > An.y) ? WP.yz : WP.xz);
float3 Alb = Albedo.rgb * lerp(0.55, 1.0, DwamFilteredXorAuto(UV / Cell));
float Ndl = saturate(dot(NS, normalize(SunDir.xyz)) * 0.85 + 0.15);
float3 C = Alb * (float3(1.0, 0.96, 0.9) * Ndl + float3(0.16, 0.18, 0.22));
return lerp(C, float3(1.0, 0.35, 0.05) * (0.35 + 0.65 * Ndl), saturate(Mix) * DebugBlend);
""")
for n, src in zip(names, [vn, wp, dist, d0, d1, d2, g0, debug, reject, cell, sun, albedo]):
    link(src, custom, n)
MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"CLAUDE: {PATH}/{NAME} créé et sauvegardé")
