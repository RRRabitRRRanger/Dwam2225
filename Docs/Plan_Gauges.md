# Plan — Jauges (composant réutilisable)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_3C]]

> Statut : 💭 **idée / avis**, à ne pas coder pour l'instant (auteur, 2026-10-04).

## L'existant (auteur)

- `Content/RPG/Systems/Gauge/Comp_Gauges` : **une jauge par composant**. Nom (`Gauge Name`), valeur actuelle, valeur max, `Change Amount` (ajouter / retirer), événements « changée », « vide », « pleine ».
- Gameplay Tags (`Config/DefaultGameplayTags.ini`) : `Gauge`, `Gauge.Health`, `Gauge.Poise`, `Gauge.Stamina`.
- Intention : **des jauges, tout simplement**, plutôt que de refaire la même chose encore et encore. Pour la vie, l'équilibre et l'endurance, mais aussi un **manomètre**, un **jouet à ressort**, etc.
- L'auteur veut **le tag et le nom** : le tag pour ce dont les systèmes ont besoin, le nom pour les objets uniques.

## Avis (Claude, 2026-10-04)

- **Bonne idée** : « une valeur bornée + des événements » revient partout.
- **Tag = identité** pour la logique du jeu : pas de faute de frappe, requêtes hiérarchiques (`Gauge.Vital.*`). Même un objet unique peut avoir un tag générique (`Gauge.Device.Pressure`).
- **Nom = étiquette libre** d'un objet unique. Pour un texte affiché, un `Text` (traduisible) plutôt qu'un `Name`.
- **Une jauge par composant** : bien (chaque jauge a ses événements). Ajouter une fonction « trouver la jauge `Gauge.X` sur un acteur ».
- **À ajouter un jour** :
  - un **minimum** (pas forcément 0 : température, pression) ;
  - des **seuils nommés**, avec événement au franchissement dans les deux sens (généralise « vide » et « plein ») ;
  - **« essayer de consommer »** → oui / non (endurance pour grimper, sprinter, enjamber) ;
  - le **montant réellement appliqué** en retour, et la **cause** du changement (indispensable au combat) ;
  - le lissage de l'affichage reste dans l'UI.
- **Régénération / décroissance** : nécessaire, mais les jeux la font de mille façons (délai après l'effort, paliers, selon l'état, plusieurs sources). D'où l'idée : des **règles de régénération interchangeables** (petites classes ou Data Assets choisis par jauge), plutôt qu'un seul comportement codé en dur.
- **Gameplay Ability System** : ses « attributs » font la même chose en beaucoup plus lourd ; inutile ici.

## Liens

- **Stamina** : endurance de l'escalade, du sprint, de l'enjambement ([[Plan_3C]]).
- **Poise** : parade, coups lourds, saut sur un ennemi ([[Plan_3C]] § 7).
