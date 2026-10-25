# Crée /Game/_Claude/M_Claude_JointBlendDebug : matériau de test du mélange des normales aux jointures.
# Généré par Claude (règle du CLAUDE.md : assets de test seulement dans Content/_Claude/).
#
# Le paramètre Mode choisit ce qu'on voit :
#   0 = normale mélangée, variante hybride (deux sondes GDF)        -> le rendu « lissé »
#   1 = normale d'origine du mesh (sans mélange)                    -> pour comparer
#   2 = variante hybride + masque du mélange en orange              -> où et combien ça mélange
#   3 = variante « surface » (méthode MF_Light2 de l'auteur)        -> gradient du GDF à la surface
# Éclairage simple et fixe (unlit), pour juger la forme et pas la lumière.

import unreal

PATH = "/Game/_Claude"
NAME = "M_Claude_JointBlendDebug"
MEL = unreal.MaterialEditingLibrary

if unreal.EditorAssetLibrary.does_asset_exist(f"{PATH}/{NAME}"):
    unreal.EditorAssetLibrary.delete_asset(f"{PATH}/{NAME}")

mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)


def node(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def scalar(name, value, x, y):
    s = node(unreal.MaterialExpressionScalarParameter, x, y)
    s.set_editor_property("parameter_name", name)
    s.set_editor_property("default_value", value)
    return s


def link(a, a_out, b, b_in):
    MEL.connect_material_expressions(a, a_out, b, b_in)


wp = node(unreal.MaterialExpressionWorldPosition, -1400, 0)
vn = node(unreal.MaterialExpressionVertexNormalWS, -1400, 200)
r1 = scalar("R1", 5.0, -1400, 400)
r2 = scalar("R2", 40.0, -1400, 520)
base = scalar("BaseBlend", 0.3, -1400, 640)
alpha = scalar("SurfaceAlpha", 0.5, -1400, 760)
mode = scalar("Mode", 0.0, -1400, 880)


def probe(radius, y):
    """Distance et gradient du GDF à WorldPos + Normal * radius."""
    mul = node(unreal.MaterialExpressionMultiply, -1150, y)
    link(vn, "", mul, "A")
    link(radius, "", mul, "B")
    add = node(unreal.MaterialExpressionAdd, -950, y)
    link(wp, "", add, "A")
    link(mul, "", add, "B")
    dist = node(unreal.MaterialExpressionDistanceToNearestSurface, -750, y)
    link(add, "", dist, "Position")
    grad = node(unreal.MaterialExpressionDistanceFieldGradient, -750, y + 100)
    link(add, "", grad, "Position")
    return dist, grad


d1, g1 = probe(r1, 0)
d2, g2 = probe(r2, 300)
g0 = node(unreal.MaterialExpressionDistanceFieldGradient, -750, 600)
link(wp, "", g0, "Position")

custom = node(unreal.MaterialExpressionCustom, -400, 300)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "ClaudeJointBlendDebug")
custom.set_editor_property("include_file_paths", ["/Plugin/DwamLighting/NormalTransfer.ush"])
names = ["N", "R1", "D1", "G1", "R2", "D2", "G2", "G0", "BaseBlend", "SurfaceAlpha", "Mode"]
inputs = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("code", """
float3 Nn = normalize(N);
float Mask;
float3 Hyb = DwamNormalTransferHybrid(Nn, R1, D1, G1, R2, D2, G2, BaseBlend, 1.0, 0.5, float3(0, 0, 1), 0.8, Mask);
float3 Surf = DwamNormalTransferSurface(Nn, G0, SurfaceAlpha);
float3 NS = Mode < 0.5 ? Hyb : (Mode < 1.5 ? Nn : (Mode < 2.5 ? Hyb : Surf));
float3 L = normalize(float3(0.5, 0.6, 0.62));
float Sh = 0.16 + 0.84 * saturate(dot(NS, L));
float3 C = float3(0.62, 0.6, 0.58) * Sh;
if (Mode > 1.5 && Mode < 2.5)
{
    C = lerp(C, float3(1.0, 0.35, 0.05) * (0.35 + 0.65 * Sh), saturate(Mask));
}
return C;
""")
for n, src in zip(names, [vn, r1, d1, g1, r2, d2, g2, g0, base, alpha, mode]):
    link(src, "", custom, n)

MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"CLAUDE: {PATH}/{NAME} créé et sauvegardé")
