# Extra selection portraits

`gaia.png` and `sho.png` are replacement mod artwork generated with the built-in
image-generation tool on September 7, 2026. They are not extracted game data.
The portraits target the original 1995 PlayStation game's appearance.

## References and selection

- [Gaia wiki](https://battlearenatoshinden.fandom.com/wiki/Gaia): distinguishes his
  original enclosed armor from the later white-haired, unarmored design.
- [Original armored Gaia artwork](https://www.fightersgeneration.com/np2/char1/gaia-armor.jpg),
  also in the [wiki gallery](https://static.wikia.nocookie.net/battlearenatoshinden/images/3/32/Gaia-armor.jpg/revision/latest?cb=20100312105535):
  helmet, segmented armor, additional shoulder arms, and claws.
- [Official PlayStation manual](https://secure.cdn.us.playstation.com/manuals/classic/games/battle-arena-toshinden-manual-en.pdf),
  Gaia profile: confirms that the additional arms belong to the armor.
- [Sho wiki](https://battlearenatoshinden.fandom.com/wiki/Sho_Shinjo): distinguishes
  illustrated hair from the shorter, lighter in-game appearance.
- [Sho wiki-gallery artwork](https://static.wikia.nocookie.net/battlearenatoshinden/images/6/6e/29.sho.jpg/revision/latest?cb=20190625004900):
  secondary reference for face, purple cowl, and red/gold clothing. This gallery
  spans the series; its artwork is not used as proof of the first game's model.
- Original-game captures from the local USA disc: `validation/sho-fight.png`,
  `validation/boss-vs-human-fight.png`, and
  `validation/extra-ps1-reference-4.png` / `extra-ps1-reference-7.png`.
  These establish the first game's red/black enclosed Gaia armor and Sho's
  muted brown hair, purple hood, and red tunic. Captures remain local and ignored.

The generated images replace the earlier unarmored Gaia and bright-red-haired
Sho portraits. No borrowed gallery art or disc captures are bundled with the mod.

## Generation prompts

### Gaia

Generate a NEW original square character-select portrait of GAIA as he appears in the FIRST 1995 PlayStation Battle Arena Toshinden. Reference 1 is original armored Gaia design artwork; reference 2 is an actual PS1 gameplay screenshot: GAIA is the huge red/black armored fighter on the LEFT, SHO is on the RIGHT. Use these references for precise identity and original-game armor, not as edit targets. Create an original bust portrait suitable for a 128x128 pixel fighting-game menu, filling the square with helmet, shoulders and upper chest. Gaia is completely enclosed in monstrous samurai/mechanical armor, NO exposed human face, NO skin, NO visible long white hair (that is the sequel design). His small central helmet is dark charcoal/gunmetal with a tall narrow gold central crest and angular gold visor/beak guard, dark face mask and narrow eye slits. Layered dark metallic chest/neck plates over deep crimson red, enormous high red-and-black shoulder armor formed by the folded extra mechanical arms; a little yellow/gold claw detail at the outer shoulder edge. Preserve recognizable silhouette of FIRST-game Gaia, not generic modern robot or fantasy demon. Slight three-quarter turn toward viewer's right. Render as restrained mid-1990s Japanese PS1 menu illustration with chunky painted color clusters, defined dark edges, simple 3-step shading, low-detail dithering, dark silver/gunmetal and crimson dominant with muted gold accents. Clear readable helmet and armor at tiny size. Solid black background. Square 1024x1024. No text, labels, symbols, border, watermark, or weapons crossing the face. Do not reuse the white-haired unarmored Gaia design.

### Sho

Generate a NEW original square character-select portrait of SHO SHINJO matching the FIRST 1995 PlayStation Battle Arena Toshinden in-game fighter. Reference 1 (wiki official character art) establishes Sho's actual facial identity and purple cowl/red-gold costume; reference 2 is a real PS1 gameplay screenshot of SHO on the LEFT showing his brown hair and purple hood from behind; reference 3 is another PS1 gameplay screenshot with SHO on the RIGHT showing original outfit and proportions. These are design references only, not edit targets. Create head and upper chest bust slightly turned toward viewer's LEFT, eyes forward, stern composed lean adult Japanese swordsman face, clean-shaven. His hair must be muted medium CHESTNUT BROWN with dark brown shadows and subdued tan highlights, matching the PS1 model; NEVER scarlet, bright red, orange, pink or silver. Swept back tied hair, medium-short framing strands around the cheeks, restrained crown spikes; no massive flowing red mane. Preserve lean narrow face and normal human features. His defining large folded PURPLE HOOD/COWL wraps neck and shoulders above deep red tunic, with black sleeves and restrained gold edging/shoulder bands visible. Hood down, face clearly visible. Framing head/shoulders occupies almost entire square; readable at 128x128. Render as restrained mid-1990s Japanese PS1 character-select menu art, chunky painted color clusters, defined dark edges, simple 3-step shading and low-detail dithering, not glossy modern anime splash art. Solid black background. Square1024x1024. No text, labels, symbols, borders, watermark or sword across face. This must look like the original PS1 brown-haired purple-cowled Sho, not the previous bright red-haired illustration.

## Encoding

`tools/build_boss_portraits.py` encodes both sources as 128x128 RGB555 textures.
The generated header is committed so ordinary builds do not require Pillow.
The same textures supply the extra-row icons and large selection portraits.
