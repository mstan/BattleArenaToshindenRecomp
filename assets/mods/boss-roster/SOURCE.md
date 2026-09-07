# Boss selection portraits

`gaia.png` and `sho.png` are new mod artwork generated with the built-in
image generation tool on September 7, 2026. They are not extracted game data.

Character design references, viewed before generation:

- Gaia: https://www.fightersgeneration.com/characters4/gaia.html
- Sho: https://www.fightersgeneration.com/characters4/sho.html

Prompt for Gaia: Create a new square character-select portrait of Gaia from
the original 1995 Battle Arena Toshinden, using the reference for character
design only. Preserve long white hair, angular ornate gold helmet, black face
mask, exposed stern mouth/chin, and black/gold armored shoulder. Head and
upper-shoulder closeup turned slightly right. Authentic 1995 Japanese fighting
game painted anime portrait, bold ink edges and limited warm colors, readable
at 128x128. Solid black background. No text, border, logos or extra characters.
Draw an original portrait, not an upscale or copy.

Prompt for Sho: Create a new square character-select portrait of Sho Shinjo
from the original 1995 Battle Arena Toshinden, using the reference for character
design only. Preserve long thick reddish-brown swept-back hair, narrow stern
eyes, angular face, purple shoulder scarf/cape and dark crimson outfit. Head
and upper-shoulder closeup turned slightly left. Authentic 1995 Japanese
fighting game painted anime portrait, bold ink edges and limited warm colors,
readable at 128x128. Solid black background. No text, border, logos, sword
covering the face or extra characters. Draw an original portrait, not a copy.

`tools/build_boss_portraits.py` encodes these sources as 128x128 RGB555 textures
for the mod. The generated C source/header are committed for ordinary builds. Re-run the
script only when intentionally regenerating these textures.
