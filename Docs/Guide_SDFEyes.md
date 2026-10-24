# Guide — SDFEyes : monter le matériau de test (étape U2)

> Accueil : [[00 - Accueil]] · Plan : [[Plan_Eyes]]

> État (2026-10-18) : `Eyelids.ush` et le code du nœud Custom ci-dessous sont **validés par compilation hors moteur** (DXC SM6, FXC SM5). Le plugin `SDFEyes` compile. **Rien n'est encore vu dans l'éditeur.** But de l'étape : retrouver dans Unreal l'image de la démo web « Paupières en pivot », avec les mêmes réglages.

## Le principe du test

On pose le matériau sur une **sphère** de l'éditeur. Chaque pixel de la sphère lance un rayon vers l'intérieur, dans le petit monde de l'œil (globe, paupières, cils). La sphère n'est que le support : elle doit être **un peu plus grande que les paupières** (rayon de la sphère ≈ 1,2 × rayon du globe).

Repère : le regard au repos suit le **X local** de la sphère (l'avant d'Unreal), le haut suit le **Z local**. Le nez est du côté **-Y**. Pour l'autre œil, le paramètre `Mirror` inverse la droite et la gauche.

## 1. Vérifier le plugin

1. Ouvrir le projet. Menu **Edit → Plugins**, chercher **SDF Eyes** : il doit être coché (activé par défaut). Sinon, le cocher et redémarrer.

## 2. Créer le matériau

1. Dans le **Content Browser**, dossier de ton choix, clic droit → **Material**. Nom : `M_SDFEyes_Test`. Double-clic pour l'ouvrir.
2. Cliquer sur le nœud de résultat (le gros nœud du matériau). Dans le panneau **Details** :
   - **Material Domain** : `Surface` ;
   - **Blend Mode** : `Opaque` ;
   - **Shading Model** : `Unlit`.

## 3. Les entrées de position

1. Clic droit dans le graphe → chercher **Absolute World Position**, le poser.
2. Clic droit → **Transform Position**. Dans **Details** : **Source** = `Absolute World Space`, **Destination** = `Local Space`. Relier **Absolute World Position** à son entrée. → ce sera `LocalPos`.
3. Clic droit → **Camera Position**, le poser. Clic droit → un deuxième **Transform Position** (Source = `Absolute World Space`, Destination = `Local Space`), lui relier **Camera Position**. → ce sera `LocalCam`.
4. La lumière : clic droit → **Vector Parameter**, nom `LightDir`, valeur par défaut `(0.6, 0.4, 0.7)` (direction **vers** la lumière, en monde). Clic droit → **Transform** (le nœud qui transforme un vecteur) : **Source** = `World Space`, **Destination** = `Local Space`. Relier `LightDir` à son entrée. → ce sera `LightLocal`.

## 4. Les paramètres

Créer des **Scalar Parameter** (clic droit → **Scalar Parameter**) avec **exactement** ces noms et ces valeurs par défaut. Ce sont les noms du JSON copié depuis la démo, plus quelques paramètres de test.

