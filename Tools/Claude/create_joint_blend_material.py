# Crée /Game/_Claude/M_Claude_JointBlendDebug : matériau de test du mélange des normales aux jointures.
# Généré par Claude (règle du CLAUDE.md : assets de test seulement dans Content/_Claude/).
#
# Le paramètre Mode choisit ce qu'on voit :
#   0 = normale mélangée, variante hybride (deux sondes GDF)        -> le rendu « lissé »
#   1 = normale d'origine du mesh (sans mélange)                    -> pour comparer
#   2 = variante hybride + masque du mélange en orange              -> où et combien ça mélange
#   3 = variante « surface » (méthode MF_Light2 de l'auteur)        -> gradient du GDF à la surface
#   4 = jonctions seules (rouge = une autre pièce est à moins de R1 ou R2)
# Albédo beige + XOR filtré (Patterns.ush) ; soleil maison calculé dans le shader (SunDir).
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
sun = node(unreal.MaterialExpressionVectorParameter, -1400, 1000)
sun.set_editor_property("parameter_name", "SunDir")
sun.set_editor_property("default_value", unreal.LinearColor(0.45, 0.55, 0.7, 0.0))
cell = scalar("Cell", 6.0, -1400, 1120)


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
custom.set_editor_property("include_file_paths", ["/Plugin/DwamLighting/NormalTransfer.ush", "/Plugin/DwamLighting/Patterns.ush"])
names = ["N", "R1", "D1", "G1", "R2", "D2", "G2", "G0", "BaseBlend", "SurfaceAlpha", "Mode", "WP", "SunDir", "Cell"]
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
// Albédo : beige, avec le XOR filtré de Quilez projeté sur le plan le plus face à la normale du mesh (lit les formes)
float3 An = abs(Nn);
float2 UV = (An.z > An.x && An.z > An.y) ? WP.xy : ((An.x > An.y) ? WP.yz : WP.xz);
float X = DwamFilteredXorAuto(UV / Cell);
float3 Albedo = float3(0.72, 0.66, 0.56) * lerp(0.55, 1.0, X);
// Soleil maison dans le shader (unlit) : diffus + ciel, terminateur doux
float3 L = normalize(SunDir.xyz);
float Ndl = saturate(dot(NS, L) * 0.85 + 0.15);
float3 C = Albedo * (float3(1.0, 0.96, 0.9) * Ndl + float3(0.16, 0.18, 0.22));
if (Mode > 1.5 && Mode < 2.5)
{
    C = lerp(C, float3(1.0, 0.35, 0.05) * (0.35 + 0.65 * Ndl), saturate(Mask));
}
if (Mode > 3.5)
{
    // Jonctions seules : rouge là où la sonde proche voit une autre pièce, sans la part de base ni les garde-fous
    float J = saturate(1.0 - D1 / max(R1, 0.01));
    float J2 = saturate(1.0 - D2 / max(R2, 0.01));
    C = lerp(float3(0.5, 0.5, 0.5) * (0.4 + 0.6 * Ndl), float3(0.9, 0.1, 0.05), max(J, J2 * 0.5));
}
if (Mode > 4.5)
{
    // Mesure : distance lue par la sonde proche, divisée par sa hauteur R1. 1 (blanc) = attendu loin de tout ;
    // 0 (noir) = la sonde croit toucher une surface ; rouge = au-delà de 1,5 ; bleu = négatif (dans un objet)
    float Ratio = D1 / max(R1, 0.01);
    C = Ratio < 0.0 ? float3(0.1, 0.2, 1.0) : (Ratio > 1.5 ? float3(1.0, 0.1, 0.1) : saturate(Ratio).xxx);
}
return C;
""")
for n, src in zip(names, [vn, r1, d1, g1, r2, d2, g2, g0, base, alpha, mode, wp, sun, cell]):
    link(src, "", custom, n)

MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"CLAUDE: {PATH}/{NAME} créé et sauvegardé")
