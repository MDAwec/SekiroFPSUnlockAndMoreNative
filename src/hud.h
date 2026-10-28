#pragma once

// HUD Layout "Screen Edges": the HUD elements are moved to the edges of a picture that is not 16:9, and the black
// background of the loading screens covers the whole picture.
void InstallHudLayout();
// Scaleform recomputed the layout of a movie for a new viewport (width and height in twips).
void HudMovieViewport(float width, float height);
void LoadingMovieViewport(float width, float height);
