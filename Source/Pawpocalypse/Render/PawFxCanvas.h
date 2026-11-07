#pragma once

#include "CoreMinimal.h"

// Godot's CanvasItem draw calls, for porting the effects line by line (phase 6 slice 3): draw_circle, draw_arc, draw_line,
// draw_polyline, draw_colored_polygon, draw_polygon with per-point colours. Each call is tessellated at once into
// vertex-coloured triangles, in call order, at the depth set by SetZ; UPawSpriteRenderer::SetFx draws a canvas as one
// translucent mesh section (primitive order kept: Godot's painter's order). Colours are Godot Colors, i.e. sRGB floats with a
// straight alpha; they are stored as sRGB bytes and decoded exactly by the material (M_BackdropFill).
//
// Not reproduced: Godot's antialiased line feather (FXAA smooths the edges).
class PAWPOCALYPSE_API FPawFxCanvas
{
public:
	static constexpr int32 CircleSegments = 64; // Godot's canvas_item_add_circle

	void Reset();
	void SetZ(double Z) { CurrentZ = Z; }
	bool IsEmpty() const { return Indices.Num() == 0; }

	void Circle(const FVector2D& Centre, double Radius, const FLinearColor& Color);
	// PointCount points from Start to End (radians, clockwise on screen), stroked Width px wide.
	void Arc(const FVector2D& Centre, double Radius, double Start, double End, int32 PointCount, const FLinearColor& Color, double Width);
	void Line(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, double Width);
	// A strip of Width px centred on the points; each joint takes the average of its two segments' normals, widened so the
	// stroke keeps its width round a bend (a mitre, capped).
	void Polyline(TConstArrayView<FVector2D> Points, const FLinearColor& Color, double Width);
	// A convex (or star-shaped about its first point) polygon, one colour.
	void Polygon(TConstArrayView<FVector2D> Points, const FLinearColor& Color);
	// The same with a colour per point (interpolated across, as Godot's draw_polygon).
	void Polygon(TConstArrayView<FVector2D> Points, TConstArrayView<FLinearColor> Colors);
	// A polygon star-shaped about Centre (a cloud: its radius wobbles but never reaches it), fanned from there: Godot's
	// draw_colored_polygon triangulates any simple outline, a fan from the first point only a convex one.
	void Fan(const FVector2D& Centre, TConstArrayView<FVector2D> Points, const FLinearColor& Color);
	// draw_polyline_colors(points, colours, width, antialiased): a colour per point, blended along each segment; joints as
	// Polyline. Feather > 0: an extra strip that fades to clear on each side (Godot's antialiased line; in the UI, where no
	// FXAA runs over Slate).
	void Polyline(TConstArrayView<FVector2D> Points, TConstArrayView<FLinearColor> PointColors, double Width, double Feather);

	// Godot's Color.from_hsv (h, s, v in 0..1), alpha 1.
	static FLinearColor FromHsv(double H, double S, double V);

	// The tessellation: field px with each vertex's world Z, sRGB colours, triangle indices.
	TArray<FVector> Verts;
	TArray<FColor> Colors;
	TArray<int32> Indices;

private:
	int32 Add(const FVector2D& P, const FColor& C);
	static FColor Bytes(const FLinearColor& C);
	double CurrentZ = 0.0;
};
