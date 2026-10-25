# Crée /Game/_Claude/M_Claude_SDFEyes : le matériau des yeux SDFEyes (Guide_SDFEyes, étape U2), câblé par script.
# Généré par Claude (règle : assets de test seulement dans Content/_Claude/).
# Unlit : le globe, les paupières et les cils sont ray-marchés dans le petit monde de l'œil (Eyelids.ush).
# Les paramètres sont réglés par USDFEyeComponent (forme, clignement, regard, micro-vie).

import unreal

PATH = "/Game/_Claude"
NAME = "M_Claude_SDFEyes"
MEL = unreal.MaterialEditingLibrary

if unreal.EditorAssetLibrary.does_asset_exist(f"{PATH}/{NAME}"):
    unreal.EditorAssetLibrary.delete_asset(f"{PATH}/{NAME}")

mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)


def node(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def link(a, b, b_in):
    MEL.connect_material_expressions(a, "", b, b_in)


def scalar(name, value, y):
    s = node(unreal.MaterialExpressionScalarParameter, -900, y)
    s.set_editor_property("parameter_name", name)
    s.set_editor_property("default_value", value)
    return s


def vector(name, rgb, y):
    v = node(unreal.MaterialExpressionVectorParameter, -1300, y)
    v.set_editor_property("parameter_name", name)
    v.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 0.0))
    return v


# Position du pixel et de la caméra dans l'espace local de la sphère ; direction de la lumière en local
wp = node(unreal.MaterialExpressionWorldPosition, -1500, -300)
local_pos = node(unreal.MaterialExpressionTransformPosition, -1300, -300)
local_pos.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
local_pos.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
link(wp, local_pos, "")

cam = node(unreal.MaterialExpressionCameraPositionWS, -1500, -150)
local_cam = node(unreal.MaterialExpressionTransformPosition, -1300, -150)
local_cam.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
local_cam.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
link(cam, local_cam, "")

light = vector("LightDir", (0.5, 0.6, 0.62), 0)
local_light = node(unreal.MaterialExpressionTransform, -1100, 0)
local_light.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
local_light.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
link(light, local_light, "")

skin = vector("SkinColor", (0.62, 0.56, 0.47), 150)

params = [("EyeRadius", 41.667), ("Mirror", 0.0), ("Debug", 0.0), ("Blink", 0.0), ("GazeX", 0.0), ("GazeY", 0.0),
          ("Follow", 0.6), ("Pupil", 0.13),
          ("EU", 0.22), ("TU", 0.24), ("PU", 0.93), ("EL", -0.22), ("TL", 0.15), ("PL", 1.0), ("Width", 0.62),
          ("Tilt", 0.1), ("Close", 0.47), ("FoldGap", 0.23), ("Fold", 0.0), ("Hood", 0.5), ("Thick", 0.11),
          ("Lash", 1.5), ("Curl", 0.32), ("Car", 0.93), ("Veins", 1.0)]
scalars = [scalar(n, v, 300 + i * 70) for i, (n, v) in enumerate(params)]

custom = node(unreal.MaterialExpressionCustom, -400, 0)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "SDFEyes")
custom.set_editor_property("include_file_paths", ["/Plugin/SDFEyes/Eyelids.ush"])
names = ["LocalPos", "LocalCam", "LightLocal", "SkinColor"] + [n for n, _ in params]
inputs = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("code", """
FSDFEyeShape S;
S.EU = EU; S.TU = TU; S.PU = PU; S.EL = EL; S.TL = TL; S.PL = PL;
S.Width = Width; S.Tilt = Tilt; S.Close = Close; S.FoldGap = FoldGap; S.Fold = Fold; S.Hood = Hood;
S.Thick = Thick; S.Lash = Lash; S.Curl = Curl; S.Car = Car; S.Veins = Veins;
FSDFEyeAnim A;
A.Blink = Blink; A.GazeX = GazeX; A.GazeY = GazeY; A.Follow = Follow; A.Pupil = Pupil;
FSDFEyeParams P = SDFEyes_Derive(S, A);

// espace local Unreal (X avant, Y droite, Z haut) -> espace œil (x droite, y haut, z avant), en rayons de globe
float Side = Mirror > 0.5 ? -1.0 : 1.0;
float3 Pl = LocalPos / EyeRadius;
float3 Cl = LocalCam / EyeRadius;
float3 RO = float3(Side*Pl.y, Pl.z, Pl.x);
float3 Cam = float3(Side*Cl.y, Cl.z, Cl.x);
float3 RD = normalize(RO - Cam);
float3 L = normalize(float3(Side*LightLocal.y, LightLocal.z, LightLocal.x));

FSDFEyeHit H = SDFEyes_Trace(RO - RD*0.01, RD, 0.0, 3.0, P);
if (!H.Hit) return SkinColor.rgb;
return SDFEyes_DemoShade(H, P, RD, L, Debug > 0.5);
""")
link(local_pos, custom, "LocalPos")
link(local_cam, custom, "LocalCam")
link(local_light, custom, "LightLocal")
link(skin, custom, "SkinColor")
for (n, _), s in zip(params, scalars):
    link(s, custom, n)

MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"CLAUDE: {PATH}/{NAME} créé et sauvegardé")
