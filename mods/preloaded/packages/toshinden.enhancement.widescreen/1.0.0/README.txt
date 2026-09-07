Battle Arena Toshinden Widescreen

Default: disabled. Enable on the launcher's Mods page.

Choose fixed 16:9 or Adaptive. Adaptive follows the current window/fullscreen
aspect from 4:3 through the runtime's 32:9 limit. Narrower/taller windows retain
a 4:3 game image.

The title mod expands the 3D field of view and uses title-specific renderer tags
for battle UI. Health gauges extend to the widescreen perimeter, while labels,
icons, timer, pause text, and pause panels retain their authored proportions and
anchors. Panorama backdrop strips are preserved at native texel density in the
widescreen margins. Pure 2D screens stay 4:3. Renderer tags leave guest
primitive packets untouched. All title-specific behavior is gated by this
package; disabling it restores the stock baseline on the next launch.