| Nom | Défaut | Rôle |
|---|---|---|
| `EyeRadius` | 42 | Rayon du globe en unités Unreal (la sphère de l'éditeur fait 50 de rayon) |
| `Mirror` | 0 | 1 pour l'œil de l'autre côté |
| `Debug` | 0 | 1 pour la vue des parties |
| `Blink` | 0 | Clignement 0..1 |
| `GazeX` | 0 | Regard gauche / droite (radians) |
| `GazeY` | 0 | Regard haut / bas (radians) |
| `Follow` | 0.6 | Les paupières suivent le regard |
| `Pupil` | 0.13 | Rayon de la pupille (radians) |
| `EU` | 0.22 | Hauteur du bord supérieur |
| `TU` | 0.24 | Sommet du bord supérieur : nez ↔ tempe |
| `PU` | 0.93 | Forme du bord supérieur |
| `EL` | -0.22 | Hauteur du bord inférieur |
| `TL` | 0.15 | Creux du bord inférieur |
| `PL` | 1 | Forme du bord inférieur |
| `Width` | 0.62 | Écart des coins |
| `Tilt` | 0.1 | Inclinaison des coins |
| `Close` | 0.47 | Ligne de fermeture |
| `FoldGap` | 0.23 | Distance du pli |
| `Fold` | 0 | Profondeur du pli |
| `Hood` | 0.5 | Pli qui tombe côté tempe |
| `Thick` | 0.11 | Épaisseur de la paupière |
| `Lash` | 1.5 | Longueur des cils |
| `Curl` | 0.32 | Courbure des cils |
| `Car` | 0.93 | Chair du coin interne |
| `Veins` | 1 | Veines de la sclère |

## 5. Le nœud Custom

1. Clic droit → **Custom**. Le sélectionner. Dans **Details** :
   - **Output Type** : `CMOT Float 3` ;
   - **Description** : `SDFEyes` ;
   - **Include File Paths** : cliquer **+**, entrer `/Plugin/SDFEyes/Eyelids.ush`.
2. **Inputs** : cliquer **+** jusqu'à avoir **28 entrées**, et leur donner ces noms (champ **Input Name**), **dans cet ordre** :
   `LocalPos`, `LocalCam`, `LightLocal`, `EyeRadius`, `Mirror`, `Debug`, `Blink`, `GazeX`, `GazeY`, `Follow`, `Pupil`, `EU`, `TU`, `PU`, `EL`, `TL`, `PL`, `Width`, `Tilt`, `Close`, `FoldGap`, `Fold`, `Hood`, `Thick`, `Lash`, `Curl`, `Car`, `Veins`.
3. **Code** : cliquer dans le champ, coller tout le bloc ci-dessous.

```hlsl
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
if (!H.Hit) return float3(0.37, 0.26, 0.21);
return SDFEyes_DemoShade(H, P, RD, L, Debug > 0.5);
```

4. Relier chaque entrée du nœud Custom au nœud du même nom (les deux **Transform Position**, le **Transform** de la lumière, et les paramètres scalaires).
5. Relier la sortie du nœud Custom à **Emissive Color**. **Apply**, puis **Save**.

## 6. La scène de test

1. Clic droit sur `M_SDFEyes_Test` → **Create Material Instance** : `MI_SDFEyes_Test`.
2. Dans le niveau, menu **Quickly add to the project** (le cube +) → **Shapes → Sphere**. Lui appliquer `MI_SDFEyes_Test` (glisser le matériau sur la sphère).
3. Regarder la sphère **depuis son +X** (son avant) : c'est là que se trouve l'ouverture des paupières.
4. Ouvrir `MI_SDFEyes_Test`, cocher les paramètres et les faire varier : `Blink`, `GazeX`, `GazeY`, `Debug` = 1 pour la vue des parties.

## 7. Ce qu'il faut regarder

- **La même image que la démo web** avec les réglages par défaut : coins, bords, cils, ligne d'eau, chair du coin interne, veines. Pour comparer, copier un JSON depuis la démo et recopier les valeurs dans l'instance.
- **Clignement** : le bord roule sur le globe, sans trou ni bande sombre.
- **Vue des parties** : bords en jaune, coins en noir, globe en vert.
- **La couleur et la luminosité peuvent différer de la démo** : la démo applique sa propre courbe de tons, Unreal applique celle du projet (ou le post-process maison). C'est attendu ; la forme, elle, doit être la même.

## Limites connues à ce stade

- Le globe et l'iris sont ceux de la démo, provisoires (étape U4 : le globe du prototype de l'auteur).
- L'éclairage est celui de la démo (`SDFEyes_DemoShade`), pas encore l'éclairage maison.
- Les cils qui sortent de la sphère sont coupés (accepté).
- Le bord des paupières n'a pas encore d'antialiasing analytique : il peut crénelér (étape U3).
- C'est la version qui marche pas à pas : elle coûte cher en très gros plan (étape U3, version analytique).

## Si ça ne compile pas

- Message « cannot open include file » : le plugin n'est pas chargé, ou le chemin dans **Include File Paths** n'est pas exactement `/Plugin/SDFEyes/Eyelids.ush`.
- Erreur sur un nom inconnu (`EU`, `Blink`…) : une entrée du nœud Custom n'a pas le bon nom.
- Copier le message d'erreur complet (fenêtre **Stats** ou **Shader compile errors**) et me l'envoyer.
