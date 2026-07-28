# Asset acquisition plan

Status: **no asset has been searched for, downloaded, purchased or imported.** This
document defines the process and the rules; `AssetRequirements.csv` defines what is
needed; `Content/AssetRegistry/ThirdPartyAssets.json` is the (currently empty)
registry of what has actually been acquired. Nothing enters the project without a
row in that registry.

## Hard rules

1. **Nothing from an existing Command & Conquer game.** No models, textures, music,
   sound, video, maps, logos, fonts, character likenesses, voice lines, source code
   or extracted data. Not for placeholders, not for reference renders, not "just
   during prototyping".
2. **A licence is read on the source page before download, every time.** Free does
   not mean redistributable, and a CC-BY-NC or "editorial use only" asset cannot ship
   in a commercial build. Site-wide claims are not evidence; the asset's own page is.
3. **No pirated mirrors, leaked marketplace packs, reuploads, or assets of unclear
   provenance.** If the chain of rights cannot be established, the asset is not used.
4. **No circumventing access controls.** Where a download needs a purchase or an
   account login, the exact item, its page and its price are listed for the account
   owner, who performs the purchase. Nobody else's account is used.
5. **Third-party content is quarantined.** It lands in `Content/ThirdParty/<Source>/`
   unmodified, with the original archive and the licence text retained. Derived
   materials go in `Content/Game/`. Originals are never edited in place.
6. **Attribution is generated, not remembered.** The credits screen and
   `Docs/Attributions.md` are produced from the registry, so an asset that is used
   but unregistered fails the build.

## Preferred sources, in order

| Priority | Source | Why | Caveat |
| --- | --- | --- | --- |
| 1 | Assets the account owner already holds (Fab / Marketplace library, Quixel Megascans) | already licensed, Unreal-native | must confirm each item's licence permits the intended distribution |
| 2 | Fab, with a licence permitting commercial redistribution in a packaged game | Unreal-native, clear terms | per-listing licence varies; read each one |
| 3 | Poly Haven (CC0), ambientCG (CC0), Kenney (CC0) | public domain, no attribution obligation | quality and style vary; needs reprocessing |
| 4 | Freesound (CC0 / CC-BY only), Sonniss GDC bundles | large, well-documented audio | Freesound licences are per-file, including NC files that must be rejected |
| 5 | Sketchfab, only where the item is explicitly licensed for commercial use | breadth | many items are CC-BY-NC or "editorial"; verify per item |
| 6 | Original procedural placeholders generated in-project | always safe, always legal | costs authoring time |

Deliberately not used: OpenGameArt (per-file licences are frequently unverifiable and
often GPL-contaminated), any "free asset" aggregator that rehosts other sites'
content, AI-generated assets whose training provenance is unclear enough to create a
distribution risk.

## Process per asset

1. The requirement exists as a row in `AssetRequirements.csv` with a budget
   (polycount, texture resolution, material count, LOD/Nanite expectations).
2. Candidates are identified and their licence pages read and recorded verbatim.
3. If the item needs a purchase or an account, it is added to the "requires account
   owner" list below with the exact URL and price, and work continues on other items.
4. On acquisition: download through the official route, record SHA-256, store the
   licence text, and write the registry row.
5. Import through the managed pipeline into `Content/ThirdParty/<Source>/`, then
   build derived assets in `Content/Game/`.
6. Validation commands check naming, redirectors, reference integrity, texture size,
   material count, missing LODs, oversized skeletal meshes and unused assets.

## Placeholder policy

A missing asset is replaced by a **procedural placeholder generated in-project** --
correctly scaled, with the right silhouette proportions, faction colour mask and
collision -- not by a grey cube, and never by a legally questionable stand-in. Each
placeholder carries an art requirement note so the eventual replacement matches the
gameplay dimensions the simulation already assumes.

## Requires the account owner

Nothing yet. This section will list exact items, URLs and prices once the
requirements in `AssetRequirements.csv` reach the stage where art is needed --
which, per `Docs/Roadmap.md`, is after the navigation and presentation stages. Buying
art before the systems that display it exist is how asset budgets get wasted.
