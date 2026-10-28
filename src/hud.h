#pragma once

// HUD Layout "Screen Edges": the HUD elements are moved to the edges of a picture that is not 16:9. The black
// backgrounds of the loading screens, the death screen, the idol menu and the notifications cover the whole picture.
void InstallHudLayout();
// Scaleform recomputed the layout of a movie (its file name) for a new viewport (width and height in twips).
void MovieViewport(const char *name, float width, float height);
